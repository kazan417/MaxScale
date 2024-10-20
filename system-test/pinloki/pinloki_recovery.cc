/*
 * Copyright (c) 2024 MariaDB plc
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file and at www.mariadb.com/bsl11.
 *
 * Change Date: 2027-09-09
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2 or later of the General
 * Public License.
 */

#include <maxtest/testconnections.hh>
#include "test_base.hh"

const char* const lorem_ipsum = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod"
                                " tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam,"
                                " quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo"
                                " consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse"
                                " cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat "
                                " non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.";

const std::string trx_file = "/var/lib/maxscale/binlogs/trx-crash.rc";
const std::string failed_write = "Failed to write "s + trx_file;
const std::string fail_mid_cmd = "echo DBG_PINLOKI_FAIL_MID_TRX=10 > "s + trx_file;
const std::string fail_after_commit = "echo DBG_PINLOKI_FAIL_AFTER_TRX_COMMIT=10 > "s + trx_file;
const std::string clear_cmd = "rm "s + trx_file;

class RecoveryTest : public TestCase
{
public:
    using TestCase::TestCase;

private:
    void pre() override
    {
        test.expect(master.query("CREATE TABLE test.t1(txt varchar(1000))"),
                    "CREATE failed: %s",
                    master.error());
    }

    /** All current tests "crash" by writing enough transactions */
    bool crash(const std::string& cmd)
    {
        test.maxscale->stop_and_check_stopped();
        test.expect(!test.maxscale->ssh_node(cmd.c_str(), true), "%s", failed_write.data());
        test.maxscale->start_and_check_started();
        maxscale = test.maxscale->rwsplit();
        maxscale.connect();
        slave.query("STOP SLAVE; START SLAVE");
        bool crashed = false;
        for (int i = 0; i < 100; ++i)   // the "crash" should happen after 4 iterations.
        {
            master.query("begin");
            insert();
            insert();
            insert();
            insert();
            master.query("commit");
            std::this_thread::sleep_for(100ms);     // allow time for maxscale to exit

            if (test.maxscale->get_n_running_processes() == 0)
            {
                crashed = true;
                break;
            }
        }

        return crashed;
    }

    /** Test that maxscale recovers after "crashing" while part
     *  of a transaction was written to disk.
     */
    void test_fail_mid_trx()
    {
        if (crash(fail_mid_cmd))
        {
            auto master_gtid = master.field("SELECT @@gtid_current_pos");
            test.maxscale->delete_log();
            test.expect(!test.maxscale->ssh_node(clear_cmd.c_str(), true), "%s", failed_write.data());
            test.maxscale->start_and_check_started();
            test.log_includes(MAKE_STR("incomplete transaction with gtid " << master_gtid).c_str());
            maxscale = test.maxscale->rwsplit();
            maxscale.connect();
            slave.query("STOP SLAVE; START SLAVE");
            sync(master, maxscale);
            sync(master, slave);
        }
        else
        {
            test.add_failure("%s", "Test test_fail_mid_trx did not crash as expected");
        }
    }

    void test_fail_after_commit()
    {
        if (crash(fail_after_commit))
        {
            auto master_gtid = master.field("SELECT @@gtid_current_pos");
            test.maxscale->delete_log();
            test.expect(!test.maxscale->ssh_node(clear_cmd.c_str(), true), "%s", failed_write.data());
            test.maxscale->start_and_check_started();
            test.log_includes(MAKE_STR("Recovered transaction with gtid " << master_gtid).c_str());
            maxscale = test.maxscale->rwsplit();
            maxscale.connect();
            slave.query("STOP SLAVE; START SLAVE");
            sync(master, maxscale);
            sync(master, slave);
        }
        else
        {
            test.add_failure("%s", "Test test_fail_after_commit did not crash as expected");
        }
    }

    void run() override
    {
        test_fail_mid_trx();
        test_fail_after_commit();
    }

    void post() override
    {
        test.expect(master.query("DROP TABLE test.t1"), "DROP failed: %s", master.error());
    }

    void insert()
    {
        test.expect(master.query(MAKE_STR("INSERT INTO test.t1 VALUES (\"" << lorem_ipsum << "\")")),
                    "INSERT failed: %s", master.error());
    }
};

int main(int argc, char** argv)
{
    TestConnections test(argc, argv);
    return RecoveryTest(test).result();
}
