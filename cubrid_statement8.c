/*
  +----------------------------------------------------------------------+
  | PHP Version 8                                                        |
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

#include "php_pdo_cubrid8.h"
#include "php_pdo_cubrid8_int.h"
#include <cas_cci.h>
#include <broker_cas_error.h>

/************************************************************************
* PRIVATE DEFINITIONS
************************************************************************/

#define CUBRID_LOB_READ_BUF_SIZE    8192

/* Maximum length for the Cubrid data types */
#define MAX_CUBRID_CHAR_LEN   1073741823
#define MAX_LEN_INTEGER	      (10 + 1)
#define MAX_LEN_SMALLINT      (5 + 1)
#define MAX_LEN_BIGINT	      (19 + 1)
#define MAX_LEN_FLOAT	      (14 + 1)
#define MAX_LEN_DOUBLE	      (28 + 1)
#define MAX_LEN_MONETARY      (28 + 2)
#define MAX_LEN_DATE	      10
#define MAX_LEN_TIME	      8
#define MAX_LEN_TIMESTAMP     23
#define MAX_LEN_DATETIME      MAX_LEN_TIMESTAMP
#define MAX_LEN_OBJECT	      MAX_CUBRID_CHAR_LEN
#define MAX_LEN_SET	          MAX_CUBRID_CHAR_LEN
#define MAX_LEN_MULTISET      MAX_CUBRID_CHAR_LEN
#define MAX_LEN_SEQUENCE      MAX_CUBRID_CHAR_LEN
#define MAX_LEN_LOB			  MAX_CUBRID_CHAR_LEN

/************************************************************************
* PRIVATE TYPE DEFINITIONS
************************************************************************/

typedef struct
{
    char *type_name;
    T_CCI_U_TYPE cubrid_u_type;
    int len;
} DB_TYPE_INFO;

/* Define Cubrid supported date types */
static const DB_TYPE_INFO db_type_info[] = {
    {"NULL", CCI_U_TYPE_NULL, 0},
    {"UNKNOWN", CCI_U_TYPE_UNKNOWN, MAX_LEN_OBJECT},

    {"CHAR", CCI_U_TYPE_CHAR, -1},
    {"STRING", CCI_U_TYPE_STRING, -1},
    {"NCHAR", CCI_U_TYPE_NCHAR, -1},
    {"VARNCHAR", CCI_U_TYPE_VARNCHAR, -1},

    {"BIT", CCI_U_TYPE_BIT, -1},
    {"VARBIT", CCI_U_TYPE_VARBIT, -1},

    {"NUMERIC", CCI_U_TYPE_NUMERIC, -1},
    {"NUMBER", CCI_U_TYPE_NUMERIC, -1},
    {"INT", CCI_U_TYPE_INT, MAX_LEN_INTEGER},
    {"SHORT", CCI_U_TYPE_SHORT, MAX_LEN_SMALLINT},
    {"BIGINT", CCI_U_TYPE_BIGINT, MAX_LEN_BIGINT},
    {"MONETARY", CCI_U_TYPE_MONETARY, MAX_LEN_MONETARY},

    {"FLOAT", CCI_U_TYPE_FLOAT, MAX_LEN_FLOAT},
    {"DOUBLE", CCI_U_TYPE_DOUBLE, MAX_LEN_DOUBLE},

    {"DATE", CCI_U_TYPE_DATE, MAX_LEN_DATE},
    {"TIME", CCI_U_TYPE_TIME, MAX_LEN_TIME},
    {"DATETIME", CCI_U_TYPE_DATETIME, MAX_LEN_DATETIME},
    {"TIMESTAMP", CCI_U_TYPE_TIMESTAMP, MAX_LEN_TIMESTAMP},

    {"SET", CCI_U_TYPE_SET, MAX_LEN_SET},
    {"MULTISET", CCI_U_TYPE_MULTISET, MAX_LEN_MULTISET},
    {"SEQUENCE", CCI_U_TYPE_SEQUENCE, MAX_LEN_SEQUENCE},
    {"RESULTSET", CCI_U_TYPE_RESULTSET, -1},

    {"OBJECT", CCI_U_TYPE_OBJECT, MAX_LEN_OBJECT},
	{"BLOB", CCI_U_TYPE_BLOB, MAX_LEN_LOB},
	{"CLOB", CCI_U_TYPE_CLOB, MAX_LEN_LOB},
	{"ENUM", CCI_U_TYPE_ENUM, -1},
};

/************************************************************************
* PRIVATE FUNCTION PROTOTYPES
************************************************************************/

static int get_cubrid_u_type_len(T_CCI_U_TYPE type);
static int type2str(T_CCI_COL_INFO * column_info, char *type_name, int type_name_len);

static php_stream *cubrid_create_lob_stream(pdo_stmt_t *stmt, T_CCI_LOB lob, T_CCI_U_TYPE type);

static void free_cubrid_lob_list(pdo_cubrid_stmt *S);

