/*
 * Copyright (c) 2020 MariaDB Corporation Ab
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file and at www.mariadb.com/bsl11.
 *
 * Change Date: 2024-08-24
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2 or later of the General
 * Public License.
 */

#pragma once

#include "mongodbclient.hh"
#include <string>
#include <utility>
#include <vector>
#include <bsoncxx/builder/basic/document.hpp>
#include <maxscale/buffer.hh>
#include "../../filter/masking/mysql.hh"
#include "mxsmongo.hh"

namespace mxsmongo
{

class Database;

enum class Conversion
{
    STRICT,
    RELAXED
};

template<class T>
T element_as(const std::string& command,
             const char* zKey,
             const bsoncxx::document::element& element,
             Conversion conversion = Conversion::STRICT);

template<>
bsoncxx::document::view element_as<bsoncxx::document::view>(const std::string& command,
                                                            const char* zKey,
                                                            const bsoncxx::document::element& element,
                                                            Conversion conversion);

template<>
bsoncxx::array::view element_as<bsoncxx::array::view>(const std::string& command,
                                                      const char* zKey,
                                                      const bsoncxx::document::element& element,
                                                      Conversion conversion);

template<>
std::string element_as<std::string>(const std::string& command,
                                    const char* zKey,
                                    const bsoncxx::document::element& element,
                                    Conversion conversion);

template<>
bool element_as<bool>(const std::string& command,
                      const char* zKey,
                      const bsoncxx::document::element& element,
                      Conversion conversion);

class Command
{
public:
    using DocumentVector = std::vector<bsoncxx::document::view>;
    using DocumentArguments = std::unordered_map<std::string, DocumentVector>;

    template<class ConcretePacket>
    Command(const std::string& name,
            Database* pDatabase,
            GWBUF* pRequest,
            const ConcretePacket& req,
            const bsoncxx::document::view& doc,
            const DocumentArguments& arguments)
        : m_name(name)
        , m_database(*pDatabase)
        , m_pRequest(gwbuf_clone(pRequest))
        , m_req(req)
        , m_doc(doc)
        , m_arguments(arguments)
        , m_append_checksum(checksum_used(req))
    {
    }

    static std::unique_ptr<Command> get(mxsmongo::Database* pDatabase,
                                        GWBUF* pRequest,
                                        const mxsmongo::Query& req,
                                        const bsoncxx::document::view& doc,
                                        const DocumentArguments& arguments);

    static std::unique_ptr<Command> get(mxsmongo::Database* pDatabase,
                                        GWBUF* pRequest,
                                        const mxsmongo::Msg& req,
                                        const bsoncxx::document::view& doc,
                                        const DocumentArguments& arguments);

    enum State
    {
        BUSY,
        READY
    };

    virtual ~Command();

    virtual GWBUF* execute() = 0;

    virtual State translate(GWBUF& mariadb_response, GWBUF** ppMongo_response);

    GWBUF* create_empty_response() const;

    GWBUF* create_hard_error(const std::string& message, int mongo_code) const;
    GWBUF* create_soft_error(const std::string& message, int mongo_code) const;
    GWBUF* create_mariadb_error(const std::string& message, int mongo_code,
                                const std::string& mariadb_message, int mariadb_code) const;

    static void check_write_batch_size(int size);

protected:
    template<class Type>
    bool optional(const char* zKey, Type* pElement, Conversion conversion = Conversion::STRICT) const
    {
        bool rv = false;

        auto element = m_doc[zKey];

        if (element)
        {
            *pElement = element_as<Type>(m_name, zKey, element, conversion);
            rv = true;
        }

        return rv;
    }

    template<class Type>
    Type required(const char* zKey, Conversion conversion = Conversion::STRICT) const
    {
        auto element = m_doc[zKey];

        if (!element)
        {
            std::stringstream ss;
            ss << "BSON field '" << m_name << "." << zKey << "' is missing but a required field";
            throw SoftError(ss.str(), error::LOCATION40414);
        }

        return element_as<Type>(m_name, zKey, element, conversion);
    }

    /**
     * Converts the values of a 'skip' and 'limit' to the corresponding
     * LIMIT clause.
     *
     * @return A LIMIT clause, if 'skip' and/or 'limit' are present in the
     *         command object, otherwise an empty string.
     */
    std::string convert_skip_and_limit() const;

    template<class T>
    T value_as(Conversion conversion = Conversion::STRICT) const
    {
        return required<T>(m_name.c_str(), conversion);
    }

    enum Quoted
    {
        NO,
        YES
    };

    const std::string& table(Quoted quoted = Quoted::YES) const;

    void free_request();

    void send_downstream(const std::string& sql);

    GWBUF* create_response(const bsoncxx::document::value& doc) const;

    GWBUF* translate_resultset(std::vector<std::string>& extractions, GWBUF* pMariadb_response);

    void add_error(bsoncxx::builder::basic::array& builder, const ComERR& err, int index);
    void add_error(bsoncxx::builder::basic::document& builder, const ComERR& err);

    const std::string       m_name;
    Database&               m_database;
    GWBUF*                  m_pRequest;
    Packet                  m_req;
    bsoncxx::document::view m_doc;
    DocumentArguments       m_arguments;
    std::string             m_last_statement;

private:
    bool checksum_used(const Msg& req)
    {
        return req.checksum_present();
    }

    bool checksum_used(const Query&)
    {
        return false;
    }

    std::pair<GWBUF*, uint8_t*> create_reply_response_buffer(size_t size_of_documents,
                                                             size_t nDocuments) const;

    GWBUF* create_reply_response(size_t size_of_documents, const std::
                                 vector<bsoncxx::document::value>& documents) const;
    GWBUF* create_reply_response(const bsoncxx::document::value& doc) const;

    GWBUF* create_msg_response(const bsoncxx::document::value& doc) const;

    std::string create_leaf_entry(const std::string& extraction, const std::string& value) const;

    std::string create_nested_entry(const std::string& extraction, const std::string& value) const;

    std::string create_entry(const std::string& extraction, const std::string& value) const;

    bool                m_append_checksum { false };
    mutable std::string m_quoted_table;
    mutable std::string m_unquoted_table;
};

}
