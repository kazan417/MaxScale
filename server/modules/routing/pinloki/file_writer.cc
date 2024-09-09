/*
 * Copyright (c) 2020 MariaDB Corporation Ab
 * Copyright (c) 2023 MariaDB plc, Finnish Branch
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file and at www.mariadb.com/bsl11.
 *
 * Change Date: 2027-04-10
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2 or later of the General
 * Public License.
 */

#include "pinloki.hh"
#include "file_writer.hh"
#include "file_reader.hh"
#include "config.hh"

#include <maxscale/protocol/mariadb/mysql.hh>
#include <mariadb_rpl.h>
#include <iostream>
#include <iomanip>

namespace
{

/** The next file name has the same base name as the file from the master, but
 *    the counter portion is generated here.  By using the master base_name the
 *    event length stays the same, which means the 'next_pos' in the events do
 *    not have to be modifed.
 */
std::string next_file_name(const std::string& master, const std::string& prev)
{
    using namespace  std;

    auto base_name = master.substr(0, master.find_last_of('.'));

    auto num = 1;
    if (!prev.empty())
    {
        auto num_str = prev.substr(prev.find_last_of(".") + 1);
        num = 1 + atoi(num_str.c_str());
    }

    return MAKE_STR(base_name << '.' << setfill('0') << setw(6) << num);
}

bool fde_events_match(const maxsql::RplEvent& a, const maxsql::RplEvent& b)
{
    bool match = false;

    if (a.buffer_size() == b.buffer_size() && memcmp(a.pHeader(), b.pHeader(), mxq::RPL_HEADER_LEN) == 0)
    {
        auto a_fde = a.format_description();
        auto b_fde = b.format_description();

        if (a_fde.checksum == b_fde.checksum && a_fde.server_version == b_fde.server_version)
        {
            match = true;
        }
    }

    return match;
}

bool is_part_of_preamble(mariadb_rpl_event event_type)
{
    return event_type == GTID_LIST_EVENT
           || event_type == FORMAT_DESCRIPTION_EVENT
           || event_type == BINLOG_CHECKPOINT_EVENT;
}
}

