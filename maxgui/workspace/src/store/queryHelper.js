/*
 * Copyright (c) 2020 MariaDB Corporation Ab
 * Copyright (c) 2023 MariaDB plc, Finnish Branch
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file and at www.mariadb.com/bsl11.
 *
 * Change Date: 2027-05-22
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2 or later of the General
 * Public License.
 */
import {
    NODE_TYPES,
    NODE_GROUP_TYPES,
    NODE_GROUP_CHILD_TYPES,
    NODE_NAME_KEYS,
    SYS_SCHEMAS,
    CREATE_TBL_TOKENS as tokens,
    COL_ATTRS,
} from '@wsSrc/store/config'
import { lodash, to, dynamicColors, uuidv1 } from '@share/utils/helpers'
import { t as typy } from 'typy'
import { getObjectRows, quotingIdentifier } from '@wsSrc/utils/helpers'
import queries from '@wsSrc/api/queries'
import { RELATIONSHIP_OPTIONALITY } from '@wsSrc/components/worksheets/ErdWke/config'
import TableParser from '@wsSrc/utils/TableParser'
import { check_charset_support } from '@wsSrc/components/common/MxsDdlEditor/utils'

const parser = new TableParser()
/**
 * @public
 * @param {Object} node
 * @returns {String} database name
 */
const getSchemaName = node => node.parentNameData[NODE_TYPES.SCHEMA]

/**
 * @private
 * @param {Object} node
 * @returns {String} table name
 */
const getTblName = node => node.parentNameData[NODE_TYPES.TBL]

/**
 * @private
 * @returns {String} node key
 */
const genNodeKey = () => lodash.uniqueId('node_key_')

/**
 * @private
 * @param {Object} param.nodeGroup - A node group. (NODE_GROUP_TYPES)
 * @param {Boolean} [param.nodeAttrs.onlyName] - If it's true, it queries only the name of the node
 * @returns {String} SQL of the node group using for fetching its children nodes
 */
function getNodeGroupSQL({ nodeGroup, nodeAttrs = { onlyName: false } }) {
    const { TBL_G, VIEW_G, SP_G, FN_G, TRIGGER_G, COL_G, IDX_G } = NODE_GROUP_TYPES
    const schemaName = getSchemaName(nodeGroup)
    const childNodeType = NODE_GROUP_CHILD_TYPES[nodeGroup.type]

    let colKey = NODE_NAME_KEYS[childNodeType],
        tblName = '',
        cols = '',
        from = '',
        cond = ''
    switch (nodeGroup.type) {
        case TRIGGER_G:
        case COL_G:
        case IDX_G:
            tblName = getTblName(nodeGroup)
            break
    }
    switch (nodeGroup.type) {
        case TBL_G:
            cols = `${colKey}, CREATE_TIME, TABLE_TYPE, TABLE_ROWS, ENGINE`
            from = 'FROM information_schema.TABLES'
            cond = `WHERE TABLE_SCHEMA = '${schemaName}' AND TABLE_TYPE = 'BASE TABLE'`
            break
        case VIEW_G:
            cols = `${colKey}, CREATE_TIME, TABLE_TYPE, TABLE_ROWS, ENGINE`
            from = 'FROM information_schema.TABLES'
            cond = `WHERE TABLE_SCHEMA = '${schemaName}' AND TABLE_TYPE != 'BASE TABLE'`
            break
        case FN_G:
            cols = `${colKey}, DTD_IDENTIFIER, IS_DETERMINISTIC, SQL_DATA_ACCESS, CREATED`
            from = 'FROM information_schema.ROUTINES'
            cond = `WHERE ROUTINE_TYPE = 'FUNCTION' AND ROUTINE_SCHEMA = '${schemaName}'`
            break
        case SP_G:
            cols = `${colKey}, IS_DETERMINISTIC, SQL_DATA_ACCESS, CREATED`
            from = 'FROM information_schema.ROUTINES'
            cond = `WHERE ROUTINE_TYPE = 'PROCEDURE' AND ROUTINE_SCHEMA = '${schemaName}'`
            break
        case TRIGGER_G:
            cols = `${colKey}, CREATED, EVENT_MANIPULATION, ACTION_STATEMENT, ACTION_TIMING`
            from = 'FROM information_schema.TRIGGERS'
            cond = `WHERE TRIGGER_SCHEMA = '${schemaName}' AND EVENT_OBJECT_TABLE = '${tblName}'`
            break
        case COL_G:
            cols = `${colKey}, COLUMN_TYPE, COLUMN_KEY, PRIVILEGES`
            from = 'FROM information_schema.COLUMNS'
            cond = `WHERE TABLE_SCHEMA = '${schemaName}' AND TABLE_NAME = '${tblName}'`
            break
        case IDX_G:
            // eslint-disable-next-line vue/max-len
            cols = `${colKey}, COLUMN_NAME, NON_UNIQUE, SEQ_IN_INDEX, CARDINALITY, NULLABLE, INDEX_TYPE`
            from = 'FROM information_schema.STATISTICS'
            cond = `WHERE TABLE_SCHEMA = '${schemaName}' AND TABLE_NAME = '${tblName}'`
            break
    }
    return `SELECT ${nodeAttrs.onlyName ? colKey : cols} ${from} ${cond} ORDER BY ${colKey};`
}

