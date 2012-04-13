AC_INIT([quartz-engine], [0.1])
AC_PREREQ(2.59)

AC_CONFIG_MACRO_DIR([m4])
m4_include([m4/pkg.m4])

AC_CONFIG_SRCDIR([README])
AC_CONFIG_HEADER(config.h)

AM_INIT_AUTOMAKE(1.9 dist-bzip2 no-define)
AM_MAINTAINER_MODE

AC_PROG_CC
AM_PROG_CC_STDC
AC_PROG_OBJC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

AM_DISABLE_STATIC
AM_PROG_LIBTOOL

PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 2.10.0,,
                  AC_MSG_ERROR([GTK+ 2.10 is required to compile quartz-engine]))

GTK_VERSION=`$PKG_CONFIG --variable=gtk_binary_version gtk+-2.0`
AC_SUBST(GTK_VERSION)

AM_CFLAGS="-arch i386 -mmacosx-version-min=10.6"
AM_OBJCFLAGS="-arch i386 -mmacosx-version-min=10.6"
AC_SUBST(AM_CFLAGS)
AC_SUBST(AM_OBJCFLAGS)

AC_OUTPUT([
  Makefile
  src/Makefile
  theme/Makefile
]) 
