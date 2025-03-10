/*
** Compile and run this standalone program in order to generate code that
** implements a function that will translate alphabetic identifiers into
** parser token codes.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

/*
** A header comment placed at the beginning of generated code.
*/
static const char zHdr[] = 
  "/***** This file contains automatically generated code ******\n"
  "**\n"
  "** The code in this file has been automatically generated by\n"
  "**\n"
  "**   sqlite/tool/mkkeywordhash.c\n"
  "**\n"
  "** The code in this file implements a function that determines whether\n"
  "** or not a given identifier is really an SQL keyword.  The same thing\n"
  "** might be implemented more directly using a hand-written hash table.\n"
  "** But by using this automatically generated code, the size of the code\n"
  "** is substantially reduced.  This is important for embedded applications\n"
  "** on platforms with limited memory.\n"
  "*/\n"
;

/*
** All the keywords of the SQL language are stored in a hash
** table composed of instances of the following structure.
*/
typedef struct Keyword Keyword;
struct Keyword {
  char *zName;         /* The keyword name */
  char *zTokenType;    /* Token value for this keyword */
  int mask;            /* Code this keyword if non-zero */
  int id;              /* Unique ID for this record */
  int hash;            /* Hash on the keyword */
  int offset;          /* Offset to start of name string */
  int len;             /* Length of this keyword, not counting final \000 */
  int prefix;          /* Number of characters in prefix */
  int longestSuffix;   /* Longest suffix that is a prefix on another word */
  int iNext;           /* Index in aKeywordTable[] of next with same hash */
  int substrId;        /* Id to another keyword this keyword is embedded in */
  int substrOffset;    /* Offset into substrId for start of this keyword */
  char zOrigName[20];  /* Original keyword name before processing */
};

/*
** Define masks used to determine which keywords are allowed
*/
#ifdef SQLITE_OMIT_ALTERTABLE
#  define ALTER      0
#else
#  define ALTER      0x00000001
#endif
#define ALWAYS       0x00000002
#ifdef SQLITE_OMIT_ANALYZE
#  define ANALYZE    0
#else
#  define ANALYZE    0x00000004
#endif
#ifdef SQLITE_OMIT_ATTACH
#  define ATTACH     0
#else
#  define ATTACH     0x00000008
#endif
#ifdef SQLITE_OMIT_AUTOINCREMENT
#  define AUTOINCR   0
#else
#  define AUTOINCR   0x00000010
#endif
#ifdef SQLITE_OMIT_CAST
#  define CAST       0
#else
#  define CAST       0x00000020
#endif
#ifdef SQLITE_OMIT_COMPOUND_SELECT
#  define COMPOUND   0
#else
#  define COMPOUND   0x00000040
#endif
#ifdef SQLITE_OMIT_CONFLICT_CLAUSE
#  define CONFLICT   0
#else
#  define CONFLICT   0x00000080
#endif
#ifdef SQLITE_OMIT_EXPLAIN
#  define EXPLAIN    0
#else
#  define EXPLAIN    0x00000100
#endif
#ifdef SQLITE_OMIT_FOREIGN_KEY
#  define FKEY       0
#else
#  define FKEY       0x00000200
#endif
#ifdef SQLITE_OMIT_PRAGMA
#  define PRAGMA     0
#else
#  define PRAGMA     0x00000400
#endif
#ifdef SQLITE_OMIT_REINDEX
#  define REINDEX    0
#else
#  define REINDEX    0x00000800
#endif
#ifdef SQLITE_OMIT_SUBQUERY
#  define SUBQUERY   0
#else
#  define SUBQUERY   0x00001000
#endif
#ifdef SQLITE_OMIT_TRIGGER
#  define TRIGGER    0
#else
#  define TRIGGER    0x00002000
#endif
#if defined(SQLITE_OMIT_AUTOVACUUM) && \
    (defined(SQLITE_OMIT_VACUUM) || defined(SQLITE_OMIT_ATTACH))
#  define VACUUM     0
#else
#  define VACUUM     0x00004000
#endif
#ifdef SQLITE_OMIT_VIEW
#  define VIEW       0
#else
#  define VIEW       0x00008000
#endif
#ifdef SQLITE_OMIT_VIRTUALTABLE
#  define VTAB       0
#else
#  define VTAB       0x00010000
#endif
#ifdef SQLITE_OMIT_AUTOVACUUM
#  define AUTOVACUUM 0
#else
#  define AUTOVACUUM 0x00020000
#endif
#ifdef SQLITE_OMIT_CTE
#  define CTE        0
#else
#  define CTE        0x00040000
#endif