namespace pinloki
{
FileWriter::FileWriter(InventoryWriter* inv, const Writer& writer)
    : m_inventory(*inv)
    , m_writer(writer)
{
}

void FileWriter::add_event(maxsql::RplEvent& rpl_event)     // FIXME, move into here
{
    auto etype = rpl_event.event_type();

    if (etype == HEARTBEAT_LOG_EVENT)
    {
        // Heartbeat event, don't process it
    }
    else if (etype == ROTATE_EVENT)
    {
        // Regardless if this is steady state or pinloki just started to run there
        // will be an initial ROTATE_EVENT followed by a FORMAT_DESCRIPTION_EVENT
        // which is when the actual rotate will be performed, unless it has already
        // been performed. In the latter case events will be appended to an existing file.
        m_rotate = rpl_event.rotate();
    }
    else
    {
        if (etype == FORMAT_DESCRIPTION_EVENT
            && (m_ignore_preamble = open_for_appending(rpl_event)) == false)
        {
            mxb_assert(m_rotate.file_name.empty() == false);
            rpl_event.set_next_pos(PINLOKI_MAGIC.size() + rpl_event.buffer_size());

            perform_rotate(m_rotate, rpl_event);
        }
        else
        {
            m_ignore_preamble = m_ignore_preamble && is_part_of_preamble(etype);

            if (!m_ignore_preamble)
            {
                rpl_event.set_next_pos(m_current_pos.write_pos + rpl_event.buffer_size());

                if (etype == GTID_LIST_EVENT)
                {
                    write_gtid_list(m_current_pos);
                }
                else if (etype != STOP_EVENT && etype != BINLOG_CHECKPOINT_EVENT)
                {
                    write_rpl_event(m_current_pos, rpl_event);
                }
            }
        }

        m_rotate.file_name.clear();
    }
}

bool FileWriter::open_for_appending(const maxsql::RplEvent& fmt_event)
{
    if (!m_newborn)
    {
        return false;
    }

    m_newborn = false;

    auto last_file_name = last_string(m_inventory.file_names());
    if (last_file_name.empty())
    {
        return false;
    }

    IFStreamReader log_file_in{last_file_name};
    if (!log_file_in.is_open())
    {
        // not really trying to open for writing, but
        MXB_THROW(BinlogWriteError, "Could not open " << last_file_name << " for append check.");
    }

    log_file_in.advance(MAGIC_SIZE);
    long file_pos = log_file_in.bytes_read();
    mxb_assert(log_file_in.at_pos(file_pos));

    maxsql::RplEvent event = maxsql::RplEvent::read_event(log_file_in, &file_pos);
    mxb_assert(event.event_type() == FORMAT_DESCRIPTION_EVENT);

    return fde_events_match(event, fmt_event) && open_binlog(last_file_name);
}

bool FileWriter::open_binlog(const std::string& file_name)
{
    IFStreamReader log_file_in{file_name};

    if (!log_file_in.is_open())
    {
        return false;
    }

    log_file_in.advance(MAGIC_SIZE);
    long read_pos = log_file_in.bytes_read();
    maxsql::RplEvent file_event = maxsql::RplEvent::read_event(log_file_in, &read_pos);
    mxb_assert(file_event.event_type() == FORMAT_DESCRIPTION_EVENT);

    if (auto encrypt = maxsql::RplEvent::read_event(log_file_in, &read_pos))
    {
        // File has more events after the FDE.
        if (encrypt.event_type() == START_ENCRYPTION_EVENT)
        {
            const auto& cnf = m_inventory.config();
            m_encrypt = mxq::create_encryption_ctx(cnf.key_id(), cnf.encryption_cipher(),
                                                   file_name, encrypt);
        }
    }

    m_current_pos.name = file_name;
    m_current_pos.file.open(m_current_pos.name, std::ios_base::in | std::ios_base::out);
    m_current_pos.file.seekp(0, std::ios_base::end);
    m_current_pos.write_pos = m_current_pos.file.tellp();

    return m_current_pos.file.good();
}

void FileWriter::perform_rotate(const maxsql::Rotate& rotate, const maxsql::RplEvent& fmt_event)
{
    auto master_file_name = rotate.file_name;
    auto last_file_name = !m_current_pos.name.empty() ?
        m_current_pos.name : last_string(m_inventory.file_names());
    auto to_file_name = m_inventory.config().path(next_file_name(master_file_name, last_file_name));

    if (!m_current_pos.file.is_open() && !last_file_name.empty())
    {
        open_binlog(last_file_name);
    }

    WritePosition previous_pos {std::move(m_current_pos)};
    // The ROTATE event at the end of the old file must be encrypted with the old context. Opening a new
    // binlog resets the context and sets it up for the next one.
    std::unique_ptr<mxq::EncryptCtx> tmp(m_encrypt.release());
    create_binlog(to_file_name, fmt_event);

    if (previous_pos.file.is_open())
    {
        m_encrypt.swap(tmp);
        write_rotate(previous_pos, to_file_name);
        m_encrypt.swap(tmp);
        previous_pos.file.close();

        if (!previous_pos.file.good())
        {
            MXB_THROW(BinlogWriteError, "File " << previous_pos.name
                                                << " did not close (flush) properly during rotate: "
                                                << errno << ", " << mxb_strerror(errno));
        }
    }
}

void FileWriter::create_binlog(const std::string& file_name, const maxsql::RplEvent& fmt_event)
{
    m_current_pos.name = file_name;
    m_current_pos.file.open(file_name, std::ios_base::out);
    m_current_pos.write_pos = 0;

    if (!m_current_pos.file.good())
    {
        MXB_THROW(BinlogWriteError, "Could not create " << file_name << " for writing.");
    }

    std::vector<char> buf;
    buf.insert(buf.end(), PINLOKI_MAGIC.begin(), PINLOKI_MAGIC.end());
    buf.insert(buf.end(), fmt_event.pBuffer(), fmt_event.pBuffer() + fmt_event.buffer_size());

    if (const auto& key_id = m_inventory.config().key_id(); !key_id.empty())
    {
        // Reset the encryption context for every new binlog. Both the FORMAT_DESCRIPTION and the
        // START_ENCRYPTION events must be unencrypted even if the previous file was also encrypted.
        m_encrypt.reset();

        if (auto key_manager = mxs::key_manager())
        {
            auto [ok, vers, key] = key_manager->get_key(key_id);

            if (ok)
            {
                maxsql::RplEvent event{mxq::create_start_encryption_event(fmt_event.server_id(),
                                                                          vers,
                                                                          buf.size())};

                auto start_encryption = event.start_encryption_event();
                const auto cipher = m_inventory.config().encryption_cipher();
                m_encrypt = std::make_unique<mxq::EncryptCtx>(cipher, key, start_encryption.iv);

                buf.insert(buf.end(), event.pBuffer(), event.pBuffer() + event.buffer_size());
            }
            else
            {
                MXB_THROW(mxq::EncryptionError, "Failed to open encryption key '" << key_id << "'.");
            }
        }
        else
        {
            MXB_THROW(mxq::EncryptionError,
                      "Encryption key ID is set to '" << key_id << "' but key manager is not enabled. "
                                                      << "Cannot write encrypted binlog files.");
        }
    }

    write_buffer(m_current_pos, buf.data(), buf.size());
    m_current_pos.file.flush();

    if (!m_current_pos.file.good())
    {
        MXB_THROW(BinlogWriteError, "Failed to write header to " << file_name << ". Deleting file.");
        remove(file_name.c_str());
    }
}

void FileWriter::write_rpl_event(WritePosition& pos, const maxsql::RplEvent& rpl_event)
{
    if (m_encrypt)
    {
        std::vector<char> plaintext(rpl_event.pBuffer(), rpl_event.pEnd());
        auto encrypted = m_encrypt->encrypt_event(plaintext, pos.write_pos);
        write_buffer(pos, encrypted.data(), encrypted.size());
    }
    else
    {
        write_buffer(pos, rpl_event.pBuffer(), rpl_event.buffer_size());
    }

    if (!pos.file.good())
    {
        MXB_THROW(BinlogWriteError, "Could not write event to " << pos.name);
    }
}

void FileWriter::write_buffer(WritePosition& pos, const char* ptr, size_t bytes)
{
    pos.file.write(ptr, bytes);
    pos.write_pos += bytes;
    pos.file.flush();

    if (!pos.file.good())
    {
        MXB_THROW(BinlogWriteError, "Could not write event to " << pos.name);
    }
}

void FileWriter::write_rotate(WritePosition& pos, const std::string& to_file_name)
{
    auto vec = maxsql::create_rotate_event(basename(to_file_name.c_str()),
                                           m_inventory.config().server_id(),
                                           pos.write_pos,
                                           mxq::Kind::Real);

    mxq::RplEvent event(std::move(vec));
    write_rpl_event(pos, event);

    if (!pos.file.good())
    {
        MXB_THROW(BinlogWriteError, "Could not write ROTATE to " << pos.name);
    }
}


void FileWriter::write_gtid_list(WritePosition& pos)
{
    constexpr int HEADER_LEN = 19;
    auto gtid_list = m_writer.get_gtid_io_pos();
    const auto NUM_GTIDS = gtid_list.gtids().size();
    const size_t EVENT_LEN = HEADER_LEN + 4 + NUM_GTIDS * (4 + 4 + 8) + 4;

    std::vector<char> data(EVENT_LEN);
    uint8_t* ptr = (uint8_t*)&data[0];

    // Zero timestamp
    mariadb::set_byte4(ptr, 0);
    ptr += 4;

    // The event
    *ptr++ = GTID_LIST_EVENT;

    // server id
    mariadb::set_byte4(ptr, m_inventory.config().server_id());
    ptr += 4;

    // Event length
    mariadb::set_byte4(ptr, EVENT_LEN);
    ptr += 4;

    // Next position
    mariadb::set_byte4(ptr, pos.write_pos + EVENT_LEN);
    ptr += 4;

    // No flags (this is a real event)
    mariadb::set_byte2(ptr, 0);
    ptr += 2;

    // Number of gtids to follow:
    mariadb::set_byte4(ptr, NUM_GTIDS);
    ptr += 4;
    // Gtids:
    for (const auto& gtid : gtid_list.gtids())
    {
        mariadb::set_byte4(ptr, gtid.domain_id());
        ptr += 4;
        mariadb::set_byte4(ptr, gtid.server_id());
        ptr += 4;
        mariadb::set_byte8(ptr, gtid.sequence_nr());
        ptr += 8;
    }

    // Checksum
    mariadb::set_byte4(ptr, crc32(0, (uint8_t*)data.data(), data.size() - 4));

    mxq::RplEvent event(std::move(data));
    write_rpl_event(pos, event);
}
}
