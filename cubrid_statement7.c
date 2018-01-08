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

static int get_cubrid_u_type_by_name(const char *type_name);
static int get_cubrid_u_type_len(T_CCI_U_TYPE type);
static int type2str(T_CCI_COL_INFO * column_info, char *type_name, int type_name_len);

static php_stream *cubrid_create_lob_stream(pdo_stmt_t *stmt, T_CCI_LOB lob, T_CCI_U_TYPE type TSRMLS_DC);

static pdo_cubrid_lob *new_cubrid_lob(void);

static int cubrid_lob_new(int con_h_id, T_CCI_LOB *lob, T_CCI_U_TYPE type, T_CCI_ERROR *err_buf);
static pdo_int64_t cubrid_lob_size(T_CCI_LOB lob, T_CCI_U_TYPE type);
static int cubrid_lob_write(int con_h_id, T_CCI_LOB lob, T_CCI_U_TYPE type, pdo_int64_t start_pos, int length, const char *buf, T_CCI_ERROR *err_buf);
static int cubrid_lob_read(int con_h_id, T_CCI_LOB lob, T_CCI_U_TYPE type, pdo_int64_t start_pos, int length, char *buf, T_CCI_ERROR *err_buf);
static int cubrid_lob_free(T_CCI_LOB lob, T_CCI_U_TYPE type);

/************************************************************************
* IMPLEMENTATION OF CUBRID PDO
************************************************************************/

static int cubrid_stmt_dtor(pdo_stmt_t *stmt TSRMLS_DC)
{
	pdo_cubrid_stmt *S = (pdo_cubrid_stmt *)stmt->driver_data;
	int i;

	if (S->stmt_handle) {
		for (i = 0; i < S->H->stmt_count; i++) {
			if (!S->H->stmt_list[i]) {
				continue;	
			}

			if (S->H->stmt_list[i]->stmt_handle == S->stmt_handle) {
				S->H->stmt_list[i] = NULL;
			}
		}

		if (S->bind_num > 0) {
			if (S->l_bind) {
				efree(S->l_bind);
				S->l_bind = NULL;
			}

			if (S->param_info) {
				cci_param_info_free(S->param_info);
				S->param_info = NULL;
			}

			S->bind_num = 0;
		}
		
		if (S->stmt_handle) {
			cci_close_req_handle(S->stmt_handle);
		}

		S->stmt_handle = 0;
	}
	
	efree(S);
	stmt->driver_data = NULL;
	
	return 1;
}

static int cubrid_stmt_execute(pdo_stmt_t *stmt TSRMLS_DC)
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

	stmt->column_count = res_col_count;
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

static char* _cubrid_dup_buf(char* src_buf,int size)
{
    int len=0;
    char* temp_buf=NULL;

    if(src_buf != NULL)
    {
        len = strlen(src_buf);
    }
    else
    {
        len =size;
    }
    if(len<=0)
    {
        return NULL;
    }
    temp_buf = (char*)emalloc(len+1);
    if(NULL==temp_buf)
    {
      return NULL;
    }
    memset(temp_buf,0,len+1);
    if(NULL!= src_buf)
        memcpy(temp_buf,src_buf,len);   

    return temp_buf;
}

static char* _cubrid_get_data_buf(int type,int num)
{
    switch(type)
    {
        case CCI_U_TYPE_BIGINT:
            return _cubrid_dup_buf(NULL,sizeof(CUBRID_LONG_LONG)* (num+1));
        case CCI_U_TYPE_FLOAT:
            return  _cubrid_dup_buf(NULL,sizeof(float)* (num+1));
        case CCI_U_TYPE_DOUBLE:     
           return  _cubrid_dup_buf(NULL,sizeof(double)* (num+1));
        case CCI_U_TYPE_BIT:
        case CCI_U_TYPE_VARBIT:      
           return  _cubrid_dup_buf(NULL,sizeof(T_CCI_BIT)* (num+1));
        case CCI_U_TYPE_DATE:
        case CCI_U_TYPE_TIME:
        case CCI_U_TYPE_TIMESTAMP:
        case CCI_U_TYPE_DATETIME:
            return _cubrid_dup_buf(NULL,sizeof(T_CCI_DATE)* (num+1));  
        case CCI_U_TYPE_INT:
        case CCI_U_TYPE_SHORT:
            return _cubrid_dup_buf(NULL,sizeof(int)* (num+1));  
        default:
           return  _cubrid_dup_buf(NULL,sizeof(void*)* (num+1));
    } 
}