/*
** These are the keywords
*/
static Keyword aKeywordTable[] = {
#ifndef MAXSCALE
  { "ABORT",            "TK_ABORT",        CONFLICT|TRIGGER       },
#endif
  { "ACTION",           "TK_ACTION",       FKEY                   },
  { "ADD",              "TK_ADD",          ALTER                  },
  { "AFTER",            "TK_AFTER",        TRIGGER                },
#ifdef MAXSCALE
  { "AGAINST",          "TK_AGAINST",      ALWAYS                 },
#endif
  { "ALL",              "TK_ALL",          ALWAYS                 },
  { "ALTER",            "TK_ALTER",        ALTER                  },
#ifdef MAXSCALE
  { "ALGORITHM",        "TK_ALGORITHM",    ANALYZE                },
#endif
  { "ANALYZE",          "TK_ANALYZE",      ANALYZE                },
  { "AND",              "TK_AND",          ALWAYS                 },
  { "AS",               "TK_AS",           ALWAYS                 },
  { "ASC",              "TK_ASC",          ALWAYS                 },
#ifdef MAXSCALE
  { "ATOMIC",           "TK_ATOMIC",       ALWAYS                 },
#endif
  { "ATTACH",           "TK_ATTACH",       ATTACH                 },
  { "AUTOINCREMENT",    "TK_AUTOINCR",     AUTOINCR               },
#ifdef MAXSCALE
  { "AUTO_INCREMENT",   "TK_AUTOINCR",     AUTOINCR               },
#endif
  { "BEFORE",           "TK_BEFORE",       TRIGGER                },
  { "BEGIN",            "TK_BEGIN",        ALWAYS                 },
  { "BETWEEN",          "TK_BETWEEN",      ALWAYS                 },
#ifdef MAXSCALE
  { "BINARY",           "TK_BINARY",       ALWAYS                 },
  { "BOTH",             "TK_TRIM_ARG",     ALWAYS                 },
#endif
  { "BY",               "TK_BY",           ALWAYS                 },
#ifdef MAXSCALE
  { "CALL",             "TK_CALL",         ALWAYS                 },
  { "CACHE",            "TK_CACHE",        ALWAYS                 },
#endif
  { "CASCADE",          "TK_CASCADE",      FKEY                   },
  { "CASE",             "TK_CASE",         ALWAYS                 },
  { "CAST",             "TK_CAST",         CAST                   },
#ifdef MAXSCALE
  { "CHARACTER",        "TK_CHARACTER",    ALWAYS                 },
  { "CHARSET",          "TK_CHARSET",      ALWAYS                 },
#endif
  { "CHECK",            "TK_CHECK",        ALWAYS                 },
#ifdef MAXSCALE
  { "CLOSE",            "TK_CLOSE",        ALWAYS                 },
#endif
  { "COLLATE",          "TK_COLLATE",      ALWAYS                 },
  { "COLUMN",           "TK_COLUMNKW",     ALTER                  },
#ifdef MAXSCALE
  { "COLUMNS",          "TK_COLUMNS",      ALWAYS                 },
  { "COMMENT",          "TK_COMMENT",      ALWAYS                 },
#endif
  { "COMMIT",           "TK_COMMIT",       ALWAYS                 },
  { "COMMITTED",        "TK_COMMITTED",    ALWAYS                 },
#ifndef MAXSCALE
  { "CONFLICT",         "TK_CONFLICT",     CONFLICT               },
#endif
#ifdef MAXSCALE
  { "CONNECTION",       "TK_CONNECTION",   ALWAYS                 },
  { "CONCURRENT",       "TK_CONCURRENT",   ALWAYS                 },
#endif
  { "CONSTRAINT",       "TK_CONSTRAINT",   ALWAYS                 },
  { "CREATE",           "TK_CREATE",       ALWAYS                 },
  { "CROSS",            "TK_JOIN_KW",      ALWAYS                 },
#ifdef MAXSCALE
  { "CURRENT",          "TK_CURRENT",      ALWAYS                 },
#endif
#ifndef MAXSCALE
  { "CURRENT_DATE",     "TK_CTIME_KW",     ALWAYS                 },
  { "CURRENT_TIME",     "TK_CTIME_KW",     ALWAYS                 },
  { "CURRENT_TIMESTAMP","TK_CTIME_KW",     ALWAYS                 },
#endif
#ifdef MAXSCALE
  { "DATA",             "TK_DATA",         ALWAYS                 },
  { "DATABASE",         "TK_DATABASE",     ALWAYS                 },
#else
  { "DATABASE",         "TK_DATABASE",     ATTACH                 },
#endif
#ifdef MAXSCALE
  { "DATABASES",        "TK_DATABASES_KW", ALWAYS                 },
  { "DEALLOCATE",       "TK_DEALLOCATE",   ALWAYS                 },
  { "DECLARE",          "TK_DECLARE",      ALWAYS                 },
#endif
  { "DEFAULT",          "TK_DEFAULT",      ALWAYS                 },
  { "DEFERRED",         "TK_DEFERRED",     ALWAYS                 },
  { "DEFERRABLE",       "TK_DEFERRABLE",   FKEY                   },
#ifdef MAXSCALE
  { "DELAYED",          "TK_DELAYED",      ALWAYS                 },
#endif
  { "DELETE",           "TK_DELETE",       ALWAYS                 },
  { "DESC",             "TK_DESC",         ALWAYS                 },
#ifdef MAXSCALE
  { "DESCRIBE",         "TK_EXPLAIN" ,     ALWAYS                 },
#endif
  { "DETACH",           "TK_DETACH",       ATTACH                 },
  { "DISTINCT",         "TK_DISTINCT",     ALWAYS                 },
#ifdef MAXSCALE
  { "DISTINCTROW",      "TK_DISTINCT",     ALWAYS                 },
  { "DIV",              "TK_DIV",          ALWAYS                 },
  { "DO",               "TK_DO",           ALWAYS                 },
#endif
  { "DROP",             "TK_DROP",         ALWAYS                 },
#ifdef MAXSCALE
  { "DUMPFILE",         "TK_DUMPFILE",     ALWAYS                 },
#endif
  { "END",              "TK_END",          ALWAYS                 },
  { "EACH",             "TK_EACH",         TRIGGER                },
#ifdef MAXSCALE
  { "ENABLE",           "TK_ENABLE",       ALWAYS                 },
  { "ENGINE",           "TK_ENGINE",       ALWAYS                 },
  { "ENUM",             "TK_ENUM",         ALWAYS                 },
#endif
  { "ELSE",             "TK_ELSE",         ALWAYS                 },
  { "ESCAPE",           "TK_ESCAPE",       ALWAYS                 },
  { "EXCEPT",           "TK_EXCEPT",       COMPOUND               },
  { "EXCLUSIVE",        "TK_EXCLUSIVE",    ALWAYS                 },
#ifdef MAXSCALE
  { "EXECUTE",          "TK_EXECUTE",      ALWAYS                 },
  { "EXCLUDE",          "TK_EXCLUDE",      ALWAYS                 },
  { "EXTENDED",         "TK_EXTENDED",     ALWAYS                 },
#endif
  { "EXISTS",           "TK_EXISTS",       ALWAYS                 },
  { "EXPLAIN",          "TK_EXPLAIN",      EXPLAIN                },
#ifndef MAXSCALE
  { "FAIL",             "TK_FAIL",         CONFLICT|TRIGGER       },
#endif
#ifdef MAXSCALE
  { "FIELDS",           "TK_FIELDS",       ALWAYS                 },
  { "FIRST",            "TK_FIRST",        ALWAYS                 },
  { "FLUSH",            "TK_FLUSH",        ALWAYS                 },
  { "FOLLOWING",        "TK_FOLLOWING",    ALWAYS                 },
#endif
#ifdef MAXSCALE
  { "FOR",              "TK_FOR",          ALWAYS                 },
#else
  { "FOR",              "TK_FOR",          TRIGGER                },
#endif
#ifdef MAXSCALE
  { "FORCE",            "TK_FORCE",        ALWAYS                 },
#endif
  { "FOREIGN",          "TK_FOREIGN",      FKEY                   },
#ifdef MAXSCALE
  { "FORMAT",           "TK_FORMAT",       ALWAYS                 },
#endif
  { "FROM",             "TK_FROM",         ALWAYS                 },
  { "FULL",             "TK_JOIN_KW",      ALWAYS                 },
#ifdef MAXSCALE
  { "FULLTEXT",         "TK_FULLTEXT",     ALWAYS                 },
  { "FUNCTION",         "TK_FUNCTION_KW",  ALWAYS                 },
#endif
  { "GLOB",             "TK_LIKE_KW",      ALWAYS                 },
#ifdef MAXSCALE
  { "GLOBAL",           "TK_GLOBAL",       ALWAYS                 },
  { "GRANT",            "TK_GRANT",        ALWAYS                 },
#endif
  { "GROUP",            "TK_GROUP",        ALWAYS                 },
#ifdef MAXSCALE
  { "HANDLER",          "TK_HANDLER",      ALWAYS                 },
  { "HARD",             "TK_HARD",         ALWAYS                 },
#endif
  { "HAVING",           "TK_HAVING",       ALWAYS                 },
#ifdef MAXSCALE
  { "HIGH_PRIORITY",    "TK_HIGH_PRIORITY",ALWAYS                 },
#endif
  { "IF",               "TK_IF",           ALWAYS                 },
#ifdef MAXSCALE
  { "IGNORE",           "TK_IGNORE",       ALWAYS                 },
#else
  { "IGNORE",           "TK_IGNORE",       CONFLICT|TRIGGER       },
#endif
  { "IMMEDIATE",        "TK_IMMEDIATE",    ALWAYS                 },
  { "IN",               "TK_IN",           ALWAYS                 },
  { "INDEX",            "TK_INDEX",        ALWAYS                 },
  { "INDEXED",          "TK_INDEXED",      ALWAYS                 },
#ifdef MAXSCALE
  { "INDEXES",          "TK_INDEXES",      ALWAYS                 },
  { "INFILE",           "TK_INFILE",       ALWAYS                 },
#endif
  { "INITIALLY",        "TK_INITIALLY",    FKEY                   },
  { "INNER",            "TK_JOIN_KW",      ALWAYS                 },
  { "INSERT",           "TK_INSERT",       ALWAYS                 },
  { "INSTEAD",          "TK_INSTEAD",      TRIGGER                },
  { "INTERSECT",        "TK_INTERSECT",    COMPOUND               },
#ifdef MAXSCALE
  { "INTERVAL",         "TK_INTERVAL",     ALWAYS                 },
#endif
  { "INTO",             "TK_INTO",         ALWAYS                 },
  { "IS",               "TK_IS",           ALWAYS                 },
  { "ISNULL",           "TK_ISNULL",       ALWAYS                 },
  { "ISOLATION",        "TK_ISOLATION",    ALWAYS                 },
  { "JOIN",             "TK_JOIN",         ALWAYS                 },
  { "KEY",              "TK_KEY",          ALWAYS                 },
#ifdef MAXSCALE
  { "KEYS",             "TK_KEYS",         ALWAYS                 },
  { "KILL",             "TK_KILL",         ALWAYS                 },
  { "LEADING",          "TK_TRIM_ARG"  ,   ALWAYS                 },
#endif
  { "LEFT",             "TK_JOIN_KW",      ALWAYS                 },
  { "LEVEL",            "TK_LEVEL",        ALWAYS                 },
  { "LIKE",             "TK_LIKE_KW",      ALWAYS                 },
  { "LIMIT",            "TK_LIMIT",        ALWAYS                 },
#ifdef MAXSCALE
  { "LOAD",             "TK_LOAD",         ALWAYS                 },
  { "LOCAL",            "TK_LOCAL",        ALWAYS                 },
  { "LOCK",             "TK_LOCK",         ALWAYS                 },
  { "LOW_PRIORITY",     "TK_LOW_PRIORITY", ALWAYS                 },
#endif
  { "MATCH",            "TK_MATCH",        ALWAYS                 },
#ifdef MAXSCALE
  { "MASTER",           "TK_MASTER",       ALWAYS                 },
  { "MERGE",            "TK_MERGE",        ALWAYS                 },
  { "MIGRATE",          "TK_MIGRATE",      ALWAYS                 },
  { "MOD",              "TK_MOD",          ALWAYS                 },
  { "MODE",             "TK_MODE",         ALWAYS                 },
  { "NAMES",            "TK_NAMES",        ALWAYS                 },
#endif
  { "NATURAL",          "TK_JOIN_KW",      ALWAYS                 },
#ifdef MAXSCALE
  { "NEXT",             "TK_NEXT",         ALWAYS                 },
#endif
  { "NO",               "TK_NO",           FKEY                   },
  { "NOT",              "TK_NOT",          ALWAYS                 },
  { "NOTNULL",          "TK_NOTNULL",      ALWAYS                 },
#ifdef MAXSCALE
  { "NOWAIT",           "TK_NOWAIT",       ALWAYS                 },
  { "NO_WRITE_TO_BINLOG","TK_NO_WRITE_TO_BINLOG",ALWAYS           },
#endif
  { "NULL",             "TK_NULL",         ALWAYS                 },
  { "OF",               "TK_OF",           ALWAYS                 },
  { "OFFSET",           "TK_OFFSET",       ALWAYS                 },
#ifdef MAXSCALE
  { "OLD_PASSWORD",     "TK_OLD_PASSWORD", ALWAYS                 },
#endif
  { "ON",               "TK_ON",           ALWAYS                 },
  { "ONE",              "TK_ONE",          ALWAYS                 },
  { "ONLY",             "TK_ONLY",         ALWAYS                 },
#ifdef MAXSCALE
  { "OPEN",             "TK_OPEN",         ALWAYS                 },
#endif
  { "OR",               "TK_OR",           ALWAYS                 },
  { "ORDER",            "TK_ORDER",        ALWAYS                 },
#ifdef MAXSCALE
  { "OTHERS",           "TK_OTHERS",       ALWAYS                 },
#endif
  { "OUTER",            "TK_JOIN_KW",      ALWAYS                 },
#ifdef MAXSCALE
  { "OUTFILE",          "TK_OUTFILE",      ALWAYS                 },
  { "OVER",             "TK_OVER",         ALWAYS                 },
#endif
#ifdef MAXSCALE
  { "PARTITION",        "TK_PARTITION",    ALWAYS                 },
  { "PARTITIONS",       "TK_PARTITIONS",   ALWAYS                 },
  { "PASSWORD",         "TK_PASSWORD",     ALWAYS                 },
  { "PERSISTENT",       "TK_PERSISTENT",   ALWAYS                 },
  { "PHASE",            "TK_PHASE",        ALWAYS                 },
#endif
#ifndef MAXSCALE
  { "PLAN",             "TK_PLAN",         EXPLAIN                },
#endif
  { "PRAGMA",           "TK_PRAGMA",       PRAGMA                 },
#ifdef MAXSCALE
  { "PRECEDING",        "TK_PRECEDING",    ALWAYS                 },
  { "PREPARE",          "TK_PREPARE",      ALWAYS                 },
  { "PREVIOUS",         "TK_PREVIOUS",     ALWAYS                 },
#endif
  { "PRIMARY",          "TK_PRIMARY",      ALWAYS                 },
#ifdef MAXSCALE
  { "PROCEDURE",        "TK_FUNCTION_KW",  ALWAYS                 },
#endif
#ifdef MAXSCALE
  { "OPTIMIZE",         "TK_OPTIMIZE",     ALWAYS                 },
  { "QUERY",            "TK_QUERY",        ALWAYS                 },
#else
  { "QUERY",            "TK_QUERY",        EXPLAIN                },
#endif
#ifdef MAXSCALE
  { "QUICK",            "TK_QUICK",        ALWAYS                 },
#endif
  { "RAISE",            "TK_RAISE",        TRIGGER                },
#ifdef MAXSCALE
  { "RANGE",            "TK_RANGE",        ALWAYS                 },
  { "READ",             "TK_READ",         ALWAYS                 },
#endif
  { "RECURSIVE",        "TK_RECURSIVE",    CTE                    },
  { "REFERENCES",       "TK_REFERENCES",   FKEY                   },
  { "REGEXP",           "TK_LIKE_KW",      ALWAYS                 },
  { "REINDEX",          "TK_REINDEX",      REINDEX                },
  { "RELEASE",          "TK_RELEASE",      ALWAYS                 },
  { "RENAME",           "TK_RENAME",       ALTER                  },
  { "REPLACE",          "TK_REPLACE",      CONFLICT               },
#ifdef MAXSCALE
  { "REPEATABLE",       "TK_REPEATABLE",   ALWAYS                 },
  { "RESET",            "TK_RESET",        ALWAYS                 },
#endif
  { "RESTRICT",         "TK_RESTRICT",     FKEY                   },
#ifdef MAXSCALE
  { "RESUME",           "TK_RESUME",       ALWAYS                 },
  { "REVOKE",           "TK_REVOKE",       ALWAYS                 },
#endif
  { "RIGHT",            "TK_JOIN_KW",      ALWAYS                 },
#ifdef MAXSCALE
  { "ROLE",             "TK_ROLE",         ALWAYS                 },
#endif
  { "ROLLBACK",         "TK_ROLLBACK",     ALWAYS                 },
#ifdef MAXSCALE
  { "ROLLUP",           "TK_ROLLUP",       ALWAYS                 },
#endif
  { "ROW",              "TK_ROW",          TRIGGER                },
#ifdef MAXSCALE
  { "ROWS",             "TK_ROWS",         ALWAYS                 },
#endif
  { "SAVEPOINT",        "TK_SAVEPOINT",    ALWAYS                 },
#ifdef MAXSCALE
  { "SCHEMAS",          "TK_DATABASES_KW", ALWAYS                 },
  { "SEQUENCE",         "TK_SEQUENCE",     ALWAYS                 },
#endif
  { "SELECT",           "TK_SELECT",       ALWAYS                 },
#ifdef MAXSCALE
  { "SESSION",          "TK_SESSION",      ALWAYS                 },
  { "SEPARATOR",        "TK_SEPARATOR",    ALWAYS                 },
  { "SHARE",            "TK_SHARE",        ALWAYS                 },
  { "SHOW",             "TK_SHOW",         ALWAYS                 },
  // Whether it is SIGNED or UNSIGNED lacks meaning.
  { "SIGNED",           "TK_UNSIGNED",     ALWAYS                 },
  { "SLAVE",            "TK_SLAVE",        ALWAYS                 },
  { "SPATIAL",          "TK_SPATIAL",      ALWAYS                 },
  { "SQL_BIG_RESULT",   "TK_SELECT_OPTIONS_KW", ALWAYS            },
  { "SQL_BUFFER_RESULT","TK_SELECT_OPTIONS_KW", ALWAYS            },
  { "SQL_CACHE",        "TK_SELECT_OPTIONS_KW", ALWAYS            },
  { "SQL_CALC_FOUND_ROWS","TK_SELECT_OPTIONS_KW", ALWAYS          },
  { "SQL_NO_CACHE",     "TK_SELECT_OPTIONS_KW", ALWAYS            },
  { "SQL_SMALL_RESULT", "TK_SELECT_OPTIONS_KW", ALWAYS            },
  { "SOFT",             "TK_SOFT",              ALWAYS            },
  { "SUSPEND",          "TK_SUSPEND",      ALWAYS                 },
#endif
  { "SET",              "TK_SET",          ALWAYS                 },
  { "SERIALIZABLE",     "TK_SERIALIZABLE", ALWAYS                 },
#ifdef MAXSCALE
  { "START",            "TK_START",        ALWAYS                 },
  { "STATEMENT",        "TK_STATEMENT",    ALWAYS                 },
  { "STATUS",           "TK_STATUS",       ALWAYS                 },
  { "STRAIGHT_JOIN",    "TK_STRAIGHT_JOIN",ALWAYS                 },
#endif
  { "TABLE",            "TK_TABLE",        ALWAYS                 },
#ifdef MAXSCALE
  { "TABLES",           "TK_TABLES",       ALWAYS                 },
#endif
  { "TEMP",             "TK_TEMP",         ALWAYS                 },
  { "TEMPORARY",        "TK_TEMP",         ALWAYS                 },
#ifdef MAXSCALE
  { "TEMPTABLE",        "TK_TEMPTABLE",    ANALYZE                },
#endif
  { "THEN",             "TK_THEN",         ALWAYS                 },
#ifdef MAXSCALE
  { "TIES",             "TK_TIES",         ANALYZE                },
#endif
  { "TO",               "TK_TO",           ALWAYS                 },
#ifdef MAXSCALE
  { "TRAILING",         "TK_TRIM_ARG",     ALWAYS                 },
#endif
  { "TRANSACTION",      "TK_TRANSACTION",  ALWAYS                 },
  { "TRIGGER",          "TK_TRIGGER",      TRIGGER                },
#ifdef MAXSCALE
  { "TRIM",             "TK_TRIM",         ALWAYS                 },
  { "TRUNCATE",         "TK_TRUNCATE",     ALWAYS                 },
  { "UNBOUNDED",        "TK_UNBOUNDED",    ALWAYS                 },
  { "UNCOMMITTED",      "TK_UNCOMMITTED",  ALWAYS                 },
#endif
  { "UNION",            "TK_UNION",        COMPOUND               },
  { "UNSIGNED",         "TK_UNSIGNED",     ALWAYS                 },
  { "UNIQUE",           "TK_UNIQUE",       ALWAYS                 },
#ifdef MAXSCALE
  { "UNLOCK",           "TK_UNLOCK",       ALWAYS                 },
#endif
  { "UPDATE",           "TK_UPDATE",       ALWAYS                 },
  { "USE",              "TK_USE",          ALWAYS                 },
  { "USER",             "TK_USER",         ALWAYS                 },
  { "USING",            "TK_USING",        ALWAYS                 },
  { "VACUUM",           "TK_VACUUM",       VACUUM                 },
#ifdef MAXSCALE
  { "VALUE",            "TK_VALUE",        ALWAYS                 },
#endif
  { "VALUES",           "TK_VALUES",       ALWAYS                 },
#ifdef MAXSCALE
  { "VARIABLES",        "TK_VARIABLES",    ALWAYS                 },
#endif
  { "VIEW",             "TK_VIEW",         VIEW                   },
  { "VIRTUAL",          "TK_VIRTUAL",      VTAB                   },
#ifdef MAXSCALE
  { "WAIT",             "TK_WAIT",         ALWAYS                 },
  { "WARNINGS",         "TK_WARNINGS",     ALWAYS                 },
  { "WINDOW",           "TK_WINDOW",       ALWAYS                 },
#endif
  { "WITH",             "TK_WITH",         CTE                    },
#ifndef MAXSCALE
  { "WITHOUT",          "TK_WITHOUT",      ALWAYS                 },
#endif
  { "WHEN",             "TK_WHEN",         ALWAYS                 },
  { "WHERE",            "TK_WHERE",        ALWAYS                 },
#ifdef MAXSCALE
  { "WORK",             "TK_WORK",         ALWAYS                 },
  { "WRITE",            "TK_WRITE",        ALWAYS                 },
  { "XA",               "TK_XA",           ALWAYS                 },
#endif
  { "ZEROFILL",         "TK_ZEROFILL",     ALWAYS                 },
};

