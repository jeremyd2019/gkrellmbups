dnl $id$
dnl configure.ac based on sylpheed-claws configure.ac

dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.50)
AC_INIT(src/gkrellmbups.c)
AC_CONFIG_AUX_DIR(config)
PACKAGE=gkrellmbups

dnl version number
MAJOR_VERSION=2
MINOR_VERSION=0
MICRO_VERSION=2
VERSION=$MAJOR_VERSION.$MINOR_VERSION.$MICRO_VERSION
CPPFLAGS="$CPPFLAGS -I/usr/local/include/"

dnl automake
AM_INIT_AUTOMAKE($PACKAGE, $VERSION, no-define)
AC_SUBST(PACKAGE)
AC_SUBST(MAJOR_VERSION)
AC_SUBST(MINOR_VERSION)
AC_SUBST(MICRO_VERSION)
AC_SUBST(VERSION)

dnl Specify a header configuration file
AM_CONFIG_HEADER(config.h)

dnl Checks for programs.
AC_PROG_CC
AC_ISC_POSIX
AM_PROG_CC_STDC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_CPP

dnl Checks for header files.
AC_HEADER_STDC


dnl check for glib
dnl AM_PATH_GLIB(1.2.6,,
dnl	AC_MSG_ERROR(Test for GLIB failed. See the file 'INSTALL' for help.),
dnl	gthread)

dnl Checks for GTK
dnl AM_PATH_GTK(1.2.6,,
dnl	AC_MSG_ERROR(Test for GTK failed. See the file 'INSTALL' for help.))


dnl Check for gkrellm
AC_CHECK_FILE(/usr/local/include/gkrellm/gkrellm.h,
        ac_cv_got_gkrellm=yes,
        ac_cv_got_gkrellm=no)

if test $ac_cv_got_gkrellm = no; then
   AC_CHECK_FILE(/usr/include/gkrellm2/gkrellm.h,,
        AC_MSG_ERROR(Test for GKrellM headers failed. See the file 'INSTALL' for help.))
fi

AC_ARG_ENABLE(nut,
	[  --enable-nut            Use Network UPS Tools (NUT) [default=no]],
	[ac_cv_enable_nut=$enableval], [ac_cv_enable_nut=no])
if test "$ac_cv_enable_nut" = yes; then
   AC_DEFINE(ENABLE_NUT, 1, Include support for cennting to a NUT server)
fi

AC_OUTPUT([
Makefile
src/version.h
src/Makefile
config/Makefile
])

dnl Output the configuration summary
echo ""
echo "$PACKAGE $VERSION"
echo ""
echo "Network UPS Tools: $ac_cv_enable_nut"
echo ""
echo "Configure finished, type 'make' to build."
