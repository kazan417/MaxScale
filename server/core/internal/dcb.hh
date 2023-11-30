/*
 * Copyright (c) 2018 MariaDB Corporation Ab
 * Copyright (c) 2023 MariaDB plc, Finnish Branch
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file and at www.mariadb.com/bsl11.
 *
 * Change Date: 2027-11-30
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2 or later of the General
 * Public License.
 */
#pragma once

#include <maxscale/ccdefs.hh>

#include <maxscale/dcb.hh>

/**
 * @brief Return the DCB currently being handled by the calling thread.
 *
 * @return A DCB, or NULL if the calling thread is not currently handling
 *         a DCB or if the calling thread is not a polling/worker thread.
 */
DCB* dcb_get_current();