/**
 * @private
 * @param {Object} param.nodeGroup - A node group. (NODE_GROUP_TYPES). Undefined if param.type === SCHEMA
 * @param {Object} param.data - data of node
 * @param {String} param.type - type of node to be generated
 * @param {String} param.name - name of the node
 * @param {Boolean} [param.nodeAttrs.isLeaf] -If it's true, child nodes are leaf nodes
 * @param {Boolean} [param.nodeAttrs.isEmptyChildren] - generate node with empty children. i.e. node.children = []
 * @returns {Object}  A node in schema sidebar
 */
function genNode({
    nodeGroup,
    data,
    type,
    name,
    nodeAttrs = { isLeaf: false, isEmptyChildren: false },
}) {
    const { SCHEMA, TBL, VIEW, SP, FN, TRIGGER, COL, IDX } = NODE_TYPES
    const { TBL_G, VIEW_G, SP_G, FN_G, COL_G, IDX_G, TRIGGER_G } = NODE_GROUP_TYPES
    const schemaName = type === SCHEMA ? name : getSchemaName(nodeGroup)
    let node = {
        id: type === SCHEMA ? name : `${nodeGroup.id}.${name}`,
        parentNameData:
            type === SCHEMA ? { [type]: name } : { ...nodeGroup.parentNameData, [type]: name },
        key: genNodeKey(),
        type,
        name,
        draggable: true,
        data,
        isSys: SYS_SCHEMAS.includes(schemaName.toLowerCase()),
    }
    /**
     * index name can be duplicated. e.g.composite indexes.
     * So this adds -node.key as a suffix to make sure id is unique.
     */
    if (type === IDX) node.id = `${nodeGroup.id}.${name}-${node.key}`

    node.level = Object.keys(node.parentNameData).length
    //TODO: Rename qualified_name to qualifiedName as others properties are using camelCase
    switch (type) {
        case TBL:
        case VIEW:
        case SP:
        case FN:
            node.qualified_name = `${quotingIdentifier(schemaName)}.${quotingIdentifier(node.name)}`
            break
        case TRIGGER:
        case COL:
        case IDX:
            node.qualified_name = `${quotingIdentifier(getTblName(nodeGroup))}.${quotingIdentifier(
                node.name
            )}`
            break
        case SCHEMA:
            node.qualified_name = quotingIdentifier(node.name)
            break
    }
    // Auto assign child node groups unless nodeAttrs is provided with values other than the default ones
    switch (type) {
        case VIEW:
        case TBL:
        case SCHEMA: {
            let childTypes = []
            if (type === VIEW || type === TBL) {
                // only TBL node has IDX_G and TRIGGER_G
                childTypes = type === VIEW ? [COL_G] : [COL_G, IDX_G, TRIGGER_G]
            } else childTypes = [TBL_G, VIEW_G, SP_G, FN_G]

            if (!nodeAttrs.isLeaf)
                node.children = childTypes.map(t => genNodeGroup({ parentNode: node, type: t }))
            if (nodeAttrs.isEmptyChildren) node.children = []
            break
        }
    }

    return node
}