static int cubrid_lob_new(int con_h_id, T_CCI_LOB *lob, T_CCI_U_TYPE type, T_CCI_ERROR *err_buf);
static int64_t cubrid_lob_size(T_CCI_LOB lob, T_CCI_U_TYPE type);
static int cubrid_lob_write(int con_h_id, T_CCI_LOB lob, T_CCI_U_TYPE type, int64_t start_pos, int length, const char *buf, T_CCI_ERROR *err_buf);
static int cubrid_lob_read(int con_h_id, T_CCI_LOB lob, T_CCI_U_TYPE type, int64_t start_pos, int length, char *buf, T_CCI_ERROR *err_buf);
static int cubrid_lob_free(T_CCI_LOB lob, T_CCI_U_TYPE type);

/************************************************************************
* IMPLEMENTATION OF CUBRID PDO
************************************************************************/

static int cubrid_stmt_dtor(pdo_stmt_t *stmt)
{
	pdo_cubrid_stmt *S = (pdo_cubrid_stmt *)stmt->driver_data;
	int i;

	if (!S) {
		return 1;
	}

	if (S->stmt_handle && S->H) {
		for (i = 0; i < S->H->stmt_count; i++) {
			if (S->H->stmt_list[i] == S) {
				S->H->stmt_list[i] = NULL;
			}
		}
	}

	if (S->l_bind) {
		efree(S->l_bind);
		S->l_bind = NULL;
	}

	if (S->param_info) {
		cci_param_info_free(S->param_info);
		S->param_info = NULL;
	}

	S->bind_num = 0;

	/* LOBs bound for an execute that failed are not released by EXEC_POST */
	free_cubrid_lob_list(S);

	if (S->stmt_handle) {
		cci_close_req_handle(S->stmt_handle);
		S->stmt_handle = 0;
	}

	efree(S);
	stmt->driver_data = NULL;

	return 1;
}

static int cubrid_stmt_execute(pdo_stmt_t *stmt)
{
	pdo_cubrid_stmt *S = (pdo_cubrid_stmt *)stmt->driver_data;

    T_CCI_COL_INFO *res_col_info;
    T_CCI_CUBRID_STMT res_sql_type;
    int res_col_count = 0;

	char exec_flag = CCI_EXEC_QUERY_ALL;

	int cubrid_retval = 0;
	long exec_ret = 0;
    T_CCI_ERROR error;
	int i;

	if (!S->stmt_handle) {
		pdo_cubrid_error_stmt(stmt, CUBRID_ER_INVALID_STMT_HANDLE, NULL, NULL);
		return 0;
	}

	if ((cubrid_retval = cci_fetch_buffer_clear(S->stmt_handle)) < 0) {
		pdo_cubrid_error_stmt(stmt, cubrid_retval, NULL, NULL);
		return 0;
	}

	if (!S->l_prepare) {
		pdo_cubrid_error_stmt(stmt, CUBRID_ER_NOT_PREPARED, NULL, NULL);
		return 0;
	}

	if (S->bind_num > 0) {
		if (!S->l_bind) {
			return 0;
		}

		for (i = 0; i < S->bind_num; i++) {
			if (!S->l_bind[i]) {
				pdo_cubrid_error_stmt(stmt, CUBRID_ER_PARAM_NOT_BIND, NULL, NULL);
				return 0;
			}
		}
	}

	exec_ret = cci_execute(S->stmt_handle, exec_flag, 0, &error);

	if (exec_ret < 0) {
		pdo_cubrid_error_stmt(stmt, exec_ret, &error, NULL);
		return 0;
    }

	res_col_info = cci_get_result_info(S->stmt_handle, &res_sql_type, &res_col_count);
    if (res_sql_type == CUBRID_STMT_SELECT && !res_col_info) {
		pdo_cubrid_error_stmt(stmt, CUBRID_ER_CANNOT_GET_COLUMN_INFO, NULL, NULL);
		return 0;
    }

	S->col_info = res_col_info;
	S->sql_type = res_sql_type;
	S->col_count = res_col_count;

	php_pdo_stmt_set_column_count(stmt, res_col_count);
	stmt->row_count = exec_ret;

	switch (S->sql_type) {
    case CUBRID_STMT_SELECT:
		S->row_count = exec_ret;
		break;
    case CUBRID_STMT_INSERT:
    case CUBRID_STMT_UPDATE:
    case CUBRID_STMT_DELETE:
		S->affected_rows = exec_ret;
		break;
    case CUBRID_STMT_CALL:
		S->row_count = exec_ret;
		break;
	default:
		break;
    }

	S->cursor_pos = 1;

	for (i = 0; i < S->bind_num; i++) {
		S->l_bind[i] = 0;
	}

	return 1;
}

/* Build a CCI set from a PHP array. Every element is bound as a string
 * (like the PHP 7 driver); the string "NULL" means a NULL element.
 * The array is NOT modified: it may be shared or immutable in PHP 8. */
