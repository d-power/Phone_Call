# -*- Autoconf -*-

# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([PHONECALLCORE], [1.0.26], [liaosenlin@d-power.com.cn])

PHONECALLCORE_MAJOR_VERSION=1
PHONECALLCORE_MINOR_VERSION=0
PHONECALLCORE_MICRO_VERSION=26

PHONECALLCORE_VERSION=$PHONECALLCORE_MAJOR_VERSION.$PHONECALLCORE_MINOR_VERSION.$PHONECALLCORE_MICRO_VERSION

AC_CANONICAL_SYSTEM

AM_INIT_AUTOMAKE()
AC_CONFIG_HEADERS([include/config.h])

AC_PROG_LIBTOOL

AC_PROG_CC
AC_PROG_CXX
AC_PROG_RANLIB
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

LIBS="-lpthread -lrt -lVideoDec -lAudioCore -lphonecallcore"

AC_SUBST(PHONECALLCORE_VERSION)

AC_CONFIG_FILES([Makefile
src/Makefile])

AC_OUTPUT

AC_MSG_NOTICE([Configured To Build Program:
Version:          ${PHONECALLCORE_VERSION}
CFLAGS:           ${CFLAGS}
CPPFLAGS:         ${CXXFLAGS}
Host setup:       ${host}
Install prefix:   ${prefix}
Compiler:         ${CC} ${CXX}
])



