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

#ifndef PHP_PDO_CUBRID8_H
#define PHP_PDO_CUBRID8_H

/* Supported PHP versions: 8.3 or later on Linux, 8.4 or later on Windows. */
#if PHP_VERSION_ID < 80300
#error "pdo_cubrid requires PHP 8.3 or later"
#endif
#if defined(PHP_WIN32) && PHP_VERSION_ID < 80400
#error "pdo_cubrid requires PHP 8.4 or later on Windows"
#endif

extern zend_module_entry pdo_cubrid_module_entry;
#define phpext_pdo_cubrid_ptr &pdo_cubrid_module_entry

#ifdef PHP_WIN32
#	define PHP_PDO_CUBRID_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_PDO_CUBRID_API __attribute__ ((visibility("default")))
#else
#	define PHP_PDO_CUBRID_API
#endif

#ifdef MS_WINDOWS
#define CUBRID_LONG_LONG _int64
#else
#define CUBRID_LONG_LONG long long
#endif

PHP_MINIT_FUNCTION(pdo_cubrid);
PHP_MSHUTDOWN_FUNCTION(pdo_cubrid);
PHP_MINFO_FUNCTION(pdo_cubrid);

#endif	/* PHP_PDO_CUBRID8_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=manual
 * vim<600: noet sw=4 ts=4
 */
