dnl $Id: config.m4,v 2.3 2006/10/17 08:36:36 cgkang Exp $
dnl config.m4 for extension cubrid
dnl don't forget to call PHP_EXTENSION(cubrid)

dnl If your extension references something external, use with:

PHP_ARG_WITH(pdo-cubrid, for CUBRID support for PDO,
[  --with-pdo-cubrid[=DIR]   PDO: CUBRID support. DIR is the CUBRID base install directory.])

if test "$PHP_PDO_CUBRID" != "no"; then

  CUBRID_INCDIR="$CUBRID/include"
  CUBRID_LIBDIR="$CUBRID/lib"

  if test "$PHP_PDO_CUBRID" != "" && test "$PHP_PDO_CUBRID" != "yes"; then
    CUBRID_INCDIR="$PHP_PDO_CUBRID/include"
    CUBRID_LIBDIR="$PHP_PDO_CUBRID/lib"
  fi

  if ! test -r "$CUBRID_INCDIR/cas_cci.h"; then
    AC_MSG_ERROR([$CUBRID_INCDIR/cas_cci.h Please set CUBRID base install dir with --with-pdo-cubrid[=DIR].])
  fi

  PHP_CHECK_LIBRARY("cascci", cci_init, [], [
  AC_MSG_ERROR([$CUBRID_LIBDIR/libcascci.so Please set CUBRID base install dir with --with-pdo-cubrid[=DIR].])
  ], [
  -L$CUBRID_LIBDIR
  ])

  dnl Action..
  PHP_ADD_INCLUDE($CUBRID_INCDIR)

  PHP_ADD_LIBRARY_WITH_PATH(cascci, $CUBRID_LIBDIR, PDO_CUBRID_SHARED_LIBADD)
  PHP_SUBST(PDO_CUBRID_SHARED_LIBADD)

  PHP_NEW_EXTENSION(pdo_cubrid, pdo_cubrid.c cubrid_driver.c cubrid_statement.c, $ext_shared,,-I$pdo_inc_path -I)
  ifdef([PHP_ADD_EXTENSION_DEP],
  [
    PHP_ADD_EXTENSION_DEP(pdo_cubrid, pdo) 
  ])
fi
