AC_INIT([ec], [0.1], [calebcase@gmail.com])
AC_CONFIG_AUX_DIR([.])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])
AM_PROG_AR
LT_INIT
AC_PROG_CC
AC_PROG_CC_C99
AM_PROG_CC_C_O
PKG_CHECK_MODULES([CHECK], [check >= 0.9.4])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
    Makefile
    include/Makefile
    src/Makefile
    test/Makefile
    test/check/Makefile
    test/example/Makefile
])
AC_OUTPUT