static T_CCI_SET cubrid_create_set_by_param(zval *parameter)
{
    T_CCI_SET set = NULL;
    int count, i = 0;
    int *indicator;
    char **data_set;
    zend_string **str_set;
    zval *z_item;

    if (Z_TYPE_P(parameter) != IS_ARRAY) {
		return NULL;
    }

    count = zend_hash_num_elements(Z_ARRVAL_P(parameter));

    indicator = (int *) ecalloc(count + 1, sizeof(int));
    data_set = (char **) ecalloc(count + 1, sizeof(char *));
    str_set = (zend_string **) ecalloc(count + 1, sizeof(zend_string *));

    ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(parameter), z_item) {
		str_set[i] = zval_get_string(z_item);
		data_set[i] = ZSTR_VAL(str_set[i]);
		indicator[i] = (strcmp(data_set[i], "NULL") == 0) ? 1 : 0;
		i++;
    } ZEND_HASH_FOREACH_END();

    if (cci_set_make(&set, CCI_U_TYPE_STRING, count, data_set, indicator) < 0) {
		set = NULL;
    }

    for (i = 0; i < count; i++) {
		zend_string_release(str_set[i]);
    }

    efree(str_set);
    efree(indicator);
    efree(data_set);

    return set;
}

static int cubrid_stmt_param_hook(pdo_stmt_t *stmt, struct pdo_bound_param_data *param,
		enum pdo_param_event event_type)
{
    zval *parameter;

    pdo_cubrid_stmt *S = (pdo_cubrid_stmt *)stmt->driver_data;

    int bind_index;

    T_CCI_U_TYPE u_type;
    T_CCI_A_TYPE a_type;

    T_CCI_LOB lob = NULL;
    pdo_cubrid_lob *lob_node;

    php_stream *stm = NULL;
    zend_string *str = NULL;
    int own_stream = 0;
    char buf[CUBRID_LOB_READ_BUF_SIZE];
    uint64_t lob_start_pos = 0;
    ssize_t lob_read_size = 0;

    T_CCI_SET set = NULL;
    T_CCI_ERROR error;
    int cubrid_retval = 0;

    if (!S->stmt_handle) {
        pdo_cubrid_error_stmt(stmt, CUBRID_ER_INVALID_STMT_HANDLE, NULL, NULL);
        return 0;
    }

    if (!S->l_prepare || !param->is_param) {
        return 1;
    }

    switch (event_type) {
	case PDO_PARAM_EVT_EXEC_PRE:
		if (param->paramno < 0 || param->paramno >= S->bind_num) {
			pdo_cubrid_error_stmt(stmt, CUBRID_ER_INVALID_INDEX, NULL, NULL);
			return 0;
		}

		bind_index = param->paramno + 1;

		parameter = &param->parameter;
		ZVAL_DEREF(parameter);

		switch (PDO_PARAM_TYPE(param->param_type)) {
		case PDO_PARAM_INT:
			u_type = CCI_U_TYPE_INT;
			break;
		case PDO_PARAM_STR:
			u_type = CCI_U_TYPE_STRING;
			break;
		case PDO_PARAM_LOB:
			u_type = CCI_U_TYPE_BLOB;
			break;
		case PDO_PARAM_NULL:
			u_type = CCI_U_TYPE_NULL;
			break;
		case PDO_PARAM_STMT:
		default:
			pdo_cubrid_error_stmt(stmt, CUBRID_ER_NOT_SUPPORTED_TYPE, NULL, NULL);
			return 0;
		}

		if (Z_TYPE_P(parameter) == IS_ARRAY) {
			u_type = CCI_U_TYPE_SET;
		}

		if (u_type == CCI_U_TYPE_NULL || Z_TYPE_P(parameter) == IS_NULL) {
			cubrid_retval = cci_bind_param(S->stmt_handle, bind_index, CCI_A_TYPE_STR, NULL, u_type, 0);
		} else if (u_type == CCI_U_TYPE_BLOB || u_type == CCI_U_TYPE_CLOB) {
			a_type = (u_type == CCI_U_TYPE_BLOB) ? CCI_A_TYPE_BLOB : CCI_A_TYPE_CLOB;

			if (Z_TYPE_P(parameter) == IS_RESOURCE) {
				php_stream_from_zval_no_verify(stm, parameter);
				if (!stm) {
					php_error_docref(NULL, E_WARNING, "Expected a stream resource when param type is LOB");
					return 0;
				}
			} else {
				/* file name */
				str = zval_get_string(parameter);
				stm = php_stream_open_wrapper(ZSTR_VAL(str), "r", REPORT_ERRORS, NULL);
				zend_string_release(str);
				if (!stm) {
					return 0;
				}
				own_stream = 1;
			}

			if ((cubrid_retval = cubrid_lob_new(S->H->conn_handle, &lob, u_type, &error)) < 0) {
				pdo_cubrid_error_stmt(stmt, cubrid_retval, &error, NULL);
				if (own_stream) {
					php_stream_close(stm);
				}
				return 0;
			}

			while (!php_stream_eof(stm)) {
				lob_read_size = php_stream_read(stm, buf, CUBRID_LOB_READ_BUF_SIZE);
				if (lob_read_size <= 0) {
					break;
				}

				if ((cubrid_retval = cubrid_lob_write(S->H->conn_handle, lob, u_type,
								lob_start_pos, (int) lob_read_size, buf, &error)) < 0) {
					pdo_cubrid_error_stmt(stmt, cubrid_retval, &error, NULL);
					cubrid_lob_free(lob, u_type);
					if (own_stream) {
						php_stream_close(stm);
					}
					return 0;
				}

				lob_start_pos += lob_read_size;
			}

			if (own_stream) {
				php_stream_close(stm);
			}

			cubrid_retval = cci_bind_param(S->stmt_handle, bind_index, a_type, (void *) lob, u_type, CCI_BIND_PTR);

			/* CCI_BIND_PTR: the lob must live until the statement is executed */
			lob_node = (pdo_cubrid_lob *) emalloc(sizeof(pdo_cubrid_lob));
			lob_node->lob = lob;
			lob_node->type = u_type;
			lob_node->next = S->lob;
			S->lob = lob_node;
		} else if (u_type == CCI_U_TYPE_SET) {
			if ((set = cubrid_create_set_by_param(parameter)) == NULL) {
				pdo_cubrid_error_stmt(stmt, CUBRID_ER_INVALID_PARAM, NULL, NULL);
				return 0;
			}

			cubrid_retval = cci_bind_param(S->stmt_handle, bind_index, CCI_A_TYPE_SET, set, u_type, 0);
			cci_set_free(set);
		} else {
			/* do not convert the bound variable itself, it may be a typed reference */
			str = zval_get_string(parameter);
			cubrid_retval = cci_bind_param(S->stmt_handle, bind_index, CCI_A_TYPE_STR, ZSTR_VAL(str), u_type, 0);
			zend_string_release(str);
		}

		if (cubrid_retval != 0 || !S->l_bind) {
			pdo_cubrid_error_stmt(stmt, cubrid_retval, NULL, NULL);
			return 0;
		}

		S->l_bind[param->paramno] = 1;

		break;
	case PDO_PARAM_EVT_EXEC_POST:
		free_cubrid_lob_list(S);
		break;
	case PDO_PARAM_EVT_ALLOC:
	case PDO_PARAM_EVT_FREE:
	case PDO_PARAM_EVT_FETCH_PRE:
	case PDO_PARAM_EVT_FETCH_POST:
	case PDO_PARAM_EVT_NORMALIZE:
		break;
	default:
		break;
	}