/**
 * @public
 * @param {Object} param.parentNode - parent node of the node group being generated
 * @param {String} param.type - type in NODE_GROUP_TYPES
 * @returns
 */
function genNodeGroup({ parentNode, type }) {
    return {
        id: `${parentNode.id}.${type}`,
        parentNameData: { ...parentNode.parentNameData, [type]: type },
        key: genNodeKey(),
        type,
        name: type,
        draggable: false,
        level: parentNode.level + 1,
        children: [],
    }
}

/**
 * @public
 * @param {Array} param.treeData - Array of tree nodes to be updated
 * @param {Object} param.node - node with new value
 * @returns {Array} new tree data
 */
function deepReplaceNode({ treeData, node }) {
    const nodeId = typy(node, 'id').safeString
    return lodash.cloneDeepWith(treeData, value => {
        if (value && value.id === nodeId) return node
    })
}

/**
 * This function returns nodes data for schema sidebar and its completion list for the editor
 * @public
 * @param {Object} param.queryResult - query result data.
 * @param {Object} param.nodeGroup -  A node group. (NODE_GROUP_TYPES)
 * @param {Object} [param.nodeAttrs] - node attributes
 * @returns {Object} - return { nodes, completionItems}.
 */
function genNodeData({ queryResult = {}, nodeGroup = null, nodeAttrs }) {
    const type = nodeGroup ? NODE_GROUP_CHILD_TYPES[nodeGroup.type] : NODE_TYPES.SCHEMA
    const { fields = [], data = [] } = queryResult
    // fields return could be in lowercase if connection is via ODBC.
    const standardizedFields = fields.map(f => f.toUpperCase())
    const rows = getObjectRows({ columns: standardizedFields, rows: data })
    const nameKey = NODE_NAME_KEYS[type]
    return rows.reduce(
        (acc, row) => {
            acc.nodes.push(
                genNode({
                    nodeGroup,
                    data: row,
                    type,
                    name: row[nameKey],
                    nodeAttrs,
                })
            )
            acc.completionItems.push({
                label: row[nameKey],
                detail: type.toUpperCase(),
                insertText: row[nameKey],
                type,
            })
            return acc
        },
        { nodes: [], completionItems: [] }
    )
}

/**
 * @public
 * @param {String} param.connId - SQL connection ID
 * @param {Object} param.nodeGroup - A node group. (NODE_GROUP_TYPES)
 * @param {Object} [param.nodeAttrs] - node attributes
 * @param {Object} param.config - axios config
 * @returns {Promise<Array>} { nodes: [], completionItems: [] }
 */
async function getChildNodeData({ connId, nodeGroup, nodeAttrs, config }) {
    const sql = getNodeGroupSQL({ nodeAttrs, nodeGroup })
    const [e, res] = await to(queries.post({ id: connId, body: { sql }, config }))
    if (e) return { nodes: [], completionItems: [] }
    else {
        return genNodeData({
            queryResult: typy(res, 'data.data.attributes.results[0]').safeObject,
            nodeGroup,
            nodeAttrs,
        })
    }
}

/**
 * @public
 * @param {String} payload.connId - SQL connection ID
 * @param {Object} payload.nodeGroup - A node group. (NODE_GROUP_TYPES)
 * @param {Array} payload.data - Array of tree node to be updated
 * @param {Array} [payload.completionItems] - Array of completion items for editor
 * @param {Object} param.config - axios config
 * @returns {Promise<Array>} { data: {}, completionItems: [] }
 */
async function getNewTreeData({ connId, nodeGroup, data, completionItems = [], config }) {
    const { nodes, completionItems: childCmplItems } = await getChildNodeData({
        connId,
        nodeGroup,
        config,
    })
    return {
        data: deepReplaceNode({
            treeData: data,
            node: { ...nodeGroup, children: nodes },
        }),
        completionItems: [...completionItems, ...childCmplItems],
    }
}

/**
 * A node in db_tree_map has several attrs but some attrs are mainly of UX purpose.
 * This function returns a minimized version of node containing only necessary attrs
 * for identifying purpose and restoring expanded state of the schemas sidebar
 * @public
 * @param {Object} node - a node in db_tree_map
 * @returns {Object} minimized node
 */
