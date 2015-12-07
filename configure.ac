# Process this file with autoconf to produce a configure script.
#
# Copyright (C) 1993-2014 Free Software Foundation, Inc.
# This file is part of GNU Make.
#
# GNU Make is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 3 of the License, or (at your option) any later
# version.
#
# GNU Make is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program.  If not, see <http://www.gnu.org/licenses/>.

AC_INIT([GNU make],[4.1.90],[bug-make@gnu.org])

AC_PREREQ([2.62])

# Autoconf setup
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_SRCDIR([vpath.c])
AC_CONFIG_HEADERS([config.h])

# Automake setup
# We have to enable "foreign" because ChangeLog is auto-generated
# We cannot enable -Werror because gettext 0.18.1 has invalid content
# When we update gettext to 0.18.3 or better we can add it again.
AM_INIT_AUTOMAKE([1.11.1 silent-rules foreign -Wall])

# Checks for programs.
#AC_PROG_RANLIB
AC_CHECK_PROG([AR], [ar], [ar], [ar])
# Perl is needed for the test suite (only)
AC_CHECK_PROG([PERL], [perl], [perl], [perl])

# Specialized system macros
AC_CANONICAL_HOST

# This test must come as early as possible after the compiler configuration
# tests, because the choice of the file model can (in principle) affect
# whether functions and headers are available, whether they work, etc.
AC_SYS_LARGEFILE

# See if the user wants to add (or not) GNU Guile support
PKG_PROG_PKG_CONFIG
AC_ARG_WITH([guile], [AS_HELP_STRING([--with-guile],
            [Support GNU Guile for embedded scripting])])

# For some strange reason, at least on Ubuntu, each version of Guile
# comes with it's own PC file so we have to specify them as individual
# packages.  Ugh.
AS_IF([test "x$with_guile" != xno],
[ PKG_CHECK_MODULES([GUILE], [guile-2.0], [have_guile=yes],
  [PKG_CHECK_MODULES([GUILE], [guile-1.8], [have_guile=yes],
    [have_guile=no])])
])

AS_IF([test "$have_guile" = yes],
      [AC_DEFINE([HAVE_GUILE], [1], [Embed GNU Guile support])])

AM_CONDITIONAL([HAVE_GUILE], [test "$have_guile" = yes])

# See if we can handle the job server feature, and if the user wants it.
AC_ARG_ENABLE([job-server],
  AC_HELP_STRING([--disable-job-server],
                 [disallow recursive make communication during -jN]),
  [make_cv_job_server="$enableval" user_job_server="$enableval"],
  [make_cv_job_server="yes"])

# If we support it and the user didn't disable it, build with jobserver
AS_CASE([/$make_cv_job_server/$user_job_server/],
  [*/no/*], [: no jobserver],
  [AC_DEFINE(MAKE_JOBSERVER, 1,
             [Define to 1 to enable job server support in GNU make.])
  ])

AC_ARG_ENABLE([load],
  AC_HELP_STRING([--disable-load],
                 [disable support for the 'load' operation]),
  [make_cv_load="$enableval" user_load="$enableval"],
  [make_cv_load="yes"])


# We might need -ldl
AS_IF([test "$make_cv_load" = yes], [
  AC_SEARCH_LIBS([dlopen], [dl], [], [make_cv_load=])
  LDFLAGS="$LDFLAGS -Wl,--export-dynamic"
  ])

# Let the makefile know what our build host is

AC_DEFINE_UNQUOTED([MAKE_HOST],["$host"],[Build host information.])
MAKE_HOST="$host"
AC_SUBST([MAKE_HOST])

AC_DEFINE_UNQUOTED([PATH_SEPARATOR_CHAR],['$PATH_SEPARATOR'],
      [Define to the character that separates directories in PATH.])

# See if the user wants to use pmake's "customs" distributed build capability
AC_SUBST([REMOTE]) REMOTE=stub
use_customs=false
AC_ARG_WITH([customs],
[AC_HELP_STRING([--with-customs=DIR],
                [enable remote jobs via Customs--see README.customs])],
[ AS_CASE([$withval], [n|no], [:],
    [make_cppflags="$CPPFLAGS"
     AS_CASE([$withval],
             [y|ye|yes], [:],
             [CPPFLAGS="$CPPFLAGS -I$with_customs/include/customs"
              make_ldflags="$LDFLAGS -L$with_customs/lib"])
     CF_NETLIBS
     AC_CHECK_HEADER([customs.h],
                     [use_customs=true
                      REMOTE=cstms
                      LIBS="$LIBS -lcustoms" LDFLAGS="$make_ldflags"],
                     [with_customs=no
                      CPPFLAGS="$make_cppflags" make_badcust=yes])
    ])
])

# Tell automake about this, so it can include the right .c files.
AM_CONDITIONAL([USE_CUSTOMS], [test "$use_customs" = true])

# Allow building with dmalloc
AM_WITH_DMALLOC

# Forcibly disable SET_MAKE.  If it's set it breaks things like the test
# scripts, etc.
SET_MAKE=

# Sanity check and inform the user of what we found

AS_IF([test "x$make_badcust" = xyes], [
echo
echo "WARNING: --with-customs specified but no customs.h could be found;"
echo "         disabling Customs support."
echo
])

AS_CASE([$with_customs],
[""|n|no|y|ye|yes], [:],
[AS_IF([test -f "$with_customs/lib/libcustoms.a"], [:],
[ echo
  echo "WARNING: '$with_customs/lib' does not appear to contain the"
  echo "         Customs library.  You must build and install Customs"
  echo "         before compiling GNU make."
  echo
])])

AS_IF([test "x$has_wait_nohang" = xno],
[ echo
  echo "WARNING: Your system has neither waitpid() nor wait3()."
  echo "         Without one of these, signal handling is unreliable."
  echo "         You should be aware that running GNU make with -j"
  echo "         could result in erratic behavior."
  echo
])

AS_IF([test "x$make_cv_job_server" = xno && test "x$user_job_server" = xyes],
[ echo
  echo "WARNING: Make job server requires a POSIX-ish system that"
  echo "         supports the pipe(), sigaction(), and either"
  echo "         waitpid() or wait3() functions.  Your system doesn't"
  echo "         appear to provide one or more of those."
  echo "         Disabling job server support."
  echo
])

AS_IF([test "x$make_cv_load" = xno && test "x$user_load" = xyes],
[ echo
  echo "WARNING: 'load' support requires a POSIX-ish system that"
  echo "         supports the dlopen(), dlsym(), and dlerror() functions."
  echo "         Your system doesn't appear to provide one or more of these."
  echo "         Disabling 'load' support."
  echo
])

# Specify what files are to be created.
AC_CONFIG_FILES([Makefile tests/config-flags.pm])

# OK, do it!

AC_OUTPUT

dnl Local Variables:
dnl comment-start: "dnl "
dnl comment-end: ""
dnl comment-start-skip: "\\bdnl\\b\\s *"
dnl compile-command: "make configure config.h.in"
dnl End:
