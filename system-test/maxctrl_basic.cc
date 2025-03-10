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
 * Minimal MaxCtrl sanity check
 */

#include <maxtest/testconnections.hh>
#include <maxbase/format.hh>
#include <maxbase/string.hh>
#include <fstream>
#include <iostream>

namespace
{
void erase_old_files(TestConnections& test, std::initializer_list<const char*> files)
{
    test.maxscale->ssh_node_f(true,
                              "rm -f /var/lib/maxscale/maxscale.cnf.d/{%s}.cnf",
                              mxb::join(files, ",").c_str());
    test.maxscale->restart();
}

bool file_exists(TestConnections& test, const char* file)
{
    return test.maxscale->ssh_node_f(true, "test -f /var/lib/maxscale/maxscale.cnf.d/%s.cnf", file) == 0;
}
}

void test_reload_tls(TestConnections& test)
{
    test.maxscale->ssh_node_f(true, "rm /var/lib/maxscale/maxscale.cnf.d/*");

    auto& mxs = *test.maxscale;
    std::string key = mxs.cert_key_path();
    std::string cert = mxs.cert_path();
    int rc = test.maxscale->ssh_node_f(true, "sed -i "
                                             " -e '/maxscale/ a admin_ssl_key=%s'"
                                             " -e '/maxscale/ a admin_ssl_cert=%s'"
                                             " /etc/maxscale.cnf", key.c_str(), cert.c_str());
    test.expect(rc == 0, "Failed to enable encryption for the REST API");
    test.maxscale->restart();

    test.tprintf("TLS reload sanity check");
    test.expect(test.maxctrl("-s -n false list servers").rc == 0, "`list servers` should work");
    test.expect(test.maxctrl("list servers").rc != 0, "Command without --secure should fail");

    test.expect(test.maxctrl("-s -n false reload tls").rc == 0, "`reload tls` should work");
    test.expect(test.maxctrl("-s -n false list servers").rc == 0, "`list servers` should work");

    // Need to copy the client certificate and key to MaxScale node.
    const char* home = mxs.access_homedir();
    std::string client_cert_src = mxb::string_printf("%s/ssl-cert/client.crt", mxt::SOURCE_DIR);
    std::string client_key_src = mxb::string_printf("%s/ssl-cert/client.key", mxt::SOURCE_DIR);
    std::string client_cert_dst = mxb::string_printf("%s/certs/extra-key.pem", home);
    std::string client_key_dst = mxb::string_printf("%s/certs/extra-cert.pem", home);
    mxs.copy_to_node(client_cert_src, client_cert_dst);
    mxs.copy_to_node(client_key_src, client_key_dst);

    test.tprintf("MXS-4968: REST-API TLS certs can be reloaded but not modified");
    auto cmd = mxb::string_printf("-s -n false alter maxscale admin_ssl_key=%s admin_ssl_cert=%s",
                                  client_key_dst.c_str(), client_cert_dst.c_str());
    test.check_maxctrl(cmd);
    test.check_maxctrl("-s -n false list servers");

    // This tests that invalid paths are handled correctly. This covers a case where a debug assertion is hit
    // if an invalid path was given to ParamPath.
    test.maxctrl("-s -n false alter maxscale admin_ssl_key=/foo/bar.cert admin_ssl_cert=/foo/bar.key");

    cmd = mxb::string_printf("-s -n false alter maxscale admin_ssl_key=%s "
                             "admin_ssl_cert=%s", key.c_str(), cert.c_str());
    test.check_maxctrl(cmd);
    // Delete the copied files.
    mxs.vm_node().delete_from_node(client_cert_dst);
    mxs.vm_node().delete_from_node(client_key_dst);

    test.check_maxctrl("-s -n false list servers");

    if (test.ok())
    {
        test.tprintf("TLS reload stress test");
        std::vector<std::thread> threads;
        std::atomic<bool> running {true};
        std::atomic<int> total {0};

        for (int i = 0; i < 10; i++)
        {
            threads.emplace_back([&](){
                while (running)
                {
                    ++total;
                    auto res = test.maxscale->ssh_output("maxctrl -s -n false list servers", false);
                    test.expect(res.rc == 0, "`list servers` should not fail: %d, %s", res.rc,
                                res.output.c_str());
                }
            });
        }

        std::ofstream commands("commands.txt");

        for (int i = 0; i < 100; i++)
        {
            commands << "reload tls\n";
        }

        commands.flush();
        test.maxscale->copy_to_node("commands.txt", "/tmp/commands.txt");
        total.store(0);
        auto res = test.maxctrl("-s -n false < /tmp/commands.txt");
        auto total_during_reload = total.load();
        test.expect(res.rc == 0, "`reload tls` should work: %d, %s", res.rc, res.output.c_str());

        running = false;

        for (auto& t : threads)
        {
            t.join();
        }

        test.tprintf("Executed %d commands", total_during_reload);
    }

    test.maxscale->ssh_node_f(true, "sed -i  -e '/admin_ssl/ d' /etc/maxscale.cnf");
    test.maxscale->ssh_node_f(true, "rm /var/lib/maxscale/maxscale.cnf.d/maxscale.cnf");
    test.maxscale->restart();
}

