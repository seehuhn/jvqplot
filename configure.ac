dnl Process this file with autoconf to produce a configure script.
dnl Copyright 2010  Jochen Voss

dnl Process this file with autoconf to produce a configure script.
AC_INIT(jvqplot, 0.2, voss@seehuhn.de)
AC_CONFIG_SRCDIR([jvqplot.c])
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(config.h)

dnl Checks for programs.
AC_PROG_CC

dnl Checks for libraries.
PKG_CHECK_MODULES(GTK, gtk+-2.0)
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