	return 1;
}

static int cubrid_stmt_fetch(pdo_stmt_t *stmt, enum pdo_fetch_orientation ori, zend_long offset)
{
	pdo_cubrid_stmt *S = (pdo_cubrid_stmt *)stmt->driver_data;
	T_CCI_CURSOR_POS origin;

	long abs_offset = 0;
	int cubrid_retval = 0;
    T_CCI_ERROR error;

	if (!S->stmt_handle) {
		pdo_cubrid_error_stmt(stmt, CUBRID_ER_INVALID_STMT_HANDLE, NULL, NULL);
		return 0;
	}

	switch (ori) {
	case PDO_FETCH_ORI_FIRST:
		origin = CCI_CURSOR_FIRST;
		offset = 1;

		abs_offset = 1;
		break;
	case PDO_FETCH_ORI_LAST:
		origin = CCI_CURSOR_LAST;
		offset = 1;

		if (S->cursor_type == PDO_CURSOR_FWDONLY) {
			abs_offset = S->row_count;
		} else {
			/* -1 mean the last */
			abs_offset = -1;
		}
		break;
	case PDO_FETCH_ORI_NEXT:
		origin = CCI_CURSOR_CURRENT;
		offset = 1;

		abs_offset = S->cursor_pos + 1;
		break;
	case PDO_FETCH_ORI_PRIOR:
		origin = CCI_CURSOR_CURRENT;
		offset = -1;

		abs_offset = S->cursor_pos - 1;
		break;
	case PDO_FETCH_ORI_ABS:
		origin = CCI_CURSOR_FIRST;

		abs_offset = offset;
		break;
	case PDO_FETCH_ORI_REL:
		origin = CCI_CURSOR_CURRENT;

		abs_offset = S->cursor_pos + offset;
		break;
	default:
		return 0;
	}

	if (S->cursor_type == PDO_CURSOR_FWDONLY && abs_offset != -1 && abs_offset < S->cursor_pos) {
		pdo_cubrid_error_stmt(stmt, CUBRID_ER_INVALID_CURSOR_POS, NULL, NULL);
		return 0;
	}

	cubrid_retval = cci_cursor(S->stmt_handle, (int) offset, origin, &error);
    if (cubrid_retval == CCI_ER_NO_MORE_DATA) {
		return 0;
    }

	if (cubrid_retval < 0) {
		pdo_cubrid_error_stmt(stmt, cubrid_retval, &error, NULL);
		return 0;
	}

	S->cursor_pos = abs_offset;

	cubrid_retval = cci_fetch(S->stmt_handle, &error);
	if (cubrid_retval < 0) {
		pdo_cubrid_error_stmt(stmt, cubrid_retval, &error, NULL);
		return 0;
	}

	return 1;
}