void test_cert_chain(TestConnections& test)
{
    test.maxscale->ssh_node_f(true, "rm /var/lib/maxscale/maxscale.cnf.d/*");
    const char* home = test.maxscale->access_homedir();
    std::string key = test.maxscale->cert_key_path();
    std::string cert = test.maxscale->cert_path();
    std::string ca_cert = test.maxscale->ca_cert_path();

    int rc = test.maxscale->ssh_node_f(
        true, "cat %s %s > %s/certs/server-chain-cert.pem",
        cert.c_str(), ca_cert.c_str(), home);
    test.expect(rc == 0, "Failed to combine certificates into a chain");

    rc = test.maxscale->ssh_node_f(
        true,
        "sed -i "
        " -e '/maxscale/ a admin_ssl_key=%s'"
        " -e '/maxscale/ a admin_ssl_cert=%s/certs/server-chain-cert.pem'"
        " /etc/maxscale.cnf", key.c_str(), home);
    test.expect(rc == 0, "Failed to enable encryption for the REST API");
    test.maxscale->restart();

    test.expect(test.maxctrl("-s -n false list servers").rc == 0, "`list servers` should work");
    test.expect(test.maxctrl("list servers").rc != 0, "Command without --secure should fail");
    test.expect(test.maxctrl("-s -n false reload tls").rc == 0, "`reload tls` should work");
    test.expect(test.maxctrl("-s -n false list servers").rc == 0, "`list servers` should work after reload");

    test.maxscale->ssh_node_f(true, "sed -i  -e '/admin_ssl/ d' /etc/maxscale.cnf");
    test.maxscale->restart();
}

void mxs4944(TestConnections& test)
{
    // Make sure there's no cached configs present
    erase_old_files(test, {"RW-Split-Router", "maxscale"});

    std::string check_cmd = "test -f /var/lib/maxscale/maxscale.cnf.d/RW-Split-Router.cnf";

    test.check_maxctrl("alter service RW-Split-Router max_sescmd_history=51");
    test.expect(file_exists(test, "RW-Split-Router"),
                "Expected persisted RW-Split-Router config to exist");

    test.check_maxctrl("alter maxscale log_info=true");
    test.expect(file_exists(test, "maxscale"),
                "Expected persisted maxscale config to exist");

    test.check_maxctrl("alter service RW-Split-Router max_sescmd_history=50");
    test.expect(!file_exists(test, "RW-Split-Router"),
                "Expected persisted RW-Split-Router config to not exist");

    test.check_maxctrl("alter maxscale log_info=false");
    test.expect(!file_exists(test, "maxscale"),
                "Expected persisted maxscale config to not exist");

    // Check that a dynamic modification followed by a restart and revert of the modification will not remove
    // the persisted configuration
    test.check_maxctrl("alter service RW-Split-Router max_sescmd_history=51");
    test.expect(file_exists(test, "RW-Split-Router"),
                "Expected persisted RW-Split-Router config to exist");

    test.check_maxctrl("alter maxscale log_info=true");
    test.expect(file_exists(test, "maxscale"), "Expected persisted maxscale config to exist");

    test.maxscale->restart();

    test.check_maxctrl("alter service RW-Split-Router max_sescmd_history=50");
    test.expect(file_exists(test, "RW-Split-Router"),
                "Expected persisted RW-Split-Router config to still exist");

    test.check_maxctrl("alter maxscale log_info=false");
    test.expect(file_exists(test, "maxscale"),
                "Expected persisted maxscale config to exist");
}

