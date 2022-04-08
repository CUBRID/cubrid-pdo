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

/************************************************************************
* IMPORTED SYSTEM HEADER FILES
************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "ext/pdo/php_pdo.h"
#include "ext/pdo/php_pdo_driver.h"

#include "zend_exceptions.h"

/************************************************************************
* OTHER IMPORTED HEADER FILES
************************************************************************/

#include "php_pdo_cubrid.h"
#include "php_pdo_cubrid_int.h"
#include <cas_cci.h>
#include <broker_cas_error.h>

/************************************************************************
* PRIVATE TYPE DEFINITIONS
************************************************************************/

typedef struct
{
    int err_code;
    char *err_msg;
} DB_ERROR_INFO;

/* Define addtion error info */
static const DB_ERROR_INFO db_error[] = {
    {CUBRID_ER_NO_MORE_MEMORY, "Memory allocation error"},
    {CUBRID_ER_INVALID_SQL_TYPE, "Invalid SQL statement type"},
    {CUBRID_ER_CANNOT_GET_COLUMN_INFO, "Cannot get column info"},
    {CUBRID_ER_INIT_ARRAY_FAIL, "Array initializing error"},
    {CUBRID_ER_UNKNOWN_TYPE, "Unknown column type"},
    {CUBRID_ER_INVALID_PARAM, "Invalid parameter"},
    {CUBRID_ER_INVALID_ARRAY_TYPE, "Invalid array type"},
    {CUBRID_ER_NOT_SUPPORTED_TYPE, "Invalid type"},
    {CUBRID_ER_OPEN_FILE, "File open error"},
    {CUBRID_ER_CREATE_TEMP_FILE, "Temporary file open error"},
    {CUBRID_ER_TRANSFER_FAIL, "Glo transfering error"},
    {CUBRID_ER_PHP, "PHP error"},
    {CUBRID_ER_REMOVE_FILE, "Error removing file"},
    {CUBRID_ER_INVALID_CONN_HANDLE, "Invalid connection handle"},
    {CUBRID_ER_INVALID_STMT_HANDLE, "Invalid request handle"},
    {CUBRID_ER_NOT_PREPARED, "Not prepared SQL statment"},
    {CUBRID_ER_PARAM_NOT_BIND, "Param not bind"},
    {CUBRID_ER_INVALID_INDEX, "Invalid index number"},
    {CUBRID_ER_INVALID_CONN_STR, "Invalid connection string"},
	{CUBRID_ER_EXEC_TIMEOUT, "Exec query timeout"},
	{CUBRID_ER_INVALID_CURSOR_POS, "Invalid cursor position (forward only)"},
};

/************************************************************************
* PRIVATE FUNCTION PROTOTYPES
************************************************************************/

static int cubrid_get_err_msg(int err_code, char *err_buf, int buf_size);
static int get_db_param(pdo_cubrid_db_handle *H, T_CCI_ERROR *error);
static int fetch_a_row(zval *arg, int req_handle, int type TSRMLS_DC);
static int cubrid_array_destroy(HashTable * ht ZEND_FILE_LINE_DC);
static int cubrid_add_index_array(zval *arg, uint index, T_CCI_SET in_set TSRMLS_DC);
static int cubrid_add_assoc_array(zval *arg, char *key, T_CCI_SET in_set TSRMLS_DC);

/************************************************************************
* IMPLEMENTATION OF CUBRID PDO
************************************************************************/

