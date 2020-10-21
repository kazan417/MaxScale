/*
 * Copyright (c) 2016 MariaDB Corporation Ab
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file and at www.mariadb.com/bsl11.
 *
 * Change Date: 2024-10-14
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2 or later of the General
 * Public License.
 */

#include <fstream>
#include <iostream>
#include <regex>
#include <maxtest/clustrix_nodes.hh>

int Clustrix_nodes::prepare_server(int m)
{
    int rv = 1;
    int ec;

    bool running = false;

    ec = ssh_node(m, "systemctl status clustrix", true);

    if (ec == 0)
    {
        printf("Clustrix running on node %d.\n", m);

        ec = ssh_node(m, "mysql -e 'SELECT @@server_id'", true);
        if (ec == 0)
        {
            running = true;
        }
        else
        {
            printf("Could not connect as root to Clustrix on node %d, restarting.\n", m);

            ec = ssh_node(m, "systemctl restart clustrix", true);

            if (ec == 0)
            {
                printf("Successfully restarted Clustrix on node %d.\n", m);
                running = true;
            }
            else
            {
                printf("Could not restart Clustrix on node %d.\n", m);
            }
        }
    }
    else
    {
        printf("Clustrix not running on node %d, starting.\n", m);

        ec = ssh_node(m, "systemctl start clustrix", true);

        if (ec == 0)
        {
            printf("Successfully started Clustrix on node %d.\n", m);
            running = true;
        }
        else
        {
            printf("Could not start Clustrix on node %d.\n", m);
        }
    }

    bool check_users = false;

    if (running)
    {
        int start = time(NULL);
        int now;

        do
        {
            ec = ssh_node(m, "mysql -e 'SELECT @@server_id'", true);
            now = time(NULL);

            if (ec != 0)
            {
                printf("Could not connect to Clustrix as root on node %d, "
                       "sleeping a while (totally at most ~1 minute) and retrying.\n", m);
                sleep(10);
            }
        }
        while (ec != 0 && now - start < 60);

        if (ec == 0)
        {
            printf("Could connect as root to Clustrix on node %d.\n", m);
            check_users = true;
        }
        else
        {
            printf("Could not connect as root to Clustrix on node %d within given timeframe.\n", m);
        }
    }

    if (check_users)
    {
        std::string command("mysql ");
        command += "-u ";
        command += this->user_name;
        command += " ";
        command += "-p";
        command += this->password;

        ec = ssh_node(m, command.c_str(), false);

        if (ec == 0)
        {
            printf("Can access Clustrix using user '%s'.\n", this->user_name);
            rv = 0;
        }
        else
        {
            printf("Cannot access Clustrix using user '%s', creating users.\n", this->user_name);
            // TODO: We need an return code here.
            create_users(m);
            rv = 0;
        }
    }

    return rv;
}


int Clustrix_nodes::start_replication()
{
    int rv = 1;



    std::string cluster_setup_sql = std::string("ALTER CLUSTER ADD '")
            + std::string(IP_private[1])
            + std::string("'");
    for (int i = 2; i < N; i++)
    {
        cluster_setup_sql += std::string(",'")
                + std::string(IP_private[i])
                + std::string("'");
    }
    connect();
    execute_query(nodes[0], "%s", cluster_setup_sql.c_str());
    close_connections();

    rv = 0;


    return rv;
}

std::string Clustrix_nodes::cnf_servers()
{
    std::string s;
    for (int i = 0; i < N; i++)
    {
        s += std::string("\\n[")
                + cnf_server_name
                + std::to_string(i + 1)
                + std::string("]\\ntype=server\\naddress=")
                + std::string(IP_private[i])
                + std::string("\\nport=")
                + std::to_string(port[i])
                + std::string("\\nprotocol=MySQLBackend\\n");
    }
    return s;
}

int Clustrix_nodes::check_replication()
{
    int res = 0;
    if (connect() == 0)
    {
        for (int i = 0; i < N; i++)
        {
            if (execute_query_count_rows(nodes[i], "select * from system.nodeinfo") != N)
            {
                res = 1;
            }
        }
    }
    else
    {
        res = 1;
    }

    close_connections(); // Some might have been created by connect().

    return res;
}

std::string Clustrix_nodes::block_command(int node) const
{
    std::string command = Mariadb_nodes::block_command(node);

    // Block health-check port as well.
    command += ";";
    command += "iptables -I INPUT -p tcp --dport 3581 -j REJECT";
    command += ";";
    command += "ip6tables -I INPUT -p tcp --dport 3581 -j REJECT";

    return command;
}

std::string Clustrix_nodes::unblock_command(int node) const
{
    std::string command = Mariadb_nodes::unblock_command(node);

    // Unblock health-check port as well.
    command += ";";
    command += "iptables -I INPUT -p tcp --dport 3581 -j ACCEPT";
    command += ";";
    command += "ip6tables -I INPUT -p tcp --dport 3581 -j ACCEPT";

    return command;
}