/* PHP 8.1 removed pdo_column_data.param_type: LOB columns are found from the column info */
static int cubrid_col_is_lob(pdo_cubrid_stmt *S, int colno)
{
	T_CCI_U_TYPE type = CCI_GET_COLLECTION_DOMAIN(CCI_GET_RESULT_INFO_TYPE(S->col_info, colno + 1));

	return (type == CCI_U_TYPE_BLOB || type == CCI_U_TYPE_CLOB);
}

static int cubrid_stmt_describe_col(pdo_stmt_t *stmt, int colno)
{
	pdo_cubrid_stmt *S = (pdo_cubrid_stmt *)stmt->driver_data;
	struct pdo_column_data *cols = stmt->columns;
    char* str;

    T_CCI_U_TYPE type;
	long type_maxlen;

	if (!S->col_info) {
		pdo_cubrid_error_stmt(stmt, CUBRID_ER_CANNOT_GET_COLUMN_INFO, NULL, NULL);
		return 0;
	}

	if (S->sql_type != CUBRID_STMT_SELECT) {
		pdo_cubrid_error_stmt(stmt, CUBRID_ER_INVALID_SQL_TYPE, NULL, NULL);
		return 0;
	}

	if (colno < 0 || colno > S->col_count - 1) {
		pdo_cubrid_error_stmt(stmt, CUBRID_ER_INVALID_INDEX, NULL, NULL);
		return 0;
	}

    str = CCI_GET_RESULT_INFO_NAME (S->col_info, colno + 1);
	cols[colno].name = zend_string_init(str, strlen(str), 0);
	cols[colno].precision = CCI_GET_RESULT_INFO_PRECISION (S->col_info, colno + 1);

	type = CCI_GET_COLLECTION_DOMAIN(CCI_GET_RESULT_INFO_TYPE(S->col_info, colno + 1));
	if ((type_maxlen = get_cubrid_u_type_len(type)) == -1) {
		type_maxlen = CCI_GET_RESULT_INFO_PRECISION(S->col_info, colno + 1);
		if (type == CCI_U_TYPE_NUMERIC) {
			type_maxlen += 2; /* "," + "-" */
		}
    }

	if (CCI_IS_COLLECTION_TYPE(CCI_GET_RESULT_INFO_TYPE(S->col_info, colno + 1))) {
		type_maxlen = MAX_LEN_SET;
    }

	cols[colno].maxlen = type_maxlen;

	return 1;
}

#define ADD_ASSOC_STRING_SAFE(arr, key, s)	add_assoc_string((arr), (key), (s) ? (s) : "")

static int cubrid_stmt_get_column_meta(pdo_stmt_t *stmt, zend_long colno, zval *return_value)
{
	pdo_cubrid_stmt *S = (pdo_cubrid_stmt*)stmt->driver_data;

    char type_name[128];

	if (!S->col_info) {
		return FAILURE;
	}

	if (colno < 0 || colno >= S->col_count) {
		pdo_cubrid_error_stmt(stmt, CUBRID_ER_INVALID_INDEX, NULL, NULL);
		return FAILURE;
	}

	array_init(return_value);

	if (type2str(&S->col_info[colno], type_name, sizeof(type_name)) == SUCCESS) {
		add_assoc_string(return_value, "type", (char *)type_name);
	}

	ADD_ASSOC_STRING_SAFE(return_value, "name", S->col_info[colno].col_name);
    ADD_ASSOC_STRING_SAFE(return_value, "table", S->col_info[colno].class_name);
    ADD_ASSOC_STRING_SAFE(return_value, "def", S->col_info[colno].default_value);

    add_assoc_long(return_value, "precision", S->col_info[colno].precision);
    add_assoc_long(return_value, "scale", S->col_info[colno].scale);

    add_assoc_long(return_value, "not_null", S->col_info[colno].is_non_null);
    add_assoc_long(return_value, "auto_increment", S->col_info[colno].is_auto_increment);
    add_assoc_long(return_value, "unique_key", S->col_info[colno].is_unique_key);
    add_assoc_long(return_value, "multiple_key", !S->col_info[colno].is_unique_key);
    add_assoc_long(return_value, "primary_key", S->col_info[colno].is_primary_key);
    add_assoc_long(return_value, "foreign_key", S->col_info[colno].is_foreign_key);
    add_assoc_long(return_value, "reverse_index", S->col_info[colno].is_reverse_index);
    add_assoc_long(return_value, "reverse_unique", S->col_info[colno].is_reverse_unique);

	/* PDO adds "pdo_type" by itself only up to PHP 8.0. "len" is overwritten by PDO,
	 * it is added here to keep the key order of the PHP 7 driver (len, pdo_type) */
	add_assoc_long(return_value, "len", stmt->columns ? stmt->columns[colno].maxlen : 0);
	add_assoc_long(return_value, "pdo_type", cubrid_col_is_lob(S, (int) colno) ? PDO_PARAM_LOB : PDO_PARAM_STR);

	return SUCCESS;
}

