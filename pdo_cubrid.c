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
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id: header 297205 2010-03-30 21:09:07Z johannes $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "ext/pdo/php_pdo.h"
#include "ext/pdo/php_pdo_driver.h"

#include "php_pdo_cubrid.h"
#include "php_pdo_cubrid_int.h"

#include <cas_cci.h>
#include "pdo_cubrid_version.h"

const zend_function_entry pdo_cubrid_functions[] = {
	{NULL, NULL, NULL}
};

#if ZEND_MODULE_API_NO >= 20050922
static const zend_module_dep pdo_cubrid_deps[] = {
	ZEND_MOD_REQUIRED("pdo")
	{NULL, NULL, NULL}
};
#endif

zend_module_entry pdo_cubrid_module_entry = {
#if ZEND_MODULE_API_NO >= 20050922
	STANDARD_MODULE_HEADER_EX, NULL,
	pdo_cubrid_deps,
#else
	STANDARD_MODULE_HEADER,
#endif
	"pdo_cubrid",
	pdo_cubrid_functions,
	PHP_MINIT(pdo_cubrid),
	PHP_MSHUTDOWN(pdo_cubrid),
	NULL,
	NULL,
	PHP_MINFO(pdo_cubrid),
	"1.0.0",
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_PDO_CUBRID
ZEND_GET_MODULE(pdo_cubrid)
#endif

PHP_MINIT_FUNCTION(pdo_cubrid)
{
	cci_init();

	REGISTER_PDO_CLASS_CONST_LONG("CUBRID_SCH_TABLE", CUBRID_SCH_TABLE);
	REGISTER_PDO_CLASS_CONST_LONG("CUBRID_SCH_VIEW", CUBRID_SCH_VIEW);
	REGISTER_PDO_CLASS_CONST_LONG("CUBRID_SCH_QUERY_SPEC", CUBRID_SCH_QUERY_SPEC);
	REGISTER_PDO_CLASS_CONST_LONG("CUBRID_SCH_ATTRIBUTE", CUBRID_SCH_ATTRIBUTE);
	REGISTER_PDO_CLASS_CONST_LONG("CUBRID_SCH_TABLE_ATTRIBUTE", CUBRID_SCH_TABLE_ATTRIBUTE);
	REGISTER_PDO_CLASS_CONST_LONG("CUBRID_SCH_METHOD", CUBRID_SCH_METHOD);
	REGISTER_PDO_CLASS_CONST_LONG("CUBRID_SCH_TABLE_METHOD", CUBRID_SCH_TABLE_METHOD);
	REGISTER_PDO_CLASS_CONST_LONG("CUBRID_SCH_METHOD_FILE", CUBRID_SCH_METHOD_FILE);
	REGISTER_PDO_CLASS_CONST_LONG("CUBRID_SCH_SUPER_TABLE", CUBRID_SCH_SUPER_TABLE);
	REGISTER_PDO_CLASS_CONST_LONG("CUBRID_SCH_SUB_TABLE", CUBRID_SCH_SUB_TABLE);
	REGISTER_PDO_CLASS_CONST_LONG("CUBRID_SCH_CONSTRAINT", CUBRID_SCH_CONSTRAINT);
	REGISTER_PDO_CLASS_CONST_LONG("CUBRID_SCH_TRIGGER", CUBRID_SCH_TRIGGER);
	REGISTER_PDO_CLASS_CONST_LONG("CUBRID_SCH_TABLE_PRIVILEGE", CUBRID_SCH_TABLE_PRIVILEGE);
	REGISTER_PDO_CLASS_CONST_LONG("CUBRID_SCH_COL_PRIVILEGE", CUBRID_SCH_COL_PRIVILEGE);
	REGISTER_PDO_CLASS_CONST_LONG("CUBRID_SCH_DIRECT_SUPER_TABLE", CUBRID_SCH_DIRECT_SUPER_TABLE);
	REGISTER_PDO_CLASS_CONST_LONG("CUBRID_SCH_PRIMARY_KEY", CUBRID_SCH_PRIMARY_KEY);

	return php_pdo_register_driver(&pdo_cubrid_driver);
}

PHP_MSHUTDOWN_FUNCTION(pdo_cubrid)
{
	cci_end();

	php_pdo_unregister_driver(&pdo_cubrid_driver);

	return SUCCESS;
}

PHP_MINFO_FUNCTION(pdo_cubrid)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "pdo_cubrid support", "enabled");
    php_info_print_table_row(2, "Client API version", PDO_CUBRID_VERSION);
    php_info_print_table_row(2, "Supported CUBRID server", "8.3.0");
	php_info_print_table_end();
}


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=manual
 * vim<600: noet sw=4 ts=4
 */
