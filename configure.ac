# -*- mode: autoconf -*-

AC_INIT([ccittfax],[0.0.1],[])

dnl AC_CONFIG_MACRO_DIR([etc/m4])
dnl AC_CONFIG_AUX_DIR([etc/m4])

AM_INIT_AUTOMAKE([-Wno-portability subdir-objects dist-bzip2 nostdinc foreign])
AC_CONFIG_HEADERS([include/ccittfax/_config.h:include/ccittfax/config.ac])
AM_SILENT_RULES([yes])

AC_PROG_CC

LT_INIT
AC_PROG_MAKE_SET(gmake)
AM_PROG_AS

AC_CONFIG_FILES(Makefile)
AC_CONFIG_FILES(include/Makefile)
AC_CONFIG_FILES(src/Makefile)
AC_CONFIG_FILES(tests/Makefile)

AC_OUTPUT