static int cubrid_stmt_get_col_data(pdo_stmt_t *stmt, int colno, zval *result, enum pdo_param_type *type)
{
	pdo_cubrid_stmt *S = (pdo_cubrid_stmt *)stmt->driver_data;

	T_CCI_U_TYPE u_type;
	T_CCI_A_TYPE a_type;
	T_CCI_LOB lob = NULL;
	php_stream *stm;

	int cubrid_retval = 0;
	char *res_buf = NULL;
    int ind = 0;

	if (!S->stmt_handle) {
		pdo_cubrid_error_stmt(stmt, CUBRID_ER_INVALID_STMT_HANDLE, NULL, NULL);
		return 0;
	}

	if (colno < 0 || colno >= S->col_count) {
		pdo_cubrid_error_stmt(stmt, CUBRID_ER_INVALID_INDEX, NULL, NULL);
		return 0;
	}

	if (!cubrid_col_is_lob(S, colno)) {
		if ((cubrid_retval = cci_get_data(S->stmt_handle, colno + 1, CCI_A_TYPE_STR, &res_buf, &ind)) < 0) {
			pdo_cubrid_error_stmt(stmt, cubrid_retval, NULL, NULL);
			return 0;
		}

		if (ind < 0) {
			ZVAL_NULL(result);
		} else {
			ZVAL_STRINGL(result, res_buf, ind);
		}

		return 1;
	}

	u_type = CCI_GET_RESULT_INFO_TYPE(S->col_info, colno + 1);

	if (u_type == CCI_U_TYPE_BLOB) {
		a_type = CCI_A_TYPE_BLOB;
	} else if (u_type == CCI_U_TYPE_CLOB) {
		a_type = CCI_A_TYPE_CLOB;
	} else {
		ZVAL_NULL(result);
		return 0;
	}

	if ((cubrid_retval = cci_get_data(S->stmt_handle, colno + 1, a_type, (void *) &lob, &ind)) < 0) {
		pdo_cubrid_error_stmt(stmt, cubrid_retval, NULL, NULL);
		return 0;
	}

	if (ind < 0) {
		ZVAL_NULL(result);
		return 1;
	}

	/* PDO reads the stream into a string if PDO::PARAM_STR is requested */
	if ((stm = cubrid_create_lob_stream(stmt, lob, u_type)) == NULL) {
		cubrid_lob_free(lob, u_type);
		return 0;
	}

	php_stream_to_zval(stm, result);

	return 1;
}

static int cubrid_stmt_next_rowset(pdo_stmt_t *stmt)
{
	pdo_cubrid_stmt *S = (pdo_cubrid_stmt*)stmt->driver_data;

    T_CCI_COL_INFO *res_col_info;
    T_CCI_CUBRID_STMT res_sql_type;
    int res_col_count = 0;

	int exec_ret = 0;
	T_CCI_ERROR error;

	if (!S->stmt_handle) {
		pdo_cubrid_error_stmt(stmt, CUBRID_ER_INVALID_STMT_HANDLE, NULL, NULL);
		return 0;
	}

	exec_ret = cci_next_result(S->stmt_handle, &error);
	if (exec_ret == CAS_ER_NO_MORE_RESULT_SET) {
		return 0;
	}

	if (exec_ret < 0) {
		pdo_cubrid_error_stmt(stmt, exec_ret, &error, NULL);
		return 0;
	}

	res_col_info = cci_get_result_info(S->stmt_handle, &res_sql_type, &res_col_count);
    if (res_sql_type == CUBRID_STMT_SELECT && !res_col_info) {
		pdo_cubrid_error_stmt(stmt, CUBRID_ER_CANNOT_GET_COLUMN_INFO, NULL, NULL);
		return 0;
    }

	S->col_info = res_col_info;
	S->sql_type = res_sql_type;
	S->col_count = res_col_count;

	php_pdo_stmt_set_column_count(stmt, res_col_count);
	stmt->row_count = exec_ret;

	switch (S->sql_type) {
    case CUBRID_STMT_SELECT:
		S->row_count = exec_ret;
		break;
    case CUBRID_STMT_INSERT:
    case CUBRID_STMT_UPDATE:
    case CUBRID_STMT_DELETE:
		S->affected_rows = exec_ret;
		break;
    case CUBRID_STMT_CALL:
		S->row_count = exec_ret;
		break;
	default:
		break;
    }

	S->cursor_pos = 1;

	return 1;
}