int _pdo_cubrid_error(pdo_dbh_t *dbh, pdo_stmt_t *stmt, int err_code, T_CCI_ERROR *error, 
		const char *sql_state, const char *file, int line TSRMLS_DC)
{
    char err_buf[1024];
    char *err_msg;
	char *err_facility_msg;

	int real_err_code;
	char *real_err_msg;
	
    int len;
	int cubrid_retval = 0;

    pdo_cubrid_db_handle *H = (pdo_cubrid_db_handle *)dbh->driver_data;
    pdo_error_type *pdo_err = stmt ? &stmt->error_code : &dbh->error_code;
    pdo_cubrid_error_info *einfo = &H->einfo;

	einfo->file = file;
    einfo->line = line;

	if (err_code == CCI_ER_DBMS) {
		err_facility_msg = "DBMS";
		if (error) {
			real_err_code = error->err_code;		
			real_err_msg = error->err_msg;
		} else {
			real_err_code = 0;
			real_err_msg = "Unknown DBMS error";
		}
	} else if (err_code < CUBRID_ER_END) {
		err_facility_msg = "UNKNOWN";

		real_err_code = -1;
		real_err_msg = NULL;
	} else{
		if (err_code > CAS_ER_IS) {
			err_facility_msg = "CAS";
		} else if (err_code > CCI_ER_END) {
			err_facility_msg = "CCI";
		} else if (err_code > CUBRID_ER_END) {
			err_facility_msg = "CLIENT";
		}

		if (err_code > CCI_ER_END) {
			cubrid_retval = cci_get_err_msg(err_code, err_buf, sizeof(err_buf));
		} else {
			cubrid_retval = cubrid_get_err_msg(err_code, err_buf, sizeof(err_buf));
		}

		if (cubrid_retval == 0) {
			real_err_msg = err_buf;
		} else {
			real_err_msg = "Unknown error";
		}

		real_err_code = err_code;
	}

    einfo->errcode = real_err_code;

	if (einfo->errmsg) {
		pefree(einfo->errmsg, dbh->is_persistent);
		einfo->errmsg = NULL;
    }

	if (real_err_msg) {
		len = strlen(err_facility_msg) + strlen(real_err_msg);
		err_msg = pemalloc(len + 3, dbh->is_persistent);	
		snprintf(err_msg, len + 3, "%s, %s", err_facility_msg, real_err_msg);

		einfo->errmsg = err_msg;
	}

    if (sql_state == NULL) {
		strcpy(*pdo_err, "HY000");
    } else {
		strcpy (*pdo_err, sql_state);
    }

    if (!dbh->methods) {
		zend_throw_exception_ex(php_pdo_get_exception(), einfo->errcode TSRMLS_CC, 
				"SQLSTATE[%s] [%d] %s", *pdo_err, einfo->errcode, einfo->errmsg);
    }
    
    return err_code;
}

static int pdo_cubrid_fetch_error_func(pdo_dbh_t *dbh, pdo_stmt_t *stmt, zval *info TSRMLS_DC)
{
    pdo_cubrid_db_handle *H = (pdo_cubrid_db_handle *)dbh->driver_data;
    pdo_cubrid_error_info *einfo = &H->einfo;

    if (einfo->errcode) {
		add_next_index_long(info, einfo->errcode);
		add_next_index_string(info, einfo->errmsg, 1);
    }
    
    return 1;
}

static int cubrid_handle_closer(pdo_dbh_t *dbh TSRMLS_DC)
{
    pdo_cubrid_db_handle *H = (pdo_cubrid_db_handle *)dbh->driver_data;
    T_CCI_ERROR error;
	int i;

	if (H) {
		for (i = 0; i < H->stmt_count; i++) {
			if (H->stmt_list[i]) {
				H->stmt_list[i]->stmt_handle = 0;
				H->stmt_list[i]->H = NULL;
			}
		}

		if (H->conn_handle) {
			cci_disconnect(H->conn_handle, &error);
			H->conn_handle = 0;
		}

		if (H->einfo.errmsg) {
			pefree(H->einfo.errmsg, dbh->is_persistent);
			H->einfo.errmsg = NULL;
		}

		if (H->stmt_count) {
			efree(H->stmt_list);
		}

		pefree(H, dbh->is_persistent);
		dbh->driver_data = NULL;
	}	

	return 0;
}

static int cubrid_handle_preparer(pdo_dbh_t *dbh, const char *sql, long sql_len, 
		pdo_stmt_t *stmt, zval *driver_options TSRMLS_DC)
{
	pdo_cubrid_db_handle *H = (pdo_cubrid_db_handle *)dbh->driver_data;
	pdo_cubrid_stmt *S = ecalloc(1, sizeof(pdo_cubrid_stmt));

	int ret = 0, stmt_handle = 0;
	int cubrid_retval = 0;
	char *nsql = NULL;
	int nsql_len = 0;
    T_CCI_ERROR error;
	int i;

	S->H = H;
	stmt->driver_data = S;
	stmt->methods = &cubrid_stmt_methods;

	S->lob = NULL;

	if (pdo_attr_lval(driver_options, PDO_ATTR_CURSOR, PDO_CURSOR_FWDONLY TSRMLS_CC) == PDO_CURSOR_SCROLL) {
		S->cursor_type = PDO_CURSOR_SCROLL;
	} else {
		S->cursor_type = PDO_CURSOR_FWDONLY;
	}

	stmt->supports_placeholders = PDO_PLACEHOLDER_POSITIONAL;
	ret = pdo_parse_params(stmt, (char *)sql, sql_len, &nsql, &nsql_len TSRMLS_CC);

	if (ret == 1) {
		sql = nsql;
		sql_len = nsql_len;
	} else if (ret == -1) {
		strcpy(dbh->error_code, stmt->error_code);
		return 0;
	}

	if ((stmt_handle = cci_prepare(H->conn_handle, (char *)sql, 0, &error)) < 0) {
		pdo_cubrid_error(dbh, stmt_handle, &error, NULL);
		if (nsql) {
			efree(nsql);
		}

		return 0;
	}
       if(H->query_timeout != -1 && H->query_timeout != 0)
       {
           cci_set_query_timeout(stmt_handle,H->query_timeout*1000);
       }
	S->stmt_handle = stmt_handle;
	S->bind_num = cci_get_bind_num(stmt_handle);

	if (S->bind_num > 0) {
		S->l_bind = (short *) safe_emalloc(S->bind_num, sizeof(short), 0);
		for (i = 0; i < S->bind_num; i++) {
			S->l_bind[i] = 0;
		}

		if ((cubrid_retval = cci_get_param_info(stmt_handle, &(S->param_info), &error)) < 0) {
			if (cubrid_retval != CAS_ER_NOT_IMPLEMENTED) {
				pdo_cubrid_error_stmt(stmt, cubrid_retval, &error, NULL);
				return 0;
			}
		}
    }

	S->l_prepare = 1;

	if (nsql) {
		efree(nsql);
	}

	H->stmt_list = erealloc(H->stmt_list, (H->stmt_count + 1) * sizeof(pdo_cubrid_stmt *));
	H->stmt_list[H->stmt_count] = S;
	H->stmt_count++;

	return 1;
}

