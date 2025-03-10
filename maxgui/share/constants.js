/*
 * Copyright (c) 2020 MariaDB Corporation Ab
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

export const PERSIST_TOKEN_OPT = 'persist=yes&max-age=604800'

// Do not alter the order of the keys as it's used for generating steps for the Config Wizard page
export const MXS_OBJ_TYPES = Object.freeze({
    SERVERS: 'servers',
    MONITORS: 'monitors',
    FILTERS: 'filters',
    SERVICES: 'services',
    LISTENERS: 'listeners',
})
