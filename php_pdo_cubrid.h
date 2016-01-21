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

#ifndef PHP_PDO_CUBRID_H
#define PHP_PDO_CUBRID_H

extern zend_module_entry pdo_cubrid_module_entry;
#define phpext_pdo_cubrid_ptr &pdo_cubrid_module_entry

#ifdef PHP_WIN32
#	define PHP_PDO_CUBRID_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_PDO_CUBRID_API __attribute__ ((visibility("default")))
#else
#	define PHP_PDO_CUBRID_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

PHP_MINIT_FUNCTION(pdo_cubrid);
PHP_MSHUTDOWN_FUNCTION(pdo_cubrid);
PHP_MINFO_FUNCTION(pdo_cubrid);

#ifdef ZTS
#define PDO_CUBRID_G(v) TSRMG(pdo_cubrid_globals_id, zend_pdo_cubrid_globals *, v)
#else
#define PDO_CUBRID_G(v) (pdo_cubrid_globals.v)
#endif

#endif	/* PHP_PDO_CUBRID_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=manual
 * vim<600: noet sw=4 ts=4
 */
