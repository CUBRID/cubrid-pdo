/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2010 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Yifan Fan <fanyifan@nhn.com>                                 |
  +----------------------------------------------------------------------+
*/

#ifndef PHP_PDO_CUBRID_INI_H
#define PHP_PDO_CUBRID_INI_H

#include <cas_cci.h>

/************************************************************************
* PRIVATE DEFINITIONS
************************************************************************/

#if defined(WINDOWS)
#define SLEEP_SEC(X)	Sleep((X) * 1000)
#define SLEEP_MILISEC(sec, msec)	Sleep((sec) * 1000 + (msec))
#else
#define SLEEP_SEC(X)	sleep(X)
#define	SLEEP_MILISEC(sec, msec) \
			do { \
			  struct timeval sleep_time_val;			\
			  sleep_time_val.tv_sec = sec;				\
			  sleep_time_val.tv_usec = (msec) * 1000;	\
			  select(0, 0, 0, 0, &sleep_time_val);		\
			} while(0)
#endif

/* ARRAY */
typedef enum
{
    CUBRID_NUM = 1,
    CUBRID_ASSOC = 2,
    CUBRID_BOTH = CUBRID_NUM | CUBRID_ASSOC,
    CUBRID_OBJECT = 4,
} T_CUBRID_ARRAY_TYPE;

/* SCHEMA */
#define CUBRID_SCH_TABLE				CCI_SCH_CLASS
#define CUBRID_SCH_VIEW					CCI_SCH_VCLASS
#define CUBRID_SCH_QUERY_SPEC			CCI_SCH_QUERY_SPEC
#define CUBRID_SCH_ATTRIBUTE			CCI_SCH_ATTRIBUTE
#define CUBRID_SCH_TABLE_ATTRIBUTE		CCI_SCH_CLASS_ATTRIBUTE
#define CUBRID_SCH_METHOD				CCI_SCH_METHOD
#define CUBRID_SCH_TABLE_METHOD			CCI_SCH_CLASS_METHOD
#define CUBRID_SCH_METHOD_FILE			CCI_SCH_METHOD_FILE
#define CUBRID_SCH_SUPER_TABLE			CCI_SCH_SUPERCLASS
#define CUBRID_SCH_SUB_TABLE			CCI_SCH_SUBCLASS
#define CUBRID_SCH_CONSTRAINT			CCI_SCH_CONSTRAINT
#define CUBRID_SCH_TRIGGER				CCI_SCH_TRIGGER
#define CUBRID_SCH_TABLE_PRIVILEGE		CCI_SCH_CLASS_PRIVILEGE
#define CUBRID_SCH_COL_PRIVILEGE		CCI_SCH_ATTR_PRIVILEGE
#define CUBRID_SCH_DIRECT_SUPER_TABLE	CCI_SCH_DIRECT_SUPER_CLASS
#define CUBRID_SCH_PRIMARY_KEY			CCI_SCH_PRIMARY_KEY

/* check if query exec finished every interval */
#define EXEC_CHECK_INTERVAL 10

/* error codes */
#define CUBRID_ER_NO_MORE_MEMORY			-2001
#define CUBRID_ER_INVALID_SQL_TYPE			-2002
#define CUBRID_ER_CANNOT_GET_COLUMN_INFO 	-2003
#define CUBRID_ER_INIT_ARRAY_FAIL			-2004
#define CUBRID_ER_UNKNOWN_TYPE				-2005
#define CUBRID_ER_INVALID_PARAM				-2006
#define CUBRID_ER_INVALID_ARRAY_TYPE 		-2007
#define CUBRID_ER_NOT_SUPPORTED_TYPE 		-2008
#define CUBRID_ER_OPEN_FILE					-2009
#define CUBRID_ER_CREATE_TEMP_FILE			-2010
#define CUBRID_ER_TRANSFER_FAIL				-2011
#define CUBRID_ER_PHP						-2012
#define CUBRID_ER_REMOVE_FILE				-2013
#define CUBRID_ER_INVALID_CONN_HANDLE		-2014
#define CUBRID_ER_INVALID_STMT_HANDLE		-2015
#define CUBRID_ER_NOT_PREPARED				-2016
#define CUBRID_ER_PARAM_NOT_BIND			-2017
#define CUBRID_ER_INVALID_INDEX				-2018
#define CUBRID_ER_INVALID_CONN_STR			-2019
#define CUBRID_ER_ROLLBACK_IN_AUTOCOMMIT	-2020
#define CUBRID_ER_EXEC_TIMEOUT				-2021
#define CUBRID_ER_INVALID_CURSOR_POS		-2022
/* CAUTION! Also add the error message string to db_error[] */

/* Define Cubrid DB parameters */
typedef struct
{
    T_CCI_DB_PARAM parameter_id;
    const char *parameter_name;
} DB_PARAMETER;

static const DB_PARAMETER db_parameters[] = {
    {CCI_PARAM_ISOLATION_LEVEL, "PARAM_ISOLATION_LEVEL"},
    {CCI_PARAM_LOCK_TIMEOUT, "LOCK_TIMEOUT"},
    {CCI_PARAM_MAX_STRING_LENGTH, "MAX_STRING_LENGTH"},
    {CCI_PARAM_AUTO_COMMIT, "PARAM_AUTO_COMMIT"}
};

/************************************************************************
* PRIVATE TYPE DEFINITIONS
************************************************************************/

typedef struct
{
    const char *file;
    int line;
    unsigned int errcode;
    char *errmsg;
} pdo_cubrid_error_info;

typedef struct
{
    int conn_handle;
	
	int isolation_level;
	int lock_timeout;
	int max_string_len;
	int auto_commit;
	int query_timeout;
    pdo_cubrid_error_info einfo;
} pdo_cubrid_db_handle;

typedef struct
{
    pdo_cubrid_db_handle *H;
    int stmt_handle;

	int cursor_type;
    int affected_rows;
    int l_prepare;
    int col_count;
    long row_count;
	long cursor_pos;
    int bind_num;
    short *l_bind;
    T_CCI_CUBRID_STMT sql_type;
	T_CCI_PARAM_INFO *param_info;
    T_CCI_COL_INFO *col_info;
} pdo_cubrid_stmt;

extern pdo_driver_t pdo_cubrid_driver;

extern int _pdo_cubrid_error(pdo_dbh_t *dbh, pdo_stmt_t *stmt, int err_code, T_CCI_ERROR *error, const char *sql_state, const char *file, int line TSRMLS_DC);
#define pdo_cubrid_error(d,e,t,z) _pdo_cubrid_error(d, NULL, e, t, z, __FILE__, __LINE__ TSRMLS_CC)
#define pdo_cubrid_error_stmt(s,e,t,z) _pdo_cubrid_error(s->dbh, s, e, t, z, __FILE__, __LINE__ TSRMLS_CC)

extern struct pdo_stmt_methods cubrid_stmt_methods;

#endif /* PHP_PDO_CUBRID_INT_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=manual
 * vim<600: noet sw=4 ts=4
 */