static char* cubrid_str2bit(char* str)
{
    int i=0,len=0,t=0;
    char* buf=NULL;
    int shift = 8;

    if(str == NULL)
        return NULL;
    len = strlen(str);

    if(0 == len%shift)
        t =1;

    buf = (char*)emalloc(len/shift+1+1);
    memset(buf,0,len/shift+1+1);

    for(i=0;i<len;i++)
    {
        if(str[len-i-1] == '1')
        {
            buf[len/shift - i/shift-t] |= (1<<(i%shift)); 
        }
        else if(str[len-i-1] == '0')
        {
        	//nothing
        }
        else
        {
            return NULL;
        }
    }
    return buf;
}

static int cubrid_data_convert(void* data ,void** pSet,int type,int* indicator)
{
    T_CCI_BIT *bit_value = NULL;
    T_CCI_DATE date = { 0, 0, 0, 0, 0, 0, 0 };
    int err_code = 0;
    
    if(data == NULL)
    {
        php_printf("cubrid_data_convert failed.data=%s<br/>",(char*)data);
        return 0;
    }
    if(strcmp(data,"NULL")==0)
    {
        *indicator=1;
    }  
    else
    {
        *indicator=0;
    } 
    
    switch(type)
    {
        case CCI_U_TYPE_BIT:
        case CCI_U_TYPE_VARBIT:  
            if(*indicator==1)
                break;
            
            bit_value = (T_CCI_BIT*)(pSet);
            bit_value[0].buf = (char*)cubrid_str2bit((char*)data);
            bit_value[0].size = strlen(bit_value[0].buf );          
/*         
        case CCI_U_TYPE_DATE:
            if(*indicator==1)
                break;
            err_code = ut_str_to_date ((char*)data, &date);
            if(err_code<0)
                return 0;
            memcpy((*pSet),&date,sizeof(T_CCI_DATE));
            break;
*/
        default:
            *pSet = data;
            return CCI_U_TYPE_STRING;
    }

    return type;
}
static T_CCI_SET cubrid_create_set_by_param(zval *parameter,int type)
{
    T_CCI_SET set=NULL;
    zval *z_array=0; 
    int z_array_count=0,i=0,set_type;
    int* indicator= NULL;     
    char** data_set=NULL;
    zval *z_item;
    T_CCI_BIT* bit;

    if(Z_TYPE_P(parameter) != IS_ARRAY)
    {
       php_printf("parameter not ARRAY.<br/>");
       return NULL;
    }

    z_array=parameter;
    z_array_count= zend_hash_num_elements(Z_ARRVAL_P(z_array));              

    indicator= (int*)_cubrid_dup_buf(NULL,sizeof(int)* (z_array_count+1));
    data_set = (char**)_cubrid_get_data_buf(type,z_array_count+1);

    if(indicator == NULL || data_set==NULL)
    {
        php_printf("malloc failed.<br/>");
        return NULL;
    }
    zend_hash_internal_pointer_reset(Z_ARRVAL_P(z_array)); 
    for (i = 0; i < z_array_count; i ++) 
    {
        // 获取当前数据
        z_item = zend_hash_get_current_data(Z_ARRVAL_P(z_array));
        convert_to_string_ex(z_item); 
        set_type = cubrid_data_convert(Z_STRVAL_P(z_item),(void**)&data_set[i] ,type,indicator+i);
        if(set_type ==0)
        {
            efree(indicator);
            efree(data_set);
            php_printf("cubrid_data_convert failed.i=%d<br/>",i);
            return 0;            
        }

         // 将数组中的内部指针向前移动一位
        zend_hash_move_forward(Z_ARRVAL_P(z_array));
    }   

    if(0> cci_set_make(&set, set_type, z_array_count, data_set, (int*)indicator))
    {
        efree(indicator);
        efree(data_set);
        php_printf("cci_set_make failed.<br/>");
        return 0;
    }
    if(CCI_U_TYPE_BIT == type || CCI_U_TYPE_VARBIT == type)
    {
        for(i=0;i<z_array_count;i++)
        {  
            if(indicator[i]==1)
             continue;      
            if(data_set[i] != NULL)
            {
                bit= (T_CCI_BIT*)&data_set[i];
                efree(bit->buf);
            }
        }    
    }
    efree(indicator);
    efree(data_set);    
    
    return set;
}
static int cubrid_type_pdo2cubrid(int pdo_type)
{
    switch (pdo_type) 
    {   
        case PDO_PARAM_LOB:
            return CCI_U_TYPE_BIT;
        default:
            return CCI_U_TYPE_STRING;
    }
}
static int cubrid_stmt_datatype_convert(int type)
{
    int u_type;
    switch (type) 
    {
        case PDO_PARAM_INT:
            u_type = CCI_U_TYPE_INT;

            break;
        case PDO_PARAM_STR:
#if PDO_DRIVER_API >= 20080721
        case PDO_PARAM_ZVAL:
#endif        
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
            return CCI_U_TYPE_UNKNOWN;
    }
    return u_type;
}

