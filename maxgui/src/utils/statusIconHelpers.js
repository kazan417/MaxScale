/*
 * Copyright (c) 2023 MariaDB plc
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
import { MXS_OBJ_TYPES } from '@share/constants'

const { SERVICES, SERVERS, MONITORS, LISTENERS } = MXS_OBJ_TYPES

export const ICON_SHEETS = {
    [MONITORS]: {
        frames: ['$vuetify.icons.mxs_stopped', '$vuetify.icons.mxs_good'],
        colorClasses: ['text-grayed-out', 'text-success'],
    },
    [SERVICES]: {
        frames: [
            '$vuetify.icons.mxs_critical',
            '$vuetify.icons.mxs_good',
            '$vuetify.icons.mxs_stopped',
        ],
        colorClasses: ['text-error', 'text-success', 'text-grayed-out'],
    },
    [LISTENERS]: {
        frames: [
            '$vuetify.icons.mxs_critical',
            '$vuetify.icons.mxs_good',
            '$vuetify.icons.mxs_stopped',
        ],
        colorClasses: ['text-error', 'text-success', 'text-grayed-out'],
    },
    [SERVERS]: {
        frames: [
            '$vuetify.icons.mxs_criticalServer',
            '$vuetify.icons.mxs_goodServer',
            '$vuetify.icons.mxs_maintenance',
        ],
        colorClasses: ['text-error', 'text-success', 'text-grayed-out'],
    },
    replication: {
        frames: [
            '$vuetify.icons.mxs_stopped',
            '$vuetify.icons.mxs_good',
            '$vuetify.icons.mxs_statusWarning',
        ],
        colorClasses: ['text-grayed-out', 'text-success', 'text-warning'],
    },
    log: {
        frames: [
            '$vuetify.icons.mxs_alertWarning',
            '$vuetify.icons.mxs_critical',
            '$vuetify.icons.mxs_statusInfo',
            '$vuetify.icons.mxs_reports',
            '$vuetify.icons.mxs_statusInfo',
            'mdi-bug',
        ],
        colorClasses: [
            'text-error',
            'text-error',
            'text-warning',
            'text-info',
            'text-info',
            'text-accent',
        ],
    },
    config_sync: {
        frames: ['mdi-sync-alert', 'mdi-cog-sync-outline', 'mdi-cog-sync-outline'],
        colorClasses: ['text-error', 'text-success', 'text-primary'],
    },
}

const LOG_PRIORITY_FRAME_IDX_MAP = {
    alert: 0,
    error: 1,
    warning: 2,
    notice: 3,
    info: 4,
    debug: 5,
}
/**
 * @returns {number} - frame index
 */
export default {
    [SERVICES]: state => {
        if (state.includes('Started')) return 1
        if (state.includes('Stopped')) return 2
        if (state.includes('Allocated') || state.includes('Failed')) return 0
        return -1
    },
    [SERVERS]: state => {
        if (state.includes('Maintenance')) return 2
        else if (state === 'Running' || state.includes('Down')) return 0
        else if (state.includes('Running')) return 1
        return -1
    },
    [MONITORS]: state => {
        if (state.includes('Running')) return 1
        if (state.includes('Stopped')) return 0
        return -1
    },
    [LISTENERS]: state => {
        if (state === 'Running') return 1
        else if (state === 'Stopped') return 2
        else if (state === 'Failed') return 0
        return -1
    },
    replication: state => {
        if (state === 'Stopped') return 0
        // healthy icon
        else if (state === 'Running') return 1
        return 2
    },
    log: priority => {
        const frameIdx = LOG_PRIORITY_FRAME_IDX_MAP[priority]
        if (frameIdx >= 0) return frameIdx
        return -1
    },
    config_sync: state => {
        if (state === 'No configuration changes') return 2
        if (state === 'OK') return 1
        return 0
    },
}