static long cubrid_handle_doer(pdo_dbh_t *dbh, const char *sql, long sql_len TSRMLS_DC)
{
	pdo_cubrid_db_handle *H = (pdo_cubrid_db_handle *)dbh->driver_data;

	int stmt_handle = 0;
	char exec_flag = CCI_EXEC_QUERY_ALL;
    T_CCI_ERROR error;

	long ret = 0;

	if ((stmt_handle = cci_prepare(H->conn_handle, (char *)sql, 0, &error)) < 0) {
		pdo_cubrid_error(dbh, stmt_handle, &error, NULL);
		return -1;
	}

       if(H->query_timeout != -1&& H->query_timeout != 0)
       {
           cci_set_query_timeout(stmt_handle,H->query_timeout*1000);
       }

    ret = cci_execute(stmt_handle, exec_flag, 0, &error);

	if (ret < 0) {
		pdo_cubrid_error(dbh, ret, &error, NULL);
		return -1;
	}

	return ret;
}

static int cubrid_handle_quoter(pdo_dbh_t *dbh, const char *unquoted, int unquotedlen, 
		char **quoted, int *quotedlen, enum pdo_param_type paramtype TSRMLS_DC)
{
	pdo_cubrid_db_handle *H = (pdo_cubrid_db_handle *)dbh->driver_data;

	T_CCI_ERROR error;
	int ret = 0;
	
	*quoted = (char *) emalloc(2 * unquotedlen + 18);

	if ((ret = cci_escape_string(H->conn_handle, *quoted, unquoted, unquotedlen, &error)) < 0) {
		pdo_cubrid_error(dbh, ret, &error, NULL);
		efree(*quoted);
		return 0;
	}
	*quotedlen = ret;
	(*quoted)[*quotedlen] = '\0';

	return 1;
}

static int cubrid_handle_begin(pdo_dbh_t *dbh TSRMLS_DC)
{
	int cubrid_retval = 0;
	T_CCI_ERROR error;

	pdo_cubrid_db_handle *H = (pdo_cubrid_db_handle *)dbh->driver_data;

	if (H->auto_commit == CCI_AUTOCOMMIT_TRUE) {
		if ((cubrid_retval = cci_set_autocommit(H->conn_handle, CCI_AUTOCOMMIT_FALSE)) < 0) {
			pdo_cubrid_error(dbh, cubrid_retval, &error, NULL);
			return 0;
		}

		dbh->auto_commit = 0;
	} else {
		if ((cubrid_retval = cci_end_tran(H->conn_handle, CCI_TRAN_COMMIT, &error)) < 0) {
			pdo_cubrid_error(dbh, cubrid_retval, &error, NULL);
			return 0;
		}
	}

	return 1;
}

static int cubrid_handle_commit(pdo_dbh_t *dbh TSRMLS_DC)
{
	pdo_cubrid_db_handle *H = (pdo_cubrid_db_handle *)dbh->driver_data;

	int cubrid_retval = 0;
    T_CCI_ERROR error;

	if ((cubrid_retval = cci_end_tran(H->conn_handle, CCI_TRAN_COMMIT, &error)) < 0) {
		pdo_cubrid_error(dbh, cubrid_retval, &error, NULL);
		return 0;
    }

	if (H->auto_commit == CCI_AUTOCOMMIT_TRUE) {
		if ((cubrid_retval = cci_set_autocommit(H->conn_handle, CCI_AUTOCOMMIT_TRUE)) < 0) {
			pdo_cubrid_error(dbh, cubrid_retval, &error, NULL);
			return 0;
		}

		dbh->auto_commit = 1;
	}

	return 1;
}

