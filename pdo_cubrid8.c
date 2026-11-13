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

#include "php_pdo_cubrid8.h"
#include "php_pdo_cubrid8_int.h"

#include <cas_cci.h>
#include "pdo_cubrid_version.h"

static const zend_module_dep pdo_cubrid_deps[] = {
	ZEND_MOD_REQUIRED("pdo")
	ZEND_MOD_END
};

zend_module_entry pdo_cubrid_module_entry = {
	STANDARD_MODULE_HEADER_EX, NULL,
	pdo_cubrid_deps,
	"pdo_cubrid",
	NULL,
	PHP_MINIT(pdo_cubrid),
	PHP_MSHUTDOWN(pdo_cubrid),
	NULL,
	NULL,
	PHP_MINFO(pdo_cubrid),
	PDO_CUBRID_VERSION,
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_PDO_CUBRID
ZEND_GET_MODULE(pdo_cubrid)
#endif

/* Driver constants: PDO::<pdo_name> (all PHP versions) and
 * Pdo\Cubrid::<name> (PHP 8.4+, driver specific subclass). */
static const struct {
	const char *pdo_name;
	const char *name;
	zend_long value;
} pdo_cubrid_constants[] = {
	{ "CUBRID_ATTR_ISOLATION_LEVEL", "ATTR_ISOLATION_LEVEL", PDO_CUBRID_ATTR_ISOLATION_LEVEL },
	{ "CUBRID_ATTR_LOCK_TIMEOUT", "ATTR_LOCK_TIMEOUT", PDO_CUBRID_ATTR_LOCK_TIMEOUT },
	{ "CUBRID_ATTR_MAX_STRING_LENGTH", "ATTR_MAX_STRING_LENGTH", PDO_CUBRID_ATTR_MAX_STRING_LENGTH },
	{ "TRAN_REP_CLASS_COMMIT_INSTANCE", "TRAN_REP_CLASS_COMMIT_INSTANCE", TRAN_REP_CLASS_COMMIT_INSTANCE },
	{ "TRAN_REP_CLASS_REP_INSTANCE", "TRAN_REP_CLASS_REP_INSTANCE", TRAN_REP_CLASS_REP_INSTANCE },
	{ "TRAN_SERIALIZABLE", "TRAN_SERIALIZABLE", TRAN_SERIALIZABLE },
	{ "CUBRID_SCH_TABLE", "SCH_TABLE", CUBRID_SCH_TABLE },
	{ "CUBRID_SCH_VIEW", "SCH_VIEW", CUBRID_SCH_VIEW },
	{ "CUBRID_SCH_QUERY_SPEC", "SCH_QUERY_SPEC", CUBRID_SCH_QUERY_SPEC },
	{ "CUBRID_SCH_ATTRIBUTE", "SCH_ATTRIBUTE", CUBRID_SCH_ATTRIBUTE },
	{ "CUBRID_SCH_TABLE_ATTRIBUTE", "SCH_TABLE_ATTRIBUTE", CUBRID_SCH_TABLE_ATTRIBUTE },
	{ "CUBRID_SCH_METHOD", "SCH_METHOD", CUBRID_SCH_METHOD },
	{ "CUBRID_SCH_TABLE_METHOD", "SCH_TABLE_METHOD", CUBRID_SCH_TABLE_METHOD },
	{ "CUBRID_SCH_METHOD_FILE", "SCH_METHOD_FILE", CUBRID_SCH_METHOD_FILE },
	{ "CUBRID_SCH_SUPER_TABLE", "SCH_SUPER_TABLE", CUBRID_SCH_SUPER_TABLE },
	{ "CUBRID_SCH_SUB_TABLE", "SCH_SUB_TABLE", CUBRID_SCH_SUB_TABLE },
	{ "CUBRID_SCH_CONSTRAINT", "SCH_CONSTRAINT", CUBRID_SCH_CONSTRAINT },
	{ "CUBRID_SCH_TRIGGER", "SCH_TRIGGER", CUBRID_SCH_TRIGGER },
	{ "CUBRID_SCH_TABLE_PRIVILEGE", "SCH_TABLE_PRIVILEGE", CUBRID_SCH_TABLE_PRIVILEGE },
	{ "CUBRID_SCH_COL_PRIVILEGE", "SCH_COL_PRIVILEGE", CUBRID_SCH_COL_PRIVILEGE },
	{ "CUBRID_SCH_DIRECT_SUPER_TABLE", "SCH_DIRECT_SUPER_TABLE", CUBRID_SCH_DIRECT_SUPER_TABLE },
	{ "CUBRID_SCH_PRIMARY_KEY", "SCH_PRIMARY_KEY", CUBRID_SCH_PRIMARY_KEY },
	{ "CUBRID_SCH_IMPORTED_KEYS", "SCH_IMPORTED_KEYS", CUBRID_SCH_IMPORTED_KEYS },
	{ "CUBRID_SCH_EXPORTED_KEYS", "SCH_EXPORTED_KEYS", CUBRID_SCH_EXPORTED_KEYS },
	{ "CUBRID_SCH_CROSS_REFERENCE", "SCH_CROSS_REFERENCE", CUBRID_SCH_CROSS_REFERENCE },
	{ "CUBRID_SCH_ATTR_WITH_SYNONYM", "SCH_ATTR_WITH_SYNONYM", CUBRID_SCH_ATTR_WITH_SYNONYM },
};

#if PHP_VERSION_ID >= 80400
static zend_class_entry *pdo_cubrid_ce;
#endif

PHP_MINIT_FUNCTION(pdo_cubrid)
{
	size_t i;

	cci_init();

#if PHP_VERSION_ID >= 80400
	{
		zend_class_entry ce;

		INIT_NS_CLASS_ENTRY(ce, "Pdo", "Cubrid", pdo_cubrid_class_methods);
		pdo_cubrid_ce = zend_register_internal_class_ex(&ce, php_pdo_get_dbh_ce());
		pdo_cubrid_ce->create_object = pdo_dbh_new;

		for (i = 0; i < sizeof(pdo_cubrid_constants) / sizeof(pdo_cubrid_constants[0]); i++) {
			zend_declare_class_constant_long(pdo_cubrid_ce, pdo_cubrid_constants[i].name,
				strlen(pdo_cubrid_constants[i].name), pdo_cubrid_constants[i].value);
		}
	}
#endif

	/* Declared after Pdo\Cubrid is registered: the subclass would inherit
	 * PDO::TRAN_* and could not declare its own TRAN_* constants. */
	for (i = 0; i < sizeof(pdo_cubrid_constants) / sizeof(pdo_cubrid_constants[0]); i++) {
		zend_declare_class_constant_long(php_pdo_get_dbh_ce(), pdo_cubrid_constants[i].pdo_name,
			strlen(pdo_cubrid_constants[i].pdo_name), pdo_cubrid_constants[i].value);
	}

	if (php_pdo_register_driver(&pdo_cubrid_driver) == FAILURE) {
		return FAILURE;
	}

#if PHP_VERSION_ID >= 80400
	/* PDO::connect("cubrid:...") returns Pdo\Cubrid, and new Pdo\Cubrid() accepts only cubrid DSNs. */
	return php_pdo_register_driver_specific_ce(&pdo_cubrid_driver, pdo_cubrid_ce);
#else
	return SUCCESS;
#endif
}

PHP_MSHUTDOWN_FUNCTION(pdo_cubrid)
{
	cci_end();

	php_pdo_unregister_driver(&pdo_cubrid_driver);

	return SUCCESS;
}

PHP_MINFO_FUNCTION(pdo_cubrid)
{
    int major, minor, patch;
    char info[128];

    cci_get_version(&major, &minor, &patch);

    snprintf(info, sizeof(info), "%d.%d.%d", major, minor, patch);

    php_info_print_table_start();
    php_info_print_table_header(2, "pdo_cubrid support", "enabled");
    php_info_print_table_row(2, "Client API version", PDO_CUBRID_VERSION);
    php_info_print_table_row(2, "CCI Version", info);
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
