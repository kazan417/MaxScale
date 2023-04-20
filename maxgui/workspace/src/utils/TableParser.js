/*
 * Copyright (c) 2023 MariaDB plc
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file and at www.mariadb.com/bsl11.
 *
 * Change Date: 2027-03-14
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2 or later of the General
 * Public License.
 */
import { t } from 'typy'
import tokenizer from '@wsSrc/utils/createTableTokenizer'

/**
 * This parser works when sql_quote_show_create on
 */
export default class TableParser {
    /**
     * @param {String} optsStr - table options string
     * @returns {Object} - table options
     */
    parseTableOpts(optsStr) {
        let match
        let opts = {}
        while ((match = tokenizer.tableOptions.exec(optsStr)) !== null) {
            const key = match[1]
            const value = match[2]
            opts[key.toLocaleLowerCase()] = value
        }
        return opts
    }
    parseColDef(def) {
        const [
            isMatched,
            name,
            dataType,
            dataTypeValue,
            un,
            zf,
            nn,
            charset,
            collate,
            generated_exp,
            generated,
            ai,
        ] = def.match(tokenizer.colDef) || []
        if (isMatched) {
            return {
                name,
                dataType,
                dataTypeValue,
                isUN: Boolean(un),
                isZF: Boolean(zf),
                isNN: Boolean(nn),
                charset,
                collate,
                generated_exp,
                generated,
                isAI: Boolean(ai),
                //TODO: Parse PK, UQ, DEFAULT exp, and COMMENT
            }
        }
        //TODO: Parse reference_definition
        return def
    }
    parseTableDefs(defsStr) {
        const defs = defsStr.split('\n')
        return defs.map(def => this.parseColDef(def.trim().replace(/,\s*$/, '')))
    }
    // Parse the result of SHOW CREATE TABLE
    parse(sql) {
        const match = sql.match(tokenizer.createTable)
        let tbl_name, table_options, table_definitions
        if (match) {
            tbl_name = t(match, '[1]').safeString.trim()
            table_definitions = this.parseTableDefs(t(match, '[2]').safeString.trim())
            table_options = this.parseTableOpts(t(match, '[3]').safeString)
        }
        return {
            tbl_name,
            table_options,
            table_definitions,
        }
    }
}