const struct pdo_stmt_methods cubrid_stmt_methods = {
    cubrid_stmt_dtor,
	cubrid_stmt_execute,
	cubrid_stmt_fetch,
	cubrid_stmt_describe_col,
	cubrid_stmt_get_col_data,
	cubrid_stmt_param_hook,
	NULL, /* set attr */
	NULL, /* get attr */
    cubrid_stmt_get_column_meta, /* get column meta */
	cubrid_stmt_next_rowset,
	NULL, /* cursor closer */
};

/************************************************************************
* PRIVATE FUNCTIONS IMPLEMENTATION
************************************************************************/

static int get_cubrid_u_type_len(T_CCI_U_TYPE type)
{
    int i;
    int size = sizeof(db_type_info)/sizeof(db_type_info[0]);
    DB_TYPE_INFO type_info;

    for (i = 0; i < size; i++) {
		type_info = db_type_info[i];
		if (type == type_info.cubrid_u_type) {
			return type_info.len;
		}
    }

    return 0;
}

struct cubrid_lob_stream_self {
	pdo_stmt_t *stmt;
	pdo_cubrid_stmt *S;
	T_CCI_LOB lob;
	T_CCI_U_TYPE type;
	uint64_t offset;
	uint64_t size;
};

static ssize_t cubrid_lob_stream_write(php_stream *stream, const char *buf, size_t count)
{
	return 0;
}

static ssize_t cubrid_lob_stream_read(php_stream *stream, char *buf, size_t count)
{
	struct cubrid_lob_stream_self *self = (struct cubrid_lob_stream_self*)stream->abstract;

	T_CCI_ERROR error;
	int read_len;
	int cubrid_retval = 0;

	/* the connection was closed while the stream is still open */
	if (!self->S || !self->S->H || !self->S->H->conn_handle) {
		stream->eof = 1;
		return -1;
	}

	if ((self->offset + count) < self->size) {
		read_len = count;
	} else {
		read_len = self->size - self->offset;
		stream->eof = 1;
	}

	if (read_len) {
		cubrid_retval = cubrid_lob_read(self->S->H->conn_handle, self->lob, self->type, self->offset, read_len, buf, &error);
		if (cubrid_retval < 0) {
			return -1;
		} else {
			self->offset += cubrid_retval;
		}
	}

	return (ssize_t)cubrid_retval;
}

static int cubrid_lob_stream_close(php_stream *stream, int close_handle)
{
	struct cubrid_lob_stream_self *self = (struct cubrid_lob_stream_self*)stream->abstract;
	zend_object *obj = &self->stmt->std;

	if (close_handle) {
		cubrid_lob_free(self->lob, self->type);
	}
	efree(self);

	/* drop the statement reference taken in cubrid_create_lob_stream() */
	OBJ_RELEASE(obj);

	return 0;
}

static int cubrid_lob_stream_flush(php_stream *stream)
{
	return 0;
}

static const php_stream_ops cubrid_lob_stream_ops = {
	cubrid_lob_stream_write,
	cubrid_lob_stream_read,
	cubrid_lob_stream_close,
	cubrid_lob_stream_flush,
	"pdo_cubrid lob stream",
	NULL, /* seek */
	NULL, /* cast */
	NULL, /* stat */
	NULL /* set_option */
};