function minimizeNode({ id, parentNameData, qualified_name, name, type, level }) {
    return {
        id,
        qualified_name,
        parentNameData,
        name,
        type,
        level,
    }
}

/**
 * @public
 * @param {Object} entity - ORM entity object
 * @param {String|Function} payload - either an entity id or a callback function that return Boolean (filter)
 * @returns {Array} returns entities
 */
function filterEntity(entity, payload) {
    if (typeof payload === 'function') return entity.all().filter(payload)
    if (entity.find(payload)) return [entity.find(payload)]
    return []
}
/**
 *
 * @param {Object} apiConnMap - connections from API mapped by id
 * @param {Array} persistentConns - current persistent connections
 * @returns {Object} - { alive_conns: [], orphaned_conn_ids: [] }
 * alive_conns: stores connections that exists in the response of a GET to /sql/
 * orphaned_conn_ids: When QueryEditor connection expires but its cloned connections (query tabs)
 * are still alive, those are orphaned connections
 */
function categorizeConns({ apiConnMap, persistentConns }) {
    let alive_conns = [],
        orphaned_conn_ids = []

    persistentConns.forEach(conn => {
        const connId = conn.id
        if (apiConnMap[connId]) {
            // if this has value, it is a cloned connection from the QueryEditor connection
            const queryEditorConnId = typy(conn, 'clone_of_conn_id').safeString
            if (queryEditorConnId && !apiConnMap[queryEditorConnId]) orphaned_conn_ids.push(conn.id)
            else
                alive_conns.push({
                    ...conn,
                    // update attributes
                    attributes: apiConnMap[connId].attributes,
                })
        }
    })

    return { alive_conns, orphaned_conn_ids }
}
/**
 * @param {String} param.driver
 * @param {String} param.server
 * @param {String} param.port
 * @param {String} param.user
 * @param {String} param.password
 * @param {String} [param.db] - required if driver is PostgreSQL
 * @returns {String}  ODBC connection_string
 */
function genConnStr({ driver, server, port, user, password, db }) {
    let connStr = `DRIVER=${driver};SERVER=${server};PORT=${port};UID=${user};PWD={${password}}`
    if (db) connStr += `;DATABASE=${db}`
    return connStr
}
/**
 * @param {String} connection_string
 * @returns {String} Database name
 */
function getDatabase(connection_string) {
    const matches = connection_string.match(/(database=)\w+/gi) || ['']
    const matched = matches[0]
    return matched.replace(/(database=)+/gi, '')
}

/**
 * @param {string} param.schema - schema name
 * @param {object} param.parsedTable - parsed ddl of a table
 *  @param {string} param.highlightColor - highlight color
 */
function genErdNode({ schema, parsedTable, highlightColor }) {
    return {
        id: `node_${uuidv1()}`,
        qualifiedName: `${schema}.${parsedTable.name}`, // using for generate links
        schema,
        data: parsedTable,
        styles: { highlightColor },
        x: 0,
        y: 0,
        vx: 0,
        vy: 0,
    }
}

const getNodeHighlightColor = node => typy(node, 'styles.highlightColor').safeString

const getColDefData = ({ node, colName }) =>
    node.data.definitions.cols.find(col => col.name === colName)

const getOptionality = colData =>
    colData.is_nn ? RELATIONSHIP_OPTIONALITY.MANDATORY : RELATIONSHIP_OPTIONALITY.OPTIONAL

const isIndex = ({ indexDefs, indexCols }) =>
    indexDefs.some(def => lodash.isEqual(def.index_cols, indexCols))

function isUniqueCol({ node, indexCols }) {
    const keys = node.data.definitions.keys
    const pks = keys[tokens.primaryKey] || []
    const uniqueKeys = keys[tokens.uniqueKey] || []
    if (!pks.length && !uniqueKeys.length) return false
    return isIndex({ indexDefs: pks, indexCols }) || isIndex({ indexDefs: uniqueKeys, indexCols })
}
function getCardinality(params) {
    return isUniqueCol(params) ? '1' : 'N'
}