/* Number of keywords */
static int nKeyword = (sizeof(aKeywordTable)/sizeof(aKeywordTable[0]));

/* Map all alphabetic characters into lower-case for hashing.  This is
** only valid for alphabetics.  In particular it does not work for '_'
** and so the hash cannot be on a keyword position that might be an '_'.
*/
#define charMap(X)   (0x20|(X))

/*
** Comparision function for two Keyword records
*/
static int keywordCompare1(const void *a, const void *b){
  const Keyword *pA = (Keyword*)a;
  const Keyword *pB = (Keyword*)b;
  int n = pA->len - pB->len;
  if( n==0 ){
    n = strcmp(pA->zName, pB->zName);
  }
  assert( n!=0 );
  return n;
}
static int keywordCompare2(const void *a, const void *b){
  const Keyword *pA = (Keyword*)a;
  const Keyword *pB = (Keyword*)b;
  int n = pB->longestSuffix - pA->longestSuffix;
  if( n==0 ){
    n = strcmp(pA->zName, pB->zName);
  }
  assert( n!=0 );
  return n;
}
static int keywordCompare3(const void *a, const void *b){
  const Keyword *pA = (Keyword*)a;
  const Keyword *pB = (Keyword*)b;
  int n = pA->offset - pB->offset;
  if( n==0 ) n = pB->id - pA->id;
  assert( n!=0 );
  return n;
}

