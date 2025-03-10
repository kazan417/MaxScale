/*
 * Copyright (c) 2016 MariaDB Corporation Ab
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

/**
 * - set global max_allowed_packet=10000000
 * - pretare statement INSERT INTO long_blob_table(x, b) VALUES(1, ?)
 * - load chunks
 * - execute statement
 * - wait 5 seconds
 * - repeat test 2 times
 */

#include <maxtest/testconnections.hh>

void run_test(TestConnections& test, size_t size, int chunks)
{
    test.reset_timeout();
    const char* insert_stmt = "INSERT INTO long_blob_table(x, b) VALUES(1, ?)";
    MYSQL* conn = test.maxscale->conn_rwsplit;
    MYSQL_STMT* stmt = mysql_stmt_init(conn);

    test.add_result(mysql_stmt_prepare(stmt, insert_stmt, strlen(insert_stmt)),
                    "Error preparing stmt: %s",
                    mysql_stmt_error(stmt));

    MYSQL_BIND param;
    param.buffer_type = MYSQL_TYPE_STRING;
    param.is_null = 0;

    test.add_result(mysql_stmt_bind_param(stmt, &param),
                    "Binding parameter failed: %s",
                    mysql_stmt_error(stmt));

    std::string data(size, '.');
    test.tprintf("Sending %zu x %d bytes of data", size, chunks);

    for (int i = 0; i < chunks; i++)
    {
        test.add_result(mysql_stmt_send_long_data(stmt, 0, data.c_str(), data.size()),
                        "Error inserting data, iteration %d, error %s",
                        i,
                        mysql_stmt_error(stmt));
    }

    test.reset_timeout();
    test.add_result(mysql_stmt_execute(stmt), "Execute failed: %s", mysql_stmt_error(stmt));

    sleep(5);
    test.check_current_operations(0);
    test.add_result(mysql_stmt_close(stmt), "Closing statement failed: %s", mysql_stmt_error(stmt));
}

int main(int argc, char* argv[])
{
    TestConnections test(argc, argv);
    test.repl->execute_query_all_nodes("set global max_allowed_packet=10000000");

    test.repl->connect();
    test.try_query(test.repl->nodes[0], "DROP TABLE IF EXISTS long_blob_table");
    test.try_query(test.repl->nodes[0], "CREATE TABLE long_blob_table(x INT, b LONGBLOB)");
    test.repl->sync_slaves();

    test.maxscale->connect();

    for (int i = 0; i < 2; i++)
    {
        run_test(test, 500000, 10);
    }

    test.maxscale->disconnect();

    test.repl->connect();
    test.try_query(test.repl->nodes[0], "DROP TABLE long_blob_table");
    test.repl->disconnect();

    return test.global_result;
}