static int type2str(T_CCI_COL_INFO * column_info, char *type_name, int type_name_len)
{
    char buf[64];

    switch (CCI_GET_COLLECTION_DOMAIN(column_info->ext_type)) {
    case CCI_U_TYPE_UNKNOWN:
		snprintf(buf, sizeof(buf), "unknown");
	break;
    case CCI_U_TYPE_CHAR:
		snprintf(buf, sizeof(buf), "char(%d)", column_info->precision);
	break;
    case CCI_U_TYPE_STRING:
		snprintf(buf, sizeof(buf), "varchar(%d)", column_info->precision);
	break;
    case CCI_U_TYPE_NCHAR:
		snprintf(buf, sizeof(buf), "nchar(%d)", column_info->precision);
	break;
    case CCI_U_TYPE_VARNCHAR:
		snprintf(buf, sizeof(buf), "varnchar(%d)", column_info->precision);
	break;
    case CCI_U_TYPE_BIT:
		snprintf(buf, sizeof(buf), "bit");
	break;
    case CCI_U_TYPE_VARBIT:
		snprintf(buf, sizeof(buf), "varbit(%d)", column_info->precision);
	break;
    case CCI_U_TYPE_NUMERIC:
		snprintf(buf, sizeof(buf), "numeric(%d,%d)", column_info->precision, column_info->scale);
	break;
    case CCI_U_TYPE_INT:
		snprintf(buf, sizeof(buf), "integer");
	break;
    case CCI_U_TYPE_SHORT:
		snprintf(buf, sizeof(buf), "smallint");
	break;
    case CCI_U_TYPE_MONETARY:
		snprintf(buf, sizeof(buf), "monetary");
	break;
    case CCI_U_TYPE_FLOAT:
		snprintf(buf, sizeof(buf), "float");
	break;
    case CCI_U_TYPE_DOUBLE:
		snprintf(buf, sizeof(buf), "double");
	break;
    case CCI_U_TYPE_DATE:
		snprintf(buf, sizeof(buf), "date");
	break;
    case CCI_U_TYPE_TIME:
		snprintf(buf, sizeof(buf), "time");
	break;
    case CCI_U_TYPE_TIMESTAMP:
		snprintf(buf, sizeof(buf), "timestamp");
	break;
    case CCI_U_TYPE_SET:
		snprintf(buf, sizeof(buf), "set");
	break;
    case CCI_U_TYPE_MULTISET:
		snprintf(buf, sizeof(buf), "multiset");
	break;
    case CCI_U_TYPE_SEQUENCE:
		snprintf(buf, sizeof(buf), "sequence");
	break;
    case CCI_U_TYPE_OBJECT:
		snprintf(buf, sizeof(buf), "object");
	break;
    case CCI_U_TYPE_BIGINT:
		snprintf(buf, sizeof(buf), "bigint");
	break;
    case CCI_U_TYPE_DATETIME:
		snprintf(buf, sizeof(buf), "datetime");
	break;
    case CCI_U_TYPE_BLOB:
        snprintf(buf, sizeof(buf), "blob");
        break;
    case CCI_U_TYPE_CLOB:
        snprintf(buf, sizeof(buf), "clob");
        break;
	case CCI_U_TYPE_ENUM:
		snprintf(buf, sizeof(buf), "enum");
		break;
    default:
		/* should not enter here */
		snprintf(buf, sizeof(buf), "[unknown]");
		return FAILURE;
    }

    if (CCI_IS_SET_TYPE(column_info->ext_type)) {
		snprintf(type_name, type_name_len, "set(%s)", buf);
    } else if (CCI_IS_MULTISET_TYPE(column_info->ext_type)) {
		snprintf(type_name, type_name_len, "multiset(%s)", buf);
    } else if (CCI_IS_SEQUENCE_TYPE(column_info->ext_type)) {
		snprintf(type_name, type_name_len, "sequence(%s)", buf);
    } else {
		snprintf(type_name, type_name_len, "%s", buf);
    }

    return SUCCESS;
}

static php_stream *cubrid_create_lob_stream(pdo_stmt_t *stmt, T_CCI_LOB lob, T_CCI_U_TYPE type)
{
	php_stream *stm;
	struct cubrid_lob_stream_self *self = ecalloc(1, sizeof(struct cubrid_lob_stream_self));

	self->lob = lob;
	self->type = type;
	self->offset = 0;
	self->size = cubrid_lob_size(lob, type);
	self->stmt = stmt;
	self->S = (pdo_cubrid_stmt *)stmt->driver_data;

	stm = php_stream_alloc(&cubrid_lob_stream_ops, self, 0, "rb");
	if (stm) {
		/* the stream reads through the statement: keep it alive */
		GC_ADDREF(&stmt->std);
		return stm;
	}

	efree(self);
	return NULL;
}

static void free_cubrid_lob_list(pdo_cubrid_stmt *S)
{
	pdo_cubrid_lob *node, *next;

	for (node = S->lob; node; node = next) {
		next = node->next;
		if (node->lob) {
			cubrid_lob_free(node->lob, node->type);
		}
		efree(node);
	}

	S->lob = NULL;
}

static int cubrid_lob_new(int con_h_id, T_CCI_LOB *lob, T_CCI_U_TYPE type, T_CCI_ERROR *err_buf)
{
    return (type == CCI_U_TYPE_BLOB) ?
        cci_blob_new(con_h_id, lob, err_buf) : cci_clob_new(con_h_id, lob, err_buf);
}

static int64_t cubrid_lob_size(T_CCI_LOB lob, T_CCI_U_TYPE type)
{
    return (type == CCI_U_TYPE_BLOB) ? cci_blob_size(lob) : cci_clob_size(lob);
}

static int cubrid_lob_write(int con_h_id, T_CCI_LOB lob, T_CCI_U_TYPE type, int64_t start_pos, int length, const char *buf, T_CCI_ERROR *err_buf)
{
    return (type == CCI_U_TYPE_BLOB) ?
        cci_blob_write(con_h_id, lob, start_pos, length, buf, err_buf) :
        cci_clob_write(con_h_id, lob, start_pos, length, buf, err_buf);
}

static int cubrid_lob_read(int con_h_id, T_CCI_LOB lob, T_CCI_U_TYPE type, int64_t start_pos, int length, char *buf, T_CCI_ERROR *err_buf)
{
    return (type == CCI_U_TYPE_BLOB) ?
        cci_blob_read(con_h_id, lob, start_pos, length, buf, err_buf) :
        cci_clob_read(con_h_id, lob, start_pos, length, buf, err_buf);
}

static int cubrid_lob_free(T_CCI_LOB lob, T_CCI_U_TYPE type)
{
    return (type == CCI_U_TYPE_BLOB) ?
        cci_blob_free(lob) : cci_clob_free(lob);
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=manual
 * vim<600: noet sw=4 ts=4
 */