static int cubrid_handle_rollback(pdo_dbh_t *dbh TSRMLS_DC)
{
	pdo_cubrid_db_handle *H = (pdo_cubrid_db_handle *)dbh->driver_data;

	int cubrid_retval = 0;
    T_CCI_ERROR error;

	if ((cubrid_retval = cci_end_tran(H->conn_handle, CCI_TRAN_ROLLBACK, &error)) < 0) {
		pdo_cubrid_error(dbh, cubrid_retval, &error, NULL);
		return 0;
    }

	if (H->auto_commit == CCI_AUTOCOMMIT_TRUE) {
		if ((cubrid_retval = cci_set_autocommit(H->conn_handle, CCI_AUTOCOMMIT_TRUE)) < 0) {
			pdo_cubrid_error(dbh, cubrid_retval, &error, NULL);
			return 0;
		}

		dbh->auto_commit = 1;
	}

	return 1;
}

static int pdo_cubrid_set_attribute(pdo_dbh_t *dbh, long attr, zval *val TSRMLS_DC)
{
	pdo_cubrid_db_handle *H = (pdo_cubrid_db_handle *)dbh->driver_data;
	int cubrid_retval = 0;
	T_CCI_ERROR error;

	switch (attr) {
	case PDO_ATTR_AUTOCOMMIT:
		convert_to_boolean(val);

		if (dbh->auto_commit ^ Z_BVAL_P(val)) {
			if (dbh->auto_commit == 0) {
				if ((cubrid_retval = cci_end_tran(H->conn_handle, CCI_TRAN_COMMIT, &error)) < 0) {
					pdo_cubrid_error(dbh, cubrid_retval, &error, NULL);
					return 0;
				}
			}
			
			if ((cubrid_retval = cci_set_autocommit(H->conn_handle, Z_BVAL_P(val))) < 0) {
				pdo_cubrid_error(dbh, cubrid_retval, &error, NULL);
				return 0;	
			}

			H->auto_commit = Z_BVAL_P(val);
			dbh->auto_commit = Z_BVAL_P(val);
		}

		return 1;

	case PDO_ATTR_TIMEOUT:
		if (H->query_timeout ^ Z_LVAL_P(val)) {
			if ((Z_LVAL_P(val) == 0) || (Z_LVAL_P(val) < 0 && Z_LVAL_P(val) != -1)) {
				return 0;
			}

			H->query_timeout = Z_LVAL_P(val);
		}

		return 1;
	case PDO_CUBRID_ATTR_ISOLATION_LEVEL:
		if ((cubrid_retval = cci_set_isolation_level(H->conn_handle, Z_LVAL_P(val), &error)) < 0) {
			pdo_cubrid_error(dbh, cubrid_retval, &error, NULL);
			return 0;	
		}

		return 1;
	case PDO_CUBRID_ATTR_LOCK_TIMEOUT:
		if ((cubrid_retval = cci_set_lock_timeout(H->conn_handle, Z_LVAL_P(val), &error)) < 0) {
			pdo_cubrid_error(dbh, cubrid_retval, &error, NULL);
			return 0;	
		}

		return 1;
	default:
		break;
	}

	return 0;
}

static int pdo_cubrid_get_attribute(pdo_dbh_t *dbh, long attr, zval *return_value TSRMLS_DC)
{
	pdo_cubrid_db_handle *H = (pdo_cubrid_db_handle *)dbh->driver_data;
	int cubrid_retval = 0, param_value;
	T_CCI_ERROR error;

	if (!H->conn_handle) {
		pdo_cubrid_error(dbh, CUBRID_ER_INVALID_CONN_HANDLE, NULL, NULL);
		return 0;
	}

	switch (attr) {
	case PDO_ATTR_AUTOCOMMIT:
		ZVAL_BOOL(return_value, dbh->auto_commit);

		break;
	case PDO_ATTR_TIMEOUT:
		ZVAL_LONG(return_value, H->query_timeout);

		break;
	case PDO_ATTR_CLIENT_VERSION: 
	{
		int major, minor, patch;
		char info[128];

		cci_get_version(&major, &minor, &patch);
		snprintf(info, sizeof(info), "%d.%d.%d", major, minor, patch);

		ZVAL_STRING(return_value, info, 1);
	}
		break;
	case PDO_ATTR_SERVER_VERSION:
	case PDO_ATTR_SERVER_INFO:
	{
		char buf[64];
		cci_get_db_version(H->conn_handle, buf, sizeof(buf));

		ZVAL_STRING(return_value, buf, 1);
	}
		break;
	case PDO_CUBRID_ATTR_ISOLATION_LEVEL:
		if ((cubrid_retval = cci_get_db_parameter(H->conn_handle, CCI_PARAM_ISOLATION_LEVEL, &param_value, &error)) < 0) {
			pdo_cubrid_error(dbh, cubrid_retval, &error, NULL);
			return 0;	
		}

		ZVAL_LONG(return_value, param_value);

		break;
	case PDO_CUBRID_ATTR_LOCK_TIMEOUT:
		if ((cubrid_retval = cci_get_db_parameter(H->conn_handle, CCI_PARAM_LOCK_TIMEOUT, &param_value, &error)) < 0) {
			pdo_cubrid_error(dbh, cubrid_retval, &error, NULL);
			return 0;	
		}

		ZVAL_LONG(return_value, param_value);

		break;
	case PDO_CUBRID_ATTR_MAX_STRING_LENGTH:
		if ((cubrid_retval = cci_get_db_parameter(H->conn_handle, CCI_PARAM_MAX_STRING_LENGTH, &param_value, &error)) < 0) {
			//pdo_cubrid_error(dbh, cubrid_retval, &error, NULL);
			//return 0;	
			param_value=0;
		}

		ZVAL_LONG(return_value, param_value);

		break;
	default:
		return 0;
	}

	return 1;
}