/*
** Return a KeywordTable entry with the given id
*/
static Keyword *findById(int id){
  int i;
  for(i=0; i<nKeyword; i++){
    if( aKeywordTable[i].id==id ) break;
  }
  return &aKeywordTable[i];
}

/*
** This routine does the work.  The generated code is printed on standard
** output.
*/
int main(int argc, char **argv){
  int i, j, k, h;
  int bestSize, bestCount;
  int count;
  int nChar;
  int totalLen = 0;
  int aHash[1000];  /* 1000 is much bigger than nKeyword */
  char zText[2000];

  /* Remove entries from the list of keywords that have mask==0 */
  for(i=j=0; i<nKeyword; i++){
    if( aKeywordTable[i].mask==0 ) continue;
    if( j<i ){
      aKeywordTable[j] = aKeywordTable[i];
    }
    j++;
  }
  nKeyword = j;

  /* Fill in the lengths of strings and hashes for all entries. */
  for(i=0; i<nKeyword; i++){
    Keyword *p = &aKeywordTable[i];
    p->len = (int)strlen(p->zName);
    assert( p->len<sizeof(p->zOrigName) );
    memcpy(p->zOrigName, p->zName, p->len+1);
    totalLen += p->len;
    p->hash = (charMap(p->zName[0])*4) ^
              (charMap(p->zName[p->len-1])*3) ^ (p->len*1);
    p->id = i+1;
  }

  /* Sort the table from shortest to longest keyword */
  qsort(aKeywordTable, nKeyword, sizeof(aKeywordTable[0]), keywordCompare1);

  /* Look for short keywords embedded in longer keywords */
  for(i=nKeyword-2; i>=0; i--){
    Keyword *p = &aKeywordTable[i];
    for(j=nKeyword-1; j>i && p->substrId==0; j--){
      Keyword *pOther = &aKeywordTable[j];
      if( pOther->substrId ) continue;
      if( pOther->len<=p->len ) continue;
      for(k=0; k<=pOther->len-p->len; k++){
        if( memcmp(p->zName, &pOther->zName[k], p->len)==0 ){
          p->substrId = pOther->id;
          p->substrOffset = k;
          break;
        }
      }
    }
  }

  /* Compute the longestSuffix value for every word */
  for(i=0; i<nKeyword; i++){
    Keyword *p = &aKeywordTable[i];
    if( p->substrId ) continue;
    for(j=0; j<nKeyword; j++){
      Keyword *pOther;
      if( j==i ) continue;
      pOther = &aKeywordTable[j];
      if( pOther->substrId ) continue;
      for(k=p->longestSuffix+1; k<p->len && k<pOther->len; k++){
        if( memcmp(&p->zName[p->len-k], pOther->zName, k)==0 ){
          p->longestSuffix = k;
        }
      }
    }
  }

  /* Sort the table into reverse order by length */
  qsort(aKeywordTable, nKeyword, sizeof(aKeywordTable[0]), keywordCompare2);

  /* Fill in the offset for all entries */
  nChar = 0;
  for(i=0; i<nKeyword; i++){
    Keyword *p = &aKeywordTable[i];
    if( p->offset>0 || p->substrId ) continue;
    p->offset = nChar;
    nChar += p->len;
    for(k=p->len-1; k>=1; k--){
      for(j=i+1; j<nKeyword; j++){
        Keyword *pOther = &aKeywordTable[j];
        if( pOther->offset>0 || pOther->substrId ) continue;
        if( pOther->len<=k ) continue;
        if( memcmp(&p->zName[p->len-k], pOther->zName, k)==0 ){
          p = pOther;
          p->offset = nChar - k;
          nChar = p->offset + p->len;
          p->zName += k;
          p->len -= k;
          p->prefix = k;
          j = i;
          k = p->len;
        }
      }
    }
  }
  for(i=0; i<nKeyword; i++){
    Keyword *p = &aKeywordTable[i];
    if( p->substrId ){
      p->offset = findById(p->substrId)->offset + p->substrOffset;
    }
  }

  /* Sort the table by offset */
  qsort(aKeywordTable, nKeyword, sizeof(aKeywordTable[0]), keywordCompare3);

  /* Figure out how big to make the hash table in order to minimize the
  ** number of collisions */
  bestSize = nKeyword;
  bestCount = nKeyword*nKeyword;
  for(i=nKeyword/2; i<=2*nKeyword; i++){
    for(j=0; j<i; j++) aHash[j] = 0;
    for(j=0; j<nKeyword; j++){
      h = aKeywordTable[j].hash % i;
      aHash[h] *= 2;
      aHash[h]++;
    }
    for(j=count=0; j<i; j++) count += aHash[j];
    if( count<bestCount ){
      bestCount = count;
      bestSize = i;
    }
  }

  /* Compute the hash */
  for(i=0; i<bestSize; i++) aHash[i] = 0;
  for(i=0; i<nKeyword; i++){
    h = aKeywordTable[i].hash % bestSize;
    aKeywordTable[i].iNext = aHash[h];
    aHash[h] = i+1;
  }

  /* Begin generating code */
  printf("%s", zHdr);
  printf("/* Hash score: %d */\n", bestCount);
  printf("static int keywordCode(const char *z, int n, int *pType){\n");
  printf("  /* zText[] encodes %d bytes of keywords in %d bytes */\n",
          totalLen + nKeyword, nChar+1 );
  for(i=j=k=0; i<nKeyword; i++){
    Keyword *p = &aKeywordTable[i];
    if( p->substrId ) continue;
    memcpy(&zText[k], p->zName, p->len);
    k += p->len;
    if( j+p->len>70 ){
      printf("%*s */\n", 74-j, "");
      j = 0;
    }
    if( j==0 ){
      printf("  /*   ");
      j = 8;
    }
    printf("%s", p->zName);
    j += p->len;
  }
  if( j>0 ){
    printf("%*s */\n", 74-j, "");
  }
  printf("  static const char zText[%d] = {\n", nChar);
  zText[nChar] = 0;
  for(i=j=0; i<k; i++){
    if( j==0 ){
      printf("    ");
    }
    if( zText[i]==0 ){
      printf("0");
    }else{
      printf("'%c',", zText[i]);
    }
    j += 4;
    if( j>68 ){
      printf("\n");
      j = 0;
    }
  }
  if( j>0 ) printf("\n");
  printf("  };\n");

  printf("  static const unsigned char aHash[%d] = {\n", bestSize);
  for(i=j=0; i<bestSize; i++){
    if( j==0 ) printf("    ");
    printf(" %3d,", aHash[i]);
    j++;
    if( j>12 ){
      printf("\n");
      j = 0;
    }
  }
  printf("%s  };\n", j==0 ? "" : "\n");    

  printf("  static const unsigned char aNext[%d] = {\n", nKeyword);
  for(i=j=0; i<nKeyword; i++){
    if( j==0 ) printf("    ");
    printf(" %3d,", aKeywordTable[i].iNext);
    j++;
    if( j>12 ){
      printf("\n");
      j = 0;
    }
  }
  printf("%s  };\n", j==0 ? "" : "\n");    

  printf("  static const unsigned char aLen[%d] = {\n", nKeyword);
  for(i=j=0; i<nKeyword; i++){
    if( j==0 ) printf("    ");
    printf(" %3d,", aKeywordTable[i].len+aKeywordTable[i].prefix);
    j++;
    if( j>12 ){
      printf("\n");
      j = 0;
    }
  }
  printf("%s  };\n", j==0 ? "" : "\n");    

  printf("  static const unsigned short int aOffset[%d] = {\n", nKeyword);
  for(i=j=0; i<nKeyword; i++){
    if( j==0 ) printf("    ");
    printf(" %3d,", aKeywordTable[i].offset);
    j++;
    if( j>12 ){
      printf("\n");
      j = 0;
    }
  }
  printf("%s  };\n", j==0 ? "" : "\n");

  printf("  static const unsigned short aCode[%d] = {\n", nKeyword);
  for(i=j=0; i<nKeyword; i++){
    char *zToken = aKeywordTable[i].zTokenType;
    if( j==0 ) printf("    ");
    printf("%s,%*s", zToken, (int)(14-strlen(zToken)), "");
    j++;
    if( j>=5 ){
      printf("\n");
      j = 0;
    }
  }
  printf("%s  };\n", j==0 ? "" : "\n");

  printf("  int i, j;\n");
  printf("  const char *zKW;\n");
  printf("  if( n>=2 ){\n");
  printf("    i = ((charMap(z[0])*4) ^ (charMap(z[n-1])*3) ^ n) %% %d;\n",
          bestSize);
  printf("    for(i=((int)aHash[i])-1; i>=0; i=((int)aNext[i])-1){\n");
  printf("      if( aLen[i]!=n ) continue;\n");
  printf("      j = 0;\n");
  printf("      zKW = &zText[aOffset[i]];\n");
  printf("#ifdef SQLITE_ASCII\n");
  printf("      while( j<n && (z[j]&~0x20)==zKW[j] ){ j++; }\n");
  printf("#endif\n");
  printf("#ifdef SQLITE_EBCDIC\n");
  printf("      while( j<n && toupper(z[j])==zKW[j] ){ j++; }\n");
  printf("#endif\n");
  printf("      if( j<n ) continue;\n");
  for(i=0; i<nKeyword; i++){
    printf("      testcase( i==%d ); /* %s */\n",
           i, aKeywordTable[i].zOrigName);
  }
  printf("      *pType = aCode[i];\n");
  printf("      break;\n");
  printf("    }\n");
  printf("  }\n");
  printf("  return n;\n");
  printf("}\n");
  printf("int sqlite3KeywordCode(const unsigned char *z, int n){\n");
  printf("  int id = TK_ID;\n");
  printf("  keywordCode((char*)z, n, &id);\n");
  printf("  return id;\n");
  printf("}\n");
  printf("#define SQLITE_N_KEYWORD %d\n", nKeyword);

  return 0;
}
