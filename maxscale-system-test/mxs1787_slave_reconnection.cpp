/**
 * MXS-1787: Crash with PS: CALL p1((SELECT f1()), ?)
 *
 * This is a modified version of the original test that revealed secondary problems in
 * readwritesplit slave reconnection and execution of session commands.
 */

#include <maxtest/testconnections.h>

using namespace std;

struct Bind
{
    Bind()
    {
        bind.buffer = &data;
        bind.buffer_type = MYSQL_TYPE_LONG;
        bind.error = &err;
        bind.is_null = &is_null;
        bind.length = &length;
    }

    MYSQL_BIND    bind;
    char          err = 0;
    char          is_null = 0;
    char          is_unsigned = 0;
    uint32_t      data = 1234;
    unsigned long length = sizeof(data);
};

int main(int argc, char* argv[])
{
    TestConnections test(argc, argv);

    test.maxctrl("enable log-priority info");

    test.maxscales->connect();

    execute_query(test.maxscales->conn_rwsplit[0], "USE test");

    // Make sure the slaves don't have the procedure
    execute_query(test.maxscales->conn_rwsplit[0], "DROP TABLE IF EXISTS t1");
    execute_query(test.maxscales->conn_rwsplit[0], "DROP FUNCTION IF EXISTS f1");
    execute_query(test.maxscales->conn_rwsplit[0], "DROP PROCEDURE IF EXISTS p1");

    // Disable binary log and create the tables
    execute_query(test.maxscales->conn_rwsplit[0], "SET sql_log_bin = 0");
    execute_query(test.maxscales->conn_rwsplit[0], "CREATE OR REPLACE TABLE t1 AS SELECT 1 AS id");
    execute_query(test.maxscales->conn_rwsplit[0],
                  "CREATE OR REPLACE FUNCTION f1() RETURNS INT DETERMINISTIC BEGIN RETURN 1; END");
    execute_query(test.maxscales->conn_rwsplit[0],
                  "CREATE OR REPLACE PROCEDURE p1(IN i INT, IN j INT) BEGIN SELECT i + j; END");
    execute_query(test.maxscales->conn_rwsplit[0], "SET sql_log_bin = 1");

    test.maxscales->disconnect();

    for (int i = 0; i < 10; i++)
    {
        test.maxscales->connect();

        MYSQL_STMT* stmt = mysql_stmt_init(test.maxscales->conn_rwsplit[0]);
        std::string query = "CALL p1((SELECT f1()), ?)";
        Bind bind;

        test.set_timeout(30);

        test.expect(mysql_stmt_prepare(stmt, query.c_str(), query.size()) == 0,
                    "Prepared statement failure: %s",
                    mysql_stmt_error(stmt));
        test.expect(mysql_stmt_bind_param(stmt, &bind.bind) == 0,
                    "Bind failure: %s",
                    mysql_stmt_error(stmt));
        test.expect(mysql_stmt_execute(stmt) == 0,
                    "Execute failure: %s",
                    mysql_stmt_error(stmt));

        mysql_stmt_close(stmt);

        test.expect(mysql_query(test.maxscales->conn_rwsplit[0], "SELECT 1") == 0,
                    "Normal queries should work");
        test.maxscales->disconnect();
    }

    return test.global_result;
}
