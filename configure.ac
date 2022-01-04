# -*- Autoconf -*-
# configure.ac - process this file with autoconf to produce a configure script
# Copyright © 2012,2013 Géraud Meyer <graud@gmx.com>
#   This file is part of CDimg|tools.
#
#   CDimg|tools is free software; you can redistribute it and/or modify it under
#   the terms of the GNU General Public License version 2 as published by the
#   Free Software Foundation.
#
#   This package is distributed in the hope that it will be useful, but WITHOUT
#   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
#   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
#   more details.
#
#   You should have received a copy of the GNU General Public License
#   along with CDimg|tools.  If not, see <http://www.gnu.org/licenses/>.

AC_PREREQ([2.62])
define([dynamic_version], [esyscmd(./version.sh | tr -d '\n')])
AC_INIT([CDimg|tools], [dynamic_version], [g_raud@gna.org], [cdimgtools])
AC_CONFIG_SRCDIR([cssdec.c])
AC_CONFIG_HEADERS([config.h])
m4_include([m4/ax_prog_perl_modules.m4])

# Checks for programs.
AC_PROG_SED
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

AC_CHECK_PROGS(ASCIIDOC, [asciidoc], [false])
AC_CHECK_PROGS(POD2MAN, [pod2man], [false])
AC_CHECK_PROGS(POD2HTML, [pod2html], [false])
AC_CHECK_PROGS(XMLTO, [xmlto], [false])

# Checks for libraries.
AC_CHECK_LIB([dvdcss], [dvdcss_open], [], [AC_MSG_ERROR([Could not find libdvdcss])])
AC_CHECK_LIB([dvdread], [UDFFindFile], [], [AC_MSG_ERROR([Could not find libdvdread])])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h limits.h stdlib.h string.h unistd.h])
AC_SYS_LARGEFILE

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_CHECK_MEMBERS([struct stat.st_rdev])
AC_TYPE_UINT32_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([strerror strtol])

# Runtime dependencies
AC_SYS_INTERPRETER
if test x$interpval = xno; then
	AC_MSG_WARN([the scripts provided may be unusable])
fi
AC_PATH_PROG(PERL, perl perl5.004 perl5.003 perl5.002 perl5.001 perl5, no)
if test x$PERL = xno; then
	PERL=/usr/bin/perl
	AC_MSG_WARN([perl not found; default of $PERL used])
fi
AX_PROG_PERL_MODULES([Pod::Usage String::Escape Data::Hexdumper], ,
	AC_MSG_WARN([some Perl modules not found]))

AC_CONFIG_FILES([config.make])
AC_OUTPUT