static int cubrid_stmt_param_hook(pdo_stmt_t *stmt, struct pdo_bound_param_data *param, 
		enum pdo_param_event event_type TSRMLS_DC)
{
    zval *parameter;

    pdo_cubrid_stmt *S = (pdo_cubrid_stmt *)stmt->driver_data;

    char *bind_value = NULL, *bind_value_type = NULL;
    int bind_value_len, bind_index,i=0;

    T_CCI_U_TYPE u_type;
    T_CCI_U_TYPE e_type;//element'datatype of set 
    T_CCI_A_TYPE a_type;

    T_CCI_BIT *bit_value = NULL;
    T_CCI_LOB lob = NULL;

    php_stream *stm = NULL;
    char* lobfile_name = NULL;
    char buf[CUBRID_LOB_READ_BUF_SIZE];
    pdo_uint64_t lob_start_pos = 0;
    size_t lob_read_size = 0;
    
    T_CCI_SET set;   
    T_CCI_ERROR error;
    int cubrid_retval = 0;

    if (!S->stmt_handle) 
    {
        pdo_cubrid_error_stmt(stmt, CUBRID_ER_INVALID_STMT_HANDLE, NULL, NULL);
        return 0;
    }

    if (!S->l_prepare || !param->is_param) 
    {
        return 1;	
    }

