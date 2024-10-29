/*
 * Copyright (c) 2020 MariaDB Corporation Ab
 * Copyright (c) 2023 MariaDB plc, Finnish Branch
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file and at www.mariadb.com/bsl11.
 *
 * Change Date: 2026-03-20
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2 or later of the General
 * Public License.
 */

#include "find_gtid.hh"
#include "inventory.hh"
#include "pinloki.hh"
#include "rpl_event.hh"
#include "file_reader.hh"
#include <maxbase/log.hh>
#include <maxscale/routingworker.hh>
#include <fstream>
#include <iostream>
#include <iomanip>

namespace pinloki
{
inline bool operator<(const GtidPosition& lhs, const GtidPosition& rhs)
{
    if (lhs.file_name.empty())
    {
        return true;
    }
    else if (rhs.file_name.empty())
    {
        return false;
    }

    auto lhs_pos = lhs.file_name.find_last_of(".");
    auto rhs_pos = lhs.file_name.find_last_of(".");

    auto lhs_num = std::atoi(&lhs.file_name[lhs_pos + 1]);
    auto rhs_num = std::atoi(&rhs.file_name[rhs_pos + 1]);

    return lhs_num < rhs_num || (lhs_num == rhs_num && lhs.file_pos < rhs.file_pos);
}

std::vector<GtidPosition> FileReader::find_gtid_position(const std::vector<maxsql::Gtid>& gtids)
{
    mxb::WatchdogNotifier::Workaround workaround(mxs::RoutingWorker::get_current());

    std::vector<GtidPosition> ret;
    // Simple linear search. If there can be a lot of files, make this a binary search, or
    // if it really becomes slow, create an index
    const auto& file_names = m_inventory.file_names();

    // Search in reverse because the gtid is likely be one of the latest files, and
    // the search can stop as soon as the gtid is greater than the gtid list in the file,
    // uh, expect for the first file which doesn't have a GTID_LIST_EVENT.

    // TODO, don't do one gtid at a time, modify the search to do all in one go.
    for (const auto& gtid : gtids)
    {
        GtidPosition pos {gtid};
        auto last_one = rend(file_names) - 1;   // which is the first, oldest file
        for (auto ite = rbegin(file_names); ite != rend(file_names); ++ite)
        {
            if (search_file(*ite, gtid, &pos, ite == last_one))
            {
                break;
            }
        }

        ret.push_back(pos);
    }

    sort(begin(ret), end(ret));

    return ret;
}

/**
 * @brief search_gtid_in_file
 * @param file
 * @param from_pos
 * @return position, or 0 if not found
 */
long FileReader::search_gtid_in_file(std::ifstream& file, const std::unique_ptr<mxq::EncryptCtx>& encrypt,
                                     long file_pos, const maxsql::Gtid& gtid)
{
    long pos = file.tellg();

    while (maxsql::RplEvent rpl = maxsql::RplEvent::read_event(file, encrypt))
    {
        if (rpl.event_type() == GTID_EVENT)
        {
            maxsql::GtidEvent event = rpl.gtid_event();

            if (event.gtid.domain_id() == gtid.domain_id() && event.gtid.sequence_nr() == gtid.sequence_nr())
            {
                return pos;
            }
        }

        pos += rpl.real_size();
        mxb_assert(pos == file.tellg());
    }

    return 0;
}

bool FileReader::search_file(const std::string& file_name,
                             const maxsql::Gtid& gtid,
                             GtidPosition* ret_pos,
                             bool first_file)
{
    if (first_file)
    {
        // First file, no need to seek into the binlog: the event is guaranteed to be in the future
        ret_pos->file_name = file_name;
        ret_pos->file_pos = PINLOKI_MAGIC.size();
        return true;
    }

    std::ifstream file {file_name, std::ios_base::in | std::ios_base::binary};

    if (!file.good())
    {
        MXB_SERROR("Could not open binlog file " << file_name);
        return false;
    }

    enum GtidListResult {NotFound, GtidInThisFile, GtidInPriorFile};
    GtidListResult result = NotFound;
    long file_pos = PINLOKI_MAGIC.size();
    file.seekg(file_pos);
    std::unique_ptr<mxq::EncryptCtx> encrypt;

    while (result == NotFound)
    {
        maxsql::RplEvent rpl = mxq::RplEvent::read_event(file, encrypt);

        if (!rpl)
        {
            break;
        }

        if (rpl.event_type() == START_ENCRYPTION_EVENT)
        {
            const auto& cnf = m_inventory.config();
            encrypt = mxq::create_encryption_ctx(cnf.key_id(), cnf.encryption_cipher(), file_name, rpl);
        }
        else if (rpl.event_type() == GTID_LIST_EVENT)
        {
            maxsql::GtidListEvent event = rpl.gtid_list();

            uint64_t highest_seq = 0;
            bool domain_in_list = false;

            for (const auto& tid : event.gtid_list.gtids())
            {
                if (tid.domain_id() == gtid.domain_id())
                {
                    domain_in_list = true;
                    highest_seq = std::max(highest_seq, tid.sequence_nr());
                }
            }

            if (!domain_in_list || (domain_in_list && highest_seq < gtid.sequence_nr()))
            {
                result = GtidInThisFile;
            }
            else if (highest_seq == gtid.sequence_nr())
            {
                result = GtidInPriorFile;
            }
            else
            {
                break;
            }
        }
    }

    bool success = false;

    if (result == GtidInThisFile || result == GtidInPriorFile)
    {
        // If the result is GtidInThisFile, the GTID is somewhere in this file. In this case the
        // GTID_LIST_EVENT had a GTID in it that was smaller than the target GTID. If the result is
        // GtidInPriorFile, the GTID list contained the exact target GTID. In this case no GTIDs need to be
        // skipped. In both cases the skipping of already replicated transactions is handled in send_event().
        success = true;
        ret_pos->file_name = file_name;
        ret_pos->file_pos = PINLOKI_MAGIC.size();
    }

    return success;
}


maxsql::GtidList find_last_gtid_list(const InventoryWriter& inv)
{
    maxsql::GtidList ret;
    if (inv.file_names().empty())
    {
        return ret;
    }

    auto file_name = inv.file_names().back();
    std::ifstream file {file_name, std::ios_base::in | std::ios_base::binary};
    long file_pos = PINLOKI_MAGIC.size();
    file.seekg(file_pos);
    std::unique_ptr<mxq::EncryptCtx> encrypt_ctx;

    while (auto rpl = mxq::RplEvent::read_event(file, encrypt_ctx))
    {
        switch (rpl.event_type())
        {
        case START_ENCRYPTION_EVENT:
            encrypt_ctx = mxq::create_encryption_ctx(inv.config().key_id(), inv.config().encryption_cipher(),
                                                     file_name, rpl);
            break;

        case GTID_LIST_EVENT:
            {
                auto event = rpl.gtid_list();

                for (const auto& gtid : event.gtid_list.gtids())
                {
                    ret.replace(gtid);
                }
            }
            break;

        case GTID_EVENT:
            {
                auto event = rpl.gtid_event();
                ret.replace(event.gtid);
            }
            break;

        case STOP_EVENT:
        case ROTATE_EVENT:
            break;

        default:
            MXB_SDEBUG("GTID search: " << rpl);
        }
    }

    return ret;
}
}
