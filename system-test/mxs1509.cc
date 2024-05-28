/*
 * Copyright (c) 2016 MariaDB Corporation Ab
 * Copyright (c) 2023 MariaDB plc, Finnish Branch
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file and at www.mariadb.com/bsl11.
 *
 * Change Date: 2024-06-03
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2 or later of the General
 * Public License.
 */

/**
 * MXS-1509: Show correct server state for multisource replication
 *
 * https://jira.mariadb.org/browse/MXS-1509
 */

#include <maxtest/testconnections.hh>
#include <sstream>

void change_master(TestConnections& test, int slave, int master, const char* name = NULL)
{
    std::string source;

    if (name)
    {
        source = "'";
        source += name;
        source += "'";
    }

    execute_query(test.repl->nodes[slave],
                  "STOP ALL SLAVES;"
                  "SET GLOBAL gtid_slave_pos='0-1-0';"
                  "CHANGE MASTER %s TO master_host='%s', master_port=3306, master_user='%s', master_password='%s', master_use_gtid=slave_pos;"
                  "START ALL SLAVES",
                  source.c_str(),
                  test.repl->ip_private(master),
                  test.repl->user_name().c_str(),
                  test.repl->password().c_str());
}

void check_status(TestConnections& test, const StringSet& expected_master, const StringSet& expected_slave)
{
    StringSet master = test.get_server_status("server1");
    StringSet slave = test.get_server_status("server2");
    test.add_result(master != expected_master,
                    "Master status is not what was expected: %s",
                    dump_status(master, expected_master).c_str());
    test.add_result(slave != expected_slave,
                    "Slave status is not what was expected: %s",
                    dump_status(slave, expected_slave).c_str());
}

void test_with_external_masters(TestConnections& test,
                                const StringSet& expected_master,
                                const StringSet& expected_slave)
{
    test.repl->connect();
    test.tprintf("Server sanity check");
    test.maxscale->wait_for_monitor();
    StringSet basic_master = {"Master", "Running"};
    StringSet basic_slave = {"Slave", "Running"};
    check_status(test, basic_master, basic_slave);

    test.tprintf("Stop replication on nodes three and four");
    execute_query(test.repl->nodes[2], "STOP ALL SLAVES; RESET SLAVE ALL;");
    execute_query(test.repl->nodes[3], "STOP ALL SLAVES; RESET SLAVE ALL;");

    test.tprintf("Point the master to an external server");
    change_master(test, 1, 0);
    change_master(test, 0, 2);
    test.maxscale->wait_for_monitor();
    check_status(test, expected_master, basic_slave);

    test.tprintf("Resetting the slave on master should have no effect");
    execute_query(test.repl->nodes[0], "STOP ALL SLAVES; RESET SLAVE ALL;");
    test.maxscale->wait_for_monitor();
    check_status(test, basic_master, basic_slave);

    test.tprintf("Configure multi-source replication, check that master status is as expected");
    change_master(test, 0, 2, "extra-slave");
    change_master(test, 1, 2, "extra-slave");
    test.maxscale->wait_for_monitor();
    check_status(test, expected_master, expected_slave);

    test.tprintf("Stopping multi-source replication on slave should "
                 "remove the Slave of External Server status");
    execute_query(test.repl->nodes[1], "STOP SLAVE 'extra-slave'; RESET SLAVE 'extra-slave';");
    test.maxscale->wait_for_monitor();
    check_status(test, expected_master, basic_slave);

    test.tprintf("Doing the same on the master should have no effect");
    execute_query(test.repl->nodes[0], "STOP ALL SLAVES; RESET SLAVE ALL;");
    test.maxscale->wait_for_monitor();
    check_status(test, basic_master, basic_slave);

    test.tprintf("Cleanup");
    test.repl->execute_query_all_nodes("STOP ALL SLAVES; RESET SLAVE ALL;");
    test.repl->fix_replication();
}

void test_main(TestConnections& test)
{
    test_with_external_masters(test, {"Master", "Running"}, {"Slave", "Running"});
    test.check_maxctrl("alter monitor MySQL-Monitor ignore_external_masters=false");
    test_with_external_masters(test,
                               {"Master", "Running", "Slave of External Server"},
                               {"Slave", "Running", "Slave of External Server"});
}

int main(int argc, char** argv)
{
    return TestConnections().run_test(argc, argv, test_main);
}