    switch (event_type) 
    {
        case PDO_PARAM_EVT_EXEC_PRE:
            if (param->paramno < 0 || param->paramno >= S->bind_num) 
            {
                pdo_cubrid_error_stmt(stmt, CUBRID_ER_INVALID_INDEX, NULL, NULL);
                return 0;
            }

            bind_index = param->paramno + 1;

			if (Z_ISREF(param->parameter)) {
				parameter = Z_REFVAL(param->parameter);
			}
			else {
				parameter = &param->parameter;
            }
            /* driver_params: cubrid data type name (string), pass by driver_options */

            /* if driver_params is null, use param->param_type */ 
            switch (param->param_type) 
            {
                case PDO_PARAM_INT:
                	u_type = CCI_U_TYPE_INT;

                		break;
                	case PDO_PARAM_STR:
#if PDO_DRIVER_API >= 20080721
                	case PDO_PARAM_ZVAL:
#endif        
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

            if(Z_TYPE_P(parameter) == IS_ARRAY)
            {
                e_type = cubrid_type_pdo2cubrid(u_type);
                u_type = CCI_U_TYPE_SET;                    
            }

            if (u_type == CCI_U_TYPE_NULL || Z_TYPE_P(parameter) == IS_NULL) 
            {
                cubrid_retval = cci_bind_param(S->stmt_handle, bind_index, CCI_A_TYPE_STR, NULL, u_type, 0);
            } 
            else
            {
                if (u_type == CCI_U_TYPE_BLOB || u_type == CCI_U_TYPE_CLOB) 
                {
                    if (Z_TYPE_P(parameter) == IS_RESOURCE)
                    {
                        php_stream_from_zval_no_verify(stm, parameter);
                        if (!stm) 
                        {
                        	php_error_docref(NULL TSRMLS_CC, E_WARNING, "Expected a stream resource when param type is LOB\n");
                        	return 0;
                        }
                    } 
                    else
                    {
                        /* file name */
                        convert_to_string(parameter);
                        lobfile_name = Z_STRVAL_P(parameter);

                        if (!(stm = php_stream_open_wrapper(lobfile_name, "r", REPORT_ERRORS, NULL))) 
                        {
                        	return 0;
                        }
                    }
                } 
                else if(u_type == CCI_U_TYPE_SET)
                {
                    set = cubrid_create_set_by_param(parameter, e_type);
                    if(set == NULL)
                   {
                       return 0;
                   }    
                }
                else 
                {
            		convert_to_string(parameter);
                }

                switch (u_type) 
                {
                    case CCI_U_TYPE_BLOB:
                    		a_type = CCI_A_TYPE_BLOB;
                    		break;
                    case CCI_U_TYPE_CLOB:
                    		a_type = CCI_A_TYPE_CLOB;
                    		break;
                    case CCI_U_TYPE_BIT:
                    case CCI_U_TYPE_VARBIT:
                    		a_type = CCI_A_TYPE_BIT;
                    		break;
                    case CCI_U_TYPE_SET:
                    case CCI_U_TYPE_MULTISET:
                    case CCI_U_TYPE_SEQUENCE:
                     		a_type = CCI_A_TYPE_SET;
                    		break;                               
                    default:
                    		a_type = CCI_A_TYPE_STR;
                    		break;
                }

                if (u_type == CCI_U_TYPE_BLOB || u_type == CCI_U_TYPE_CLOB) 
                {
                    if ((cubrid_retval = cubrid_lob_new(S->H->conn_handle, &lob, u_type, &error)) < 0)
                    {
                        pdo_cubrid_error_stmt(stmt, cubrid_retval, &error, NULL);

                        if (lobfile_name)
                        {
                        	php_stream_close(stm);
                        }

                        return 0;
                    }

                    while (!php_stream_eof(stm))
                    { 
                        lob_read_size = php_stream_read(stm, buf, CUBRID_LOB_READ_BUF_SIZE); 

                        if ((cubrid_retval = cubrid_lob_write(S->H->conn_handle, lob, u_type, 
                        				lob_start_pos, lob_read_size, buf, &error)) < 0) 
                        {
                            pdo_cubrid_error_stmt(stmt, cubrid_retval, &error, NULL);
                            php_stream_close(stm);

                            return 0;
                        }        

                        lob_start_pos += lob_read_size;
                    }
                	
                    if (lobfile_name) 
                    {
                    	php_stream_close(stm);
                    }

                    cubrid_retval = cci_bind_param(S->stmt_handle, bind_index, a_type, (void *) lob, u_type, CCI_BIND_PTR); 

                    S->lob = new_cubrid_lob();
                    S->lob->lob = lob;
                    S->lob->type = u_type;
                } 
                else if (u_type == CCI_U_TYPE_BIT) 
                {
                    bind_value = Z_STRVAL_P(parameter);
                    bind_value_len = Z_STRLEN_P(parameter);
                    bit_value = (T_CCI_BIT *) emalloc(sizeof(T_CCI_BIT));
                    bit_value->size = bind_value_len;
                    bit_value->buf = bind_value;

                    cubrid_retval = cci_bind_param(S->stmt_handle, bind_index, a_type, (void *) bit_value, u_type, 0);

                    efree(bit_value);
                } 
                else if(u_type == CCI_U_TYPE_SET)
                {
                    cubrid_retval = cci_bind_param (S->stmt_handle, bind_index, a_type,set, u_type, 0);    
                    cci_set_free(set);
                }
                else 
                {
                    bind_value = Z_STRVAL_P(parameter);
                    cubrid_retval = cci_bind_param(S->stmt_handle, bind_index, a_type, bind_value, u_type, 0);
                } 
            }

            if (cubrid_retval != 0 || !S->l_bind) 
            {
                pdo_cubrid_error_stmt(stmt, cubrid_retval, NULL, NULL);
                return 0;
            }

            S->l_bind[param->paramno] = 1;

            break;
	case PDO_PARAM_EVT_EXEC_POST:
            if (S->lob)
            {
                if (S->lob->lob) 
                {
                	cubrid_lob_free(S->lob->lob, S->lob->type);
                }
                efree(S->lob);
                S->lob = NULL;
            }

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

static int cubrid_stmt_fetch(pdo_stmt_t *stmt, enum pdo_fetch_orientation ori, long offset TSRMLS_DC)
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

	cubrid_retval = cci_cursor(S->stmt_handle, offset, origin, &error);
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

static int cubrid_stmt_describe_col(pdo_stmt_t *stmt, int colno TSRMLS_DC)
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

	if (type == CCI_U_TYPE_BLOB || type == CCI_U_TYPE_CLOB) {
		cols[colno].param_type = PDO_PARAM_LOB;
	} else {
		/* should make decision later, use STR now */
		cols[colno].param_type = PDO_PARAM_STR;
	}

	return 1;
}

static int cubrid_stmt_get_column_meta(pdo_stmt_t *stmt, long colno, zval *return_value TSRMLS_DC)
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

	add_assoc_string(return_value, "name", S->col_info[colno].col_name);
    add_assoc_string(return_value, "table", S->col_info[colno].class_name);
    add_assoc_string(return_value, "def", S->col_info[colno].default_value);

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

	return 1;
}

static int cubrid_stmt_get_col_data(pdo_stmt_t *stmt, int colno, char **ptr, unsigned long *len, 
		int *caller_frees TSRMLS_DC)
{
	pdo_cubrid_stmt *S = (pdo_cubrid_stmt *)stmt->driver_data;
	struct pdo_column_data *cols = stmt->columns; 

	T_CCI_U_TYPE u_type;
	T_CCI_A_TYPE a_type;
	T_CCI_LOB lob = NULL;

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

	*caller_frees = 0;

	switch (cols[colno].param_type) {
	case PDO_PARAM_STR:
#if PDO_DRIVER_API >= 20080721
	case PDO_PARAM_ZVAL:
#endif
		if ((cubrid_retval = cci_get_data(S->stmt_handle, colno + 1, CCI_A_TYPE_STR, &res_buf, &ind)) < 0) {
			pdo_cubrid_error_stmt(stmt, cubrid_retval, NULL, NULL);
			return 0;
		}

		if (ind < 0) {
			*ptr = NULL;
			*len = 0;
		} else {
			*ptr = res_buf;
			*len = ind;
		}

		break;
	case PDO_PARAM_LOB:
		u_type = CCI_GET_RESULT_INFO_TYPE(S->col_info, colno + 1);

		if (u_type == CCI_U_TYPE_BLOB) {
			a_type = CCI_A_TYPE_BLOB;
		} else if (u_type == CCI_U_TYPE_CLOB) {
			a_type = CCI_A_TYPE_CLOB;
		} else {
			return 0;
		}

		if ((cubrid_retval = cci_get_data(S->stmt_handle, colno + 1, a_type, (void *) &lob, &ind)) < 0) {
			pdo_cubrid_error_stmt(stmt, cubrid_retval, NULL, NULL);
			return 0;
		}

		if (ind < 0) {
			return 0;
		}

		*ptr = (char *)cubrid_create_lob_stream(stmt, lob, u_type TSRMLS_CC);
		*len = 0;

		return *ptr ? 1 : 0;
	case PDO_PARAM_INT:

		break;
	default:
		return 0;
	}
	
	return 1;
}

static int cubrid_stmt_next_rowset(pdo_stmt_t *stmt TSRMLS_DC)
{
	pdo_cubrid_stmt *S = (pdo_cubrid_stmt*)stmt->driver_data;

    T_CCI_COL_INFO *res_col_info;
    T_CCI_CUBRID_STMT res_sql_type;
    int res_col_count = 0;

	int exec_ret = 0;
	T_CCI_ERROR error;

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

	stmt->column_count = res_col_count;
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

struct pdo_stmt_methods cubrid_stmt_methods = {
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

static int get_cubrid_u_type_by_name(const char *type_name)
{
    int i;
    int size = sizeof(db_type_info)/sizeof(db_type_info[0]);

    for (i = 0; i < size; i++) {
		if (strcasecmp(type_name, db_type_info[i].type_name) == 0) {
			return db_type_info[i].cubrid_u_type;
		}
    }

    return CCI_U_TYPE_UNKNOWN;
}

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
	pdo_uint64_t offset;
	pdo_uint64_t size;
};

static size_t cubrid_lob_stream_write(php_stream *stream, const char *buf, size_t count TSRMLS_DC)
{
	return 0;
}

static size_t cubrid_lob_stream_read(php_stream *stream, char *buf, size_t count TSRMLS_DC)
{
	struct cubrid_lob_stream_self *self = (struct cubrid_lob_stream_self*)stream->abstract;

	T_CCI_ERROR error;
	int read_len;
	int cubrid_retval = 0;

	if ((self->offset + count) < self->size) {
		read_len = count;	
	} else {
		read_len = self->size - self->offset;	
		stream->eof = 1;
	}

	if (read_len) {
		cubrid_retval = cubrid_lob_read(self->S->H->conn_handle, self->lob, self->type, self->offset, read_len, buf, &error);
		if (cubrid_retval < 0) {
			return 0;
		} else {
			self->offset += cubrid_retval;
		}
	}

	return (size_t)cubrid_retval;
}

static int cubrid_lob_stream_close(php_stream *stream, int close_handle TSRMLS_DC)
{
	struct cubrid_lob_stream_self *self = (struct cubrid_lob_stream_self*)stream->abstract;
	pdo_stmt_t *stmt = self->stmt;

	if (close_handle) {
        zend_object *obj = &stmt->std;
        GC_REFCOUNT(obj)--;
		cubrid_lob_free(self->lob, self->type);
	}
	efree(self);
	return 0;
}

static int cubrid_lob_stream_flush(php_stream *stream TSRMLS_DC)
{
	return 0;
}

static php_stream_ops cubrid_lob_stream_ops = {
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

static php_stream *cubrid_create_lob_stream(pdo_stmt_t *stmt, T_CCI_LOB lob, T_CCI_U_TYPE type TSRMLS_DC)
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
        zend_object *obj;
        obj = &stmt->std;
        GC_REFCOUNT(obj)++;
		return stm;
	}

	efree(self);
	return NULL;
}

static pdo_cubrid_lob *new_cubrid_lob(void)
{
    pdo_cubrid_lob *lob = (pdo_cubrid_lob *) emalloc(sizeof(pdo_cubrid_lob));

    lob->lob = NULL;
    lob->type = CCI_U_TYPE_BLOB;

    return lob;
}

static int cubrid_lob_new(int con_h_id, T_CCI_LOB *lob, T_CCI_U_TYPE type, T_CCI_ERROR *err_buf)
{
    return (type == CCI_U_TYPE_BLOB) ? 
        cci_blob_new(con_h_id, lob, err_buf) : cci_clob_new(con_h_id, lob, err_buf);
}

static pdo_int64_t cubrid_lob_size(T_CCI_LOB lob, T_CCI_U_TYPE type)
{
    return (type == CCI_U_TYPE_BLOB) ? cci_blob_size(lob) : cci_clob_size(lob);
}

static int cubrid_lob_write(int con_h_id, T_CCI_LOB lob, T_CCI_U_TYPE type, pdo_int64_t start_pos, int length, const char *buf, T_CCI_ERROR *err_buf)
{
    return (type == CCI_U_TYPE_BLOB) ? 
        cci_blob_write(con_h_id, lob, start_pos, length, buf, err_buf) : 
        cci_clob_write(con_h_id, lob, start_pos, length, buf, err_buf);
}

static int cubrid_lob_read(int con_h_id, T_CCI_LOB lob, T_CCI_U_TYPE type, pdo_int64_t start_pos, int length, char *buf, T_CCI_ERROR *err_buf)
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
