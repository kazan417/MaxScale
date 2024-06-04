#!/bin/bash
#
# Copyright (c) 2016 MariaDB Corporation Ab
# Copyright (c) 2023 MariaDB plc, Finnish Branch
#
# Use of this software is governed by the Business Source License included
# in the LICENSE.TXT file and at www.mariadb.com/bsl11.
#
# Change Date: 2026-09-21
#
# On the date above, in accordance with the Business Source License, use
# of this software will be governed by version 2 or later of the General
# Public License.
#

for ((i=0 ; i<100 ; i++)) ;
do
	echo "Iteration $i"
	mariadb --ssl-verify-server-cert=0 --host=${maxscale_000_network} -P 4006 -u $node_user -p$node_password --verbose --force --unbuffered=true --disable-reconnect $ssl_options > /dev/null < $src_dir/session_hang/setmix.sql >& /dev/null
done

