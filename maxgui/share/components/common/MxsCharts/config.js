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
import Chart from 'chart.js/auto'
import { Tooltip } from 'chart.js'
import 'chartjs-adapter-date-fns'
import chartTrendline from 'chartjs-plugin-trendline'
import annotationPlugin from 'chartjs-plugin-annotation'

Chart.register(chartTrendline)
Chart.register(annotationPlugin)

Chart.defaults.font.family = "'azo-sans-web', adrianna, serif"
Chart.defaults.color = '#424F62'
Chart.defaults.font.size = 10
Chart.defaults.scale.grid.lineWidth = 0.6
Chart.defaults.scale.grid.color = 'rgba(234, 234, 234, 1)'
Chart.defaults.scale.grid.drawTicks = false
Chart.defaults.scale.grid.drawBorder = false

/**
 * @param elements {Chart.Element[]} the tooltip elements
 * @param eventPosition {Point} the position of the event in canvas coordinates
 * @returns {TooltipPosition} the tooltip position
 */
Tooltip.positioners.mxsCursor = (elements, eventPosition) => ({
    x: eventPosition.x,
    y: eventPosition.y,
})