/**
 * @param {object} param.srcNode - referencing table
 * @param {object} param.targetNode - referenced table
 * @param {object} param.fk - parsed fk data
 * @param {string} param.indexColName - source column name
 * @param {string} param.referencedIndexColName - target column name
 * @param {boolean} param.isPartOfCompositeKey - is a part of composite FK
 * @param {string} param.srcCardinality - either 1 or N
 * @param {string} param.targetCardinality - either 1 or N
 */
function genErdLink({
    srcNode,
    targetNode,
    fk,
    indexColName,
    referencedIndexColName,
    isPartOfCompositeKey,
    srcCardinality,
    targetCardinality,
}) {
    const { name, on_delete, on_update } = fk

    const colData = getColDefData({ node: srcNode, colName: indexColName })
    const referencedColData = getColDefData({ node: targetNode, colName: referencedIndexColName })

    const srcOptionality = getOptionality(colData)
    const targetOptionality = getOptionality(referencedColData)
    const type = `${srcOptionality}..${srcCardinality}:${targetOptionality}..${targetCardinality}`

    let link = {
        id: `link_${uuidv1()}`,
        source: srcNode.id,
        target: targetNode.id,
        relationshipData: {
            type,
            name,
            on_delete,
            on_update,
            source_attr: indexColName,
            target_attr: referencedIndexColName,
        },
    }
    if (isPartOfCompositeKey) link.isPartOfCompositeKey = isPartOfCompositeKey
    return link
}

function handleGenErdLink({ srcNode, fk, nodes }) {
    const { index_cols, referenced_schema_name, referenced_table_name, referenced_index_cols } = fk
    let links = []

    const target = `${referenced_schema_name}.${referenced_table_name}`
    const targetNode = nodes.find(n => n.qualifiedName === target)
    const invisibleHighlightColor = getNodeHighlightColor(targetNode)
    if (targetNode) {
        const srcCardinality = getCardinality({ node: srcNode, indexCols: index_cols })
        const targetCardinality = getCardinality({
            node: targetNode,
            indexCols: referenced_index_cols,
        })
        for (const [i, item] of index_cols.entries()) {
            const indexColName = item.name
            const referencedIndexColName = referenced_index_cols[i].name
            let linkObj = genErdLink({
                srcNode,
                targetNode,
                fk,
                indexColName,
                referencedIndexColName,
                isPartOfCompositeKey: i >= 1,
                srcCardinality,
                targetCardinality,
            })
            linkObj.styles = { invisibleHighlightColor }
            links.push(linkObj)
        }
    }
    return links
}
/**
 * @param {Object} parsedDdl - parsed ddl map of schemas
 */
function genErdData(parsedDdl) {
    let nodes = [],
        links = []

    Object.keys(parsedDdl).forEach(schema => {
        nodes = [
            ...nodes,
            ...parsedDdl[schema].map((tbl, i) =>
                genErdNode({ schema, parsedTable: tbl, highlightColor: dynamicColors(i) })
            ),
        ]
    })
    nodes.forEach(node => {
        const fks = typy(node.data.definitions.keys[tokens.foreignKey]).safeArray
        fks.forEach(fk => {
            links = [
                ...links,
                ...handleGenErdLink({
                    srcNode: node,
                    fk: { ...fk, referenced_schema_name: fk.referenced_schema_name || node.schema },
                    nodes,
                }),
            ]
        })
    })
    return { nodes, links }
}

/**
 * @param {string} param.connId - id of connection
 * @param {string[]} param.tableNodes - tables to be queried and parsed
 * @param {Object} param.config - axios config
 * @returns {Object} key is the schema name, value is the parsed data of tables
 */
async function queryAndParseDDL({ connId, tableNodes, config }) {
    let errors = []
    const [setVariableErr] = await to(
        queries.post({ id: connId, body: { sql: 'SET SESSION sql_quote_show_create = 1' }, config })
    )
    const [getScriptErr, res] = await to(
        queries.post({
            id: connId,
            body: {
                sql: tableNodes.map(node => `SHOW CREATE TABLE ${node.qualified_name};`).join('\n'),
                max_rows: 0,
            },
            config,
        })
    )
    if (setVariableErr) errors.push(setVariableErr)
    if (getScriptErr) errors.push(getScriptErr)
    const parsedDdl = typy(res, 'data.data.attributes.results').safeArray.reduce((acc, item, i) => {
        const schema = tableNodes[i].parentNameData[NODE_TYPES.SCHEMA]
        if (!acc[schema]) acc[schema] = []
        const parsed = parser.parse(typy(item, 'data[0][1]').safeString)
        acc[schema].push(parsed)
        return acc
    }, {})
    return [errors, parsedDdl]
}

