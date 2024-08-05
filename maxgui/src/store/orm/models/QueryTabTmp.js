/*
 * Copyright (c) 2023 MariaDB plc
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file and at www.mariadb.com/bsl11.
 *
 * Change Date: 2029-02-28
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2 or later of the General
 * Public License.
 */
import Extender from '@/store/orm/Extender'
import { TMP_ORM_ENTITY_MAP } from '@/constants/workspace'

export default class QueryTabTmp extends Extender {
  static entity = TMP_ORM_ENTITY_MAP.QUERY_TABS_TMP

  /**
   * @returns {Object} - return fields that are not key, relational fields
   */
  static getNonKeyFields() {
    return {
      // fields for QueryResult
      has_kill_flag: this.boolean(false),
      previewing_node: this.attr({}),
      // fields for AlterEditor
      alter_editor_staging_data: this.attr({}),
      // fields for auto completion feature
      schema_identifier_names_completion_items: this.attr([]),
      /**
       * Below object fields have these properties
       * @property {number} start_time (ms)
       * @property {number} end_time (ms)
       * @property {boolean} is_loading
       * @property {object} data
       */
      prvw_data: this.attr({}),
      prvw_data_details: this.attr({}),
      query_results: this.attr({}),
      process_list: this.attr({}),
      insight_data: this.attr({}),
      ddl_result: this.attr({}),
    }
  }

  static fields() {
    return {
      id: this.attr(null), // use QueryTab Id as PK for this table
      ...this.getNonKeyFields(),
    }
  }
}
