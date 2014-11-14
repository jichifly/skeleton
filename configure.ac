# configure.ac
# 11/13/2014  jichi
# Originally generated using autoscan
# See: http://mij.oltrelinux.com/devel/autoconf-automake/
# See: http://www.sympa.org/distribution/current/configure.ac

AC_PREREQ([2.69])
AC_INIT([skgen], [0.1], [jichifly@gmail.com])
AM_INIT_AUTOMAKE([subdir-objects foreign -Wall])
#AC_CONFIG_SRCDIR([src/skgen.cc])
#AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX
#AC_PROG_CC


# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([inttypes.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T
AC_CHECK_TYPES([ptrdiff_t])


# Define arguments
AC_ARG_WITH(rose, [--with-rose=DIR path to rose compiler (http://rosecompiler.org)])
AC_ARG_WITH(boost, [--with-boost=DIR path to boost 1.47 (http://www.boost.org)])

if test "x$with_rose" = "x"
then
  AC_CHECK_HEADER(rose.h, [], [AC_MSG_ERROR(rose.h not found)])
else
  CPPFLAGS="$CPPFLAGS -I$with_rose/include"
  LDFLAGS="$LDFLAGS -L$with_rose/lib"
fi

if test "x$with_boost" = "x"
then
  AC_CHECK_HEADER(boost/version.hpp, [], [AC_MSG_ERROR(boost not found)])
else
  CPPFLAGS="$CPPFLAGS -I$with_boost/include"
  LDFLAGS="$LDFLAGS -L$with_boost/lib"
fi

# Checks for library functions.

AC_CONFIG_FILES([
  Makefile
  src/Makefile
])
AC_OUTPUT

# EOF