static char *pdo_cubrid_last_insert_id(pdo_dbh_t *dbh, const char *name, unsigned int *len TSRMLS_DC)
{
	pdo_cubrid_db_handle *H = (pdo_cubrid_db_handle *)dbh->driver_data;

    int cubrid_retval = 0;
    T_CCI_ERROR error;

	char *last_id = NULL;
	char *id = NULL;

	if ((cubrid_retval = cci_get_last_insert_id(H->conn_handle, &last_id, &error)) < 0) {
		pdo_cubrid_error(dbh, cubrid_retval, &error, NULL);
		return NULL;
	}

	if (last_id == NULL) {
		*len = 0;
	} else {
		*len = strlen(last_id);
		id = estrdup(last_id);
	}

	return id;
}

static int pdo_cubrid_check_liveness(pdo_dbh_t *dbh TSRMLS_DC)
{
	pdo_cubrid_db_handle *H = (pdo_cubrid_db_handle *)dbh->driver_data;

	int stmt_handle = 0;
    T_CCI_ERROR error;
	long ret = 0;
	int cubrid_retval = 0;
	int result = 0, ind;

	if ((stmt_handle = cci_prepare(H->conn_handle, "select 1+1 from db_root", 0, &error)) < 0) {
		return FAILURE;
	}

    if ((ret = cci_execute(stmt_handle, 0, 0, &error)) < 0) {
		return FAILURE;
	}

	while (1) {
		cubrid_retval = cci_cursor(stmt_handle, 1, CCI_CURSOR_CURRENT, &error);
		if (cubrid_retval == CCI_ER_NO_MORE_DATA) {
			break;
		}

		if (cubrid_retval < 0) {
			return FAILURE;	
		}

		if ((cubrid_retval = cci_fetch(stmt_handle, &error)) < 0) {
			return FAILURE;
		}

		if ((cubrid_retval = cci_get_data(stmt_handle, 1, CCI_A_TYPE_INT, &result, &ind)) < 0) {
			return FAILURE;
		}

		if (result != 2) {
			return FAILURE;
		}
	}

	return SUCCESS;
}

static PHP_METHOD(PDO, cubrid_schema)
{
	pdo_dbh_t *dbh;
	pdo_cubrid_db_handle *H;

    char *class_name = NULL, *attr_name = NULL;
    long schema_type, class_name_len, attr_name_len;

    int request_handle;
    int flag = 0;
    int cubrid_retval = 0;
    int i = 0;

    T_CCI_ERROR error;
    zval *temp_element = NULL;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l|ss", 
		&schema_type, &class_name, &class_name_len, &attr_name, &attr_name_len) == FAILURE) {
		return;
    }
    
    switch (schema_type) {
    case CUBRID_SCH_TABLE:
    case CUBRID_SCH_VIEW:
		flag = CCI_CLASS_NAME_PATTERN_MATCH;
		break;
    case CUBRID_SCH_ATTRIBUTE:
    case CUBRID_SCH_TABLE_ATTRIBUTE:
		flag = CCI_ATTR_NAME_PATTERN_MATCH;
		break;
    default:
		flag = 0;
		break;
    }

	dbh = zend_object_store_get_object(getThis() TSRMLS_CC);
	PDO_CONSTRUCT_CHECK;

	H = (pdo_cubrid_db_handle *)dbh->driver_data;

    if ((cubrid_retval = cci_schema_info(H->conn_handle, schema_type, class_name, attr_name, (char) flag, &error)) < 0) {
		pdo_cubrid_error(dbh, cubrid_retval, &error, NULL);
		RETURN_FALSE;
    }

    request_handle = cubrid_retval;

    array_init(return_value);

    for (i = 0; ; i++) {
		cubrid_retval = cci_cursor(request_handle, 1, CCI_CURSOR_CURRENT, &error);
		if (cubrid_retval == CCI_ER_NO_MORE_DATA) {
			break;
		}

		if (cubrid_retval < 0) {
			pdo_cubrid_error(dbh, cubrid_retval, &error, NULL);
            goto ERR_CUBRID_SCHEMA;
		}

		if ((cubrid_retval = cci_fetch(request_handle, &error)) < 0) {
			pdo_cubrid_error(dbh, cubrid_retval, &error, NULL);
            goto ERR_CUBRID_SCHEMA;
		}

		MAKE_STD_ZVAL(temp_element);
		if ((cubrid_retval = fetch_a_row(temp_element, request_handle, CUBRID_ASSOC TSRMLS_CC)) != SUCCESS) {
			pdo_cubrid_error(dbh, cubrid_retval, NULL, NULL);
			FREE_ZVAL(temp_element);
            goto ERR_CUBRID_SCHEMA;
		}

		zend_hash_index_update(Z_ARRVAL_P(return_value), i, (void *) &temp_element, sizeof(zval *), NULL);
    }

    cci_close_req_handle(request_handle);

    return;