/**
 * @param {array} keys - parsed keys from DDL of a table
 * @param {string} colName - column name to be looked up
 * @returns {string} type of the key
 */
function findKeyTypeByColName({ keys, colName }) {
    const keyTypes = [tokens.primaryKey, tokens.uniqueKey, tokens.key]
    return keyTypes.find(type =>
        typy(keys, `[${type}]`).safeArray.some(key =>
            key.index_cols.some(item => item.name === colName)
        )
    )
}

/**
 * @param {array} keys - parsed keys from DDL of a table
 * @param {string} keyType - type of the key
 * @param {string} colName - column name to be looked up
 * @returns {string} constraint name of the key
 */
function getIdxNameByColName({ keys, keyType, colName }) {
    for (const key of typy(keys, `[${keyType}]`).safeArray) {
        if (key.index_cols.some(item => item.name === colName)) return key.name
    }
}

/**
 * Transform the parsed output of TableParser into a structure
 * that is used by mxs-ddl-editor
 * @param {String} param.schema
 * @param {Object} param.parsedTable - output of TableParser
 * @param {Object} param.charsetCollationMap - collations mapped by charset
 * @returns {Object}
 */
function tableParserTransformer({ schema, parsedTable, charsetCollationMap }) {
    const {
        definitions: { cols, keys },
    } = parsedTable
    const charset = parsedTable.options.charset
    const collation =
        typy(parsedTable, 'options.collation').safeString ||
        typy(charsetCollationMap, `[${charset}].defCollation`).safeString

    const colsTransformed = cols.map(col => {
        let type = col.data_type
        if (col.data_type_size) type += `(${col.data_type_size})`
        const keyType = findKeyTypeByColName({ keys, colName: col.name })
        let uq = ''
        if (keyType === tokens.uniqueKey) {
            uq = getIdxNameByColName({
                keys,
                keyType,
                colName: col.name,
            })
        }
        const {
            ID,
            NAME,
            TYPE,
            PK,
            NN,
            UN,
            UQ,
            ZF,
            AI,
            GENERATED,
            DEF_EXP,
            CHARSET,
            COLLATE,
            COMMENT,
        } = COL_ATTRS
        //TODO: refactor UI input components to accept boolean
        return {
            [ID]: uuidv1(),
            [NAME]: col.name,
            [TYPE]: type,
            [PK]: keyType === tokens.primaryKey ? 'YES' : 'NO',
            [NN]: col.is_nn ? tokens.nn : tokens.null,
            [UN]: col.is_un ? tokens.un : '',
            [UQ]: uq,
            [ZF]: col.is_zf ? tokens.zf : '',
            [AI]: col.is_ai ? tokens.ai : '',
            [GENERATED]: col.generated_type ? col.generated_type : '(none)',
            [DEF_EXP]: col.generated_exp ? col.generated_exp : typy(col.default_exp).safeString,
            [CHARSET]: check_charset_support(col.data_type) ? col.charset || charset : '',
            [COLLATE]: check_charset_support(col.data_type) ? col.collate || collation : '',
            [COMMENT]: typy(col.comment).safeString,
        }
    })

    return {
        options: {
            schema,
            ...parsedTable.options,
            charset,
            collation,
            name: parsedTable.name,
        },
        definitions: {
            data: colsTransformed.map(col => [...Object.values(col)]),
        },
    }
}

export default {
    getSchemaName,
    getTblName,
    genNodeGroup,
    genNodeData,
    getChildNodeData,
    getNewTreeData,
    deepReplaceNode,
    minimizeNode,
    filterEntity,
    categorizeConns,
    genConnStr,
    getDatabase,
    genErdData,
    queryAndParseDDL,
    findKeyTypeByColName,
    getIdxNameByColName,
    tableParserTransformer,
}