int main(int argc, char** argv)
{
    TestConnections test(argc, argv);

    // Create one connection so that the sessions endpoint has some content
    auto c = test.maxscale->rwsplit();
    c.connect();

    int rc = test.maxscale->ssh_node_f(false, "maxctrl --help list servers");
    test.expect(rc == 0, "`--help list servers` should work");

    rc = test.maxscale->ssh_node_f(false, "maxctrl --tsv list servers|grep 'Master, Running'");
    test.expect(rc == 0, "`list servers` should return at least one row with: Master, Running");

    rc = test.maxscale->ssh_node_f(false, "maxctrl set server server2 maintenance");
    test.expect(rc == 0, "`set server` should work");

    rc = test.maxscale->ssh_node_f(false, "maxctrl --tsv list servers|grep 'Maintenance'");
    test.expect(rc == 0, "`list servers` should return at least one row with: Maintanance");

    rc = test.maxscale->ssh_node_f(false, "maxctrl clear server server2 maintenance");
    test.expect(rc == 0, "`clear server` should work");

    rc = test.maxscale->ssh_node_f(false, "maxctrl --tsv list servers|grep 'Maintenance'");
    test.expect(rc != 0, "`list servers` should have no rows with: Maintanance");

    test.tprintf("Execute all available commands");

    // Collecting the commands into a file and feeding that into maxctrl speeds up
    // the testing by quite a bit.
    std::ofstream commands("commands.txt");
    commands
        << "list servers\n"
        << "list services\n"
        << "list listeners RW-Split-Router\n"
        << "list monitors\n"
        << "list sessions\n"
        << "list filters\n"
        << "list modules\n"
        << "list threads\n"
        << "list users\n"
        << "list commands\n"
        << "show server server1\n"
        << "show servers\n"
        << "show service RW-Split-Router\n"
        << "show services\n"
        << "show monitor MySQL-Monitor\n"
        << "show monitors\n"
        << "show session 1\n"
        << "show sessions\n"
        << "show filter qla\n"
        << "show filters\n"
        << "show module readwritesplit\n"
        << "show modules\n"
        << "show maxscale\n"
        << "show thread 1\n"
        << "show threads\n"
        << "show logging\n"
        << "show commands mariadbmon\n"
        << "clear server server1 maintenance\n"
        << "enable log-priority info\n"
        << "disable log-priority info\n"
        << "create server server5 127.0.0.1 3306\n"
        << "create monitor mon1 mariadbmon user=skysql password=skysql\n"
        << "create service svc1 readwritesplit user=skysql password=skysql\n"
        << "create filter qla2 qlafilter filebase=/tmp/qla2.log\n"
        << "create listener svc1 listener1 9999\n"
        << "create user maxuser maxpwd\n"
        << "link service svc1 server5\n"
        << "link monitor mon1 server5\n"
        << "alter service-filters svc1 qla2\n"
        << "unlink service svc1 server5\n"
        << "unlink monitor mon1 server5\n"
        << "alter service-filters svc1"
        << "destroy server server5\n"
        << "destroy listener svc1 listener1\n"
        << "destroy monitor mon1\n"
        << "destroy filter qla2\n"
        << "destroy service svc1\n"
        << "destroy user maxuser\n"
        << "stop service RW-Split-Router\n"
        << "stop monitor MySQL-Monitor\n"
        << "stop maxscale\n"
        << "start service RW-Split-Router\n"
        << "start monitor MySQL-Monitor\n"
        << "start maxscale\n"
        << "alter server server1 port=3307\n"
        << "alter server server1 port=3306\n"
        << "alter monitor MySQL-Monitor auto_failover=true\n"
        << "alter service RW-Split-Router max_slave_connections=3\n"
        << "alter service RW-Split-Router slave_selection_criteria=adaptive_routing\n"
        << "alter logging ms_timestamp true\n"
        << "alter maxscale passive true\n"
        << "rotate logs\n"
        << "call command mariadbmon reset-replication MySQL-Monitor\n"
        << "api get servers\n"
        << "classify 'select 1'\n"
        << "debug stacktrace\n"
        << "debug stacktrace --raw\n"
        << "debug stacktrace --fold\n"
        << "debug stacktrace --duration=1\n"
        << "debug stacktrace --duration=1 --interval=100\n"
        << "create report test-report.txt\n";

    commands.flush();
    test.maxscale->copy_to_node("commands.txt", "/tmp/commands.txt");
    test.maxscale->ssh_node_f(false, "maxctrl --timeout 30s < /tmp/commands.txt");

    test.tprintf("MXS-3697: MaxCtrl fails with \"ENOENT: no such file or directory, stat '/~/.maxctrl.cnf'\" "
                 "when running commands from the root directory.");
    rc = test.maxscale->ssh_node_f(false, "cd / && maxctrl list servers");
    test.expect(rc == 0, "Failed to execute a command from the root directory");

    test.tprintf("MXS-4169: Listeners wrongly require ssl_ca_cert when created at runtime");
    test.check_maxctrl("create service my-test-service readconnroute user=maxskysql password=skysql");
    std::string key = test.maxscale->cert_key_path();
    std::string cert = test.maxscale->cert_path();
    test.check_maxctrl(
        "create listener my-test-service my-test-listener 6789 ssl=true "
        "ssl_key=" + key + " ssl_cert=" + cert);
    test.check_maxctrl("destroy listener my-test-listener");
    test.check_maxctrl("destroy service my-test-service");

    // Also checks that MaxCtrl works correctly when the REST API uses encryption.
    test.tprintf("MXS-4041: Reloading of REST API TLS certificates");
    test_reload_tls(test);

    test.tprintf("MXS-4442: TLS certificate chain in admin_ssl_cert");
    test_cert_chain(test);

    test.tprintf("MXS-4171: Runtime modifications to static parameters");

    auto res = test.maxctrl("alter service RW-Split-Router router=readwritesplit");
    test.expect(res.rc == 0, "Changing `router` to its current value should succeed: %s", res.output.c_str());

    res = test.maxctrl("alter service RW-Split-Router router=readconnroute");
    test.expect(res.rc != 0, "Changing `router` to a new value should fail.");

    res = test.maxctrl("alter listener RW-Split-Listener protocol=MySQLClient");
    test.expect(res.rc == 0, "Old alias for module name should compare equal: %s", res.output.c_str());

    res = test.maxctrl("alter listener RW-Split-Listener protocol=cdc");
    test.expect(res.rc != 0, "Changing listener protocol should fail.");

    test.tprintf("MXS-4944: Logically identical runtime configurations are not erased");
    mxs4944(test);

    // MXS-5126: Crash in cache filter with 'maxctrl show filters'
    c.connect();
    test.check_maxctrl("create filter CacheFilter cache storage=storage_inmemory");
    test.check_maxctrl("alter service-filters RW-Split-Router CacheFilter");

    for (int i = 0; i < 10; i++)
    {
        c.query("SELECT 1");
    }

    test.check_maxctrl("show filters");

    test.check_maxscale_alive();
    return test.global_result;
}