ERR_CUBRID_SCHEMA:

    cubrid_array_destroy(return_value->value.ht ZEND_FILE_LINE_CC);
    cci_close_req_handle(request_handle);

    RETURN_FALSE;
}

static const zend_function_entry dbh_methods[] = {
	PHP_ME(PDO, cubrid_schema, NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};

static const zend_function_entry *pdo_cubrid_get_driver_methods(pdo_dbh_t *dbh, int kind TSRMLS_DC)
{
	switch (kind) {
		case PDO_DBH_DRIVER_METHOD_KIND_DBH:
			return dbh_methods;
		default:
			return NULL;
	}
}

static struct pdo_dbh_methods cubrid_methods = {
	cubrid_handle_closer,
	cubrid_handle_preparer,
	cubrid_handle_doer,
	cubrid_handle_quoter,
	cubrid_handle_begin,
	cubrid_handle_commit,
	cubrid_handle_rollback,
	pdo_cubrid_set_attribute,
	pdo_cubrid_last_insert_id,
	pdo_cubrid_fetch_error_func,
	pdo_cubrid_get_attribute,
	pdo_cubrid_check_liveness,
	pdo_cubrid_get_driver_methods, /* get driver methods */
	NULL, /* request shutdown */
};

static int pdo_cubrid_handle_factory(pdo_dbh_t *dbh, zval *driver_options TSRMLS_DC)
{
    pdo_cubrid_db_handle *H = NULL;

    int cubrid_retval = 0, cubrid_conn;
    T_CCI_ERROR error;
    int ret = 0, vars_size;
	int i;

    char *host = NULL, *dbname = NULL;
    unsigned int port = 55300;

    struct pdo_data_src_parser vars[] = {
		{ "host", "localhost", 0 },
		{ "port", "55300", 0},
		{ "dbname", "demodb", 0 }
    };

	char connect_url[2048] = {'\0'};

	vars_size = sizeof(vars)/sizeof(vars[0]);

    H = pecalloc(1, sizeof(pdo_cubrid_db_handle), dbh->is_persistent);
    dbh->driver_data = H;

    H->einfo.errcode = 0;
    H->einfo.errmsg = NULL;

    if (php_pdo_parse_data_source(dbh->data_source, dbh->data_source_len, vars, vars_size) != vars_size) {
		pdo_cubrid_error(dbh, CUBRID_ER_INVALID_CONN_STR, NULL, NULL);
		goto cleanup;
	}

	host = vars[0].optval;
	if(vars[1].optval) {
		port = atoi(vars[1].optval);
	}
	dbname = vars[2].optval;

	snprintf(connect_url, sizeof(connect_url), "cci:CUBRID:%s:%d:%s:%s:%s:", host, (int)port, dbname, dbh->username, dbh->password);

	if (driver_options)
	{
		HashPosition position;
		HashTable *ht = Z_ARRVAL_P(driver_options);
		zval **data = NULL;
		int first = 1;
		char temp_buffer[1024]={'\0'};

		for (zend_hash_internal_pointer_reset_ex(ht, &position);
			zend_hash_get_current_data_ex(ht, (void**) &data, &position) == SUCCESS;
			zend_hash_move_forward_ex(ht, &position)) {
				char *key = NULL;
				uint  klen;
				ulong index;

				if (Z_TYPE_PP(data) != IS_STRING) {
					pdo_cubrid_error(dbh, CUBRID_ER_INVALID_CONN_STR, NULL, NULL);
					goto cleanup;
				}

				if (zend_hash_get_current_key_ex(ht, &key, &klen, &index, 0, &position) != HASH_KEY_IS_STRING) {
					pdo_cubrid_error(dbh, CUBRID_ER_INVALID_CONN_STR, NULL, NULL);
					goto cleanup;
				} 

				snprintf(temp_buffer, sizeof(temp_buffer)-1, "%s%s=%s", first?"?":"&", key, Z_STRVAL_PP(data));
				strncat(connect_url, temp_buffer, sizeof(connect_url)-strlen(connect_url)-1);

				if (first)
				{
					first = 0;
				}
		}
	}

	if ((cubrid_conn = cci_connect_with_url_ex(connect_url, dbh->username, dbh->password, &error)) < 0) {
		pdo_cubrid_error(dbh, cubrid_conn, &error, NULL);
		goto cleanup;
	}

    H->conn_handle = cubrid_conn;
	H->query_timeout = -1;
	H->stmt_count = 0;
	H->stmt_list = NULL;

	if ((H->auto_commit = cci_get_autocommit(cubrid_conn)) < 0) {
		pdo_cubrid_error(dbh, H->auto_commit, NULL, NULL);
		goto cleanup;
	}

	if ((cubrid_retval = get_db_param(H, &error)) < 0 &&
		cubrid_retval != CAS_ER_NOT_IMPLEMENTED) {
		pdo_cubrid_error(dbh, cubrid_retval, &error, NULL);
		cci_disconnect(cubrid_conn, &error);
		goto cleanup;
	}

	if ((cubrid_retval = cci_end_tran(cubrid_conn, CCI_TRAN_COMMIT, &error)) < 0) {
		pdo_cubrid_error(dbh, cubrid_retval, &error, NULL);
		cci_disconnect(cubrid_conn, &error);
		goto cleanup;
    }

	dbh->auto_commit = H->auto_commit;
	dbh->native_case = PDO_CASE_LOWER;
	dbh->alloc_own_columns = 1;
	dbh->max_escaped_char_length = 2;
	dbh->methods = &cubrid_methods;

    ret = 1;

cleanup:    
    for (i = 0; i < sizeof(vars)/sizeof(vars[0]); i++) {
		if (vars[i].freeme) {
			efree(vars[i].optval);
		}
    }

    dbh->methods = &cubrid_methods;

	if (!ret) {
		pefree(H, dbh->is_persistent);
		dbh->driver_data = NULL;
	}

    return ret;
}

pdo_driver_t pdo_cubrid_driver = {
	PDO_DRIVER_HEADER(cubrid),
	pdo_cubrid_handle_factory
};

/************************************************************************
* PRIVATE FUNCTIONS IMPLEMENTATION
************************************************************************/

static int cubrid_get_err_msg(int err_code, char *err_buf, int buf_size)
{
	int i;
	int size = sizeof(db_error)/sizeof(db_error[0]);

	if (err_buf == NULL || buf_size <= 0) {
		return -1;
	}

	for (i = 0; i < size; i++) {
		if (err_code == db_error[i].err_code) {
			strlcpy(err_buf, db_error[i].err_msg, buf_size);
			return 0;
		}
	}
	
	return -1;
}

static int get_db_param(pdo_cubrid_db_handle *H, T_CCI_ERROR *error)
{
	int isolation_level, lock_timeout, max_string_len;
	int cubrid_retval = 0;

	if ((cubrid_retval = cci_get_db_parameter(H->conn_handle, 
					CCI_PARAM_ISOLATION_LEVEL, &isolation_level, error)) < 0) {
		return cubrid_retval;
    }

	H->isolation_level = isolation_level;

	if ((cubrid_retval = cci_get_db_parameter(H->conn_handle, 
					CCI_PARAM_LOCK_TIMEOUT, &lock_timeout, error)) < 0) {
		return cubrid_retval;
    }

	H->lock_timeout = lock_timeout;

	if ((cubrid_retval = cci_get_db_parameter(H->conn_handle, 
					CCI_PARAM_MAX_STRING_LENGTH, &max_string_len, error)) < 0) {
		//php_printf("CCI_PARAM_MAX_STRING_LENGTH  %d:%s<br />",error->err_code,error->err_msg);
		//return cubrid_retval;
		max_string_len=0;
    }

	H->max_string_len = max_string_len;

    return 0; 
}

static int fetch_a_row(zval *arg, int req_handle, int type TSRMLS_DC)
{
    T_CCI_COL_INFO *column_info = NULL;
    int column_count = 0;
    T_CCI_U_TYPE column_type;
    char *column_name;

    int cubrid_retval = 0;
    int null_indicator;
    int i;

    if ((column_info = cci_get_result_info(req_handle, NULL, &column_count)) == NULL) {
		return CUBRID_ER_CANNOT_GET_COLUMN_INFO;
    }

    array_init(arg);

    for (i = 0; i < column_count; i++) {
		column_type = CCI_GET_RESULT_INFO_TYPE(column_info, i + 1);
		column_name = CCI_GET_RESULT_INFO_NAME(column_info, i + 1);

		if (CCI_IS_SET_TYPE(column_type) || CCI_IS_MULTISET_TYPE(column_type) || 
				CCI_IS_SEQUENCE_TYPE(column_type)) {
			T_CCI_SET res_buf = NULL;

			if ((cubrid_retval = cci_get_data(req_handle, i + 1, CCI_A_TYPE_SET, &res_buf, &null_indicator)) < 0) {
				goto ERR_FETCH_A_ROW;
			}

			if (null_indicator < 0) {
				if (type & CUBRID_NUM) {
					add_index_unset(arg, i);
				}

				if (type & CUBRID_ASSOC) {
					add_assoc_unset(arg, column_name);
				}
			} else {
				if (type & CUBRID_NUM) {
					cubrid_retval = cubrid_add_index_array(arg, i, res_buf TSRMLS_CC);
				} 
				
				if (type & CUBRID_ASSOC) {
					cubrid_retval = cubrid_add_assoc_array(arg, column_name, res_buf TSRMLS_CC);
				}
				
				if (cubrid_retval < 0) {
					cci_set_free(res_buf);
					goto ERR_FETCH_A_ROW;
				}

				cci_set_free(res_buf);
			}
		} else {
			char *res_buf = NULL;

			if ((cubrid_retval = cci_get_data(req_handle, i + 1, CCI_A_TYPE_STR, &res_buf, &null_indicator)) < 0) {
				goto ERR_FETCH_A_ROW;
			}

			if (null_indicator < 0) {
				if (type & CUBRID_NUM) {
					add_index_unset(arg, i);
				}

				if (type & CUBRID_ASSOC) {
					add_assoc_unset(arg, column_name);
				}
			} else {
				if (type & CUBRID_NUM) {
					add_index_stringl(arg, i, res_buf, null_indicator, 1);
				} 
				
				if (type & CUBRID_ASSOC) {
					add_assoc_stringl(arg, column_name, res_buf, null_indicator, 1);
				}
			}
		}
    }

    return SUCCESS;

ERR_FETCH_A_ROW:
    cubrid_array_destroy(arg->value.ht ZEND_FILE_LINE_CC);
    return cubrid_retval;
}

static int cubrid_array_destroy(HashTable * ht ZEND_FILE_LINE_DC)
{
    zend_hash_destroy(ht);
    FREE_HASHTABLE_REL(ht);
    return SUCCESS;
}

static int cubrid_add_index_array(zval *arg, uint index, T_CCI_SET in_set TSRMLS_DC)
{
    zval *tmp_ptr;

    int i;
    int res;
    int ind;
    char *buffer;

    int set_size = cci_set_size(in_set);

    MAKE_STD_ZVAL(tmp_ptr);
    array_init(tmp_ptr);

    for (i = 0; i < set_size; i++) {
		res = cci_set_get(in_set, i + 1, CCI_A_TYPE_STR, &buffer, &ind);
		if (res < 0) {
			cubrid_array_destroy(HASH_OF(tmp_ptr) ZEND_FILE_LINE_CC);
			FREE_ZVAL(tmp_ptr);
			return res;
		}

		if (ind < 0) {
			add_index_unset(tmp_ptr, i);
		} else {
			add_index_string(tmp_ptr, i, buffer, 1);
		}
    }

    res = zend_hash_index_update(HASH_OF(arg), index, (void *) &tmp_ptr, sizeof(zval *), NULL);
    if (res == FAILURE) {
		cubrid_array_destroy(HASH_OF(tmp_ptr) ZEND_FILE_LINE_CC);
		FREE_ZVAL(tmp_ptr);
		return CUBRID_ER_PHP;
    }

    return 0;
}

static int cubrid_add_assoc_array(zval *arg, char *key, T_CCI_SET in_set TSRMLS_DC)
{
    zval *tmp_ptr;

    int i;
    int ind;
    char *buffer;
    int cubrid_retval = 0;

    int set_size = cci_set_size(in_set);

    MAKE_STD_ZVAL(tmp_ptr);
    array_init(tmp_ptr);

    for (i = 0; i < set_size; i++) {
		if ((cubrid_retval = cci_set_get(in_set, i + 1, CCI_A_TYPE_STR, &buffer, &ind)) < 0) {
			cubrid_array_destroy(HASH_OF(tmp_ptr) ZEND_FILE_LINE_CC);
			FREE_ZVAL(tmp_ptr);
			return cubrid_retval;
		}

		if (ind < 0) {
			add_index_unset(tmp_ptr, i);
		} else {
			add_index_string(tmp_ptr, i, buffer, 1);
		}
    }

    if ((cubrid_retval = zend_hash_update(HASH_OF(arg), key, strlen(key) + 1, (void *) &tmp_ptr, sizeof(zval *), NULL)) == FAILURE) {
		cubrid_array_destroy(HASH_OF(tmp_ptr) ZEND_FILE_LINE_CC);
		FREE_ZVAL(tmp_ptr);
		return CUBRID_ER_PHP;
    }

    return 0;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=manual
 * vim<600: noet sw=4 ts=4
 */
