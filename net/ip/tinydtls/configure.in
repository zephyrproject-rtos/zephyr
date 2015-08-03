#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
#
# Copyright (C) 2011--2015 Olaf Bergmann <bergmann@tzi.org>
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation
# files (the "Software"), to deal in the Software without
# restriction, including without limitation the rights to use, copy,
# modify, merge, publish, distribute, sublicense, and/or sell copies
# of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
# BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
# ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

AC_PREREQ([2.65])
AC_INIT([tinydtls], [0.8.2])
AC_CONFIG_SRCDIR([dtls.c])
dnl AC_CONFIG_HEADERS([config.h])

AC_ARG_WITH(contiki,
  [AS_HELP_STRING([--with-contiki],[build libtinydtls for the Contiki OS])],
  [AC_DEFINE(WITH_CONTIKI,1,[Define to 1 if building for Contiki.])
   WITH_CONTIKI=1],
  [])

AC_PATH_PROG(DOXYGEN, doxygen, [:])
AC_PATH_PROG(ETAGS, etags, [/bin/false])

if test "${with_contiki}" != "yes" ; then
# Checks for programs.
AC_PROG_MAKE_SET
AC_PROG_CC
AC_PROG_RANLIB

AC_C_BIGENDIAN

# Checks for libraries.
AC_SEARCH_LIBS([gethostbyname], [nsl])
AC_SEARCH_LIBS([socket], [socket])
fi

AC_ARG_WITH(debug,
  [AS_HELP_STRING([--without-debug],[disable all debug output and assertions])],
  [CPPFLAGS="${CPPFLAGS} -DNDEBUG"
   NDEBUG=1], 
  [])

AC_ARG_WITH(ecc,
  [AS_HELP_STRING([--without-ecc],[disable support for TLS_ECDHE_ECDSA_WITH_AES_128_CCM_8])],
  [],
  [AC_DEFINE(DTLS_ECC, 1, [Define to 1 if building with ECC support.])
   OPT_OBJS="${OPT_OBJS} ecc/ecc.o"
   DTLS_ECC=1])

AC_ARG_WITH(psk,
  [AS_HELP_STRING([--without-psk],[disable support for TLS_PSK_WITH_AES_128_CCM_8])],
  [],
  [AC_DEFINE(DTLS_PSK, 1, [Define to 1 if building with PSK support])
   DTLS_PSK=1])

CPPFLAGS="${CPPFLAGS} -DDTLSv12 -DWITH_SHA256"
OPT_OBJS="${OPT_OBJS} sha2/sha2.o"

AC_SUBST(OPT_OBJS)
AC_SUBST(NDEBUG)
AC_SUBST(WITH_CONTIKI)
AC_SUBST(DTLS_ECC)
AC_SUBST(DTLS_PSK)

if test "${with_contiki}" = "yes" ; then
  AC_MSG_NOTICE([skipping header checks for Contiki])
else
  # Checks for header files.
  AC_CHECK_HEADERS([assert.h arpa/inet.h fcntl.h inttypes.h netdb.h netinet/in.h stddef.h stdint.h stdlib.h string.h strings.h sys/param.h sys/socket.h unistd.h])

  AC_CHECK_HEADERS([sys/time.h time.h])
  AC_CHECK_HEADERS([sys/types.h sys/stat.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_SIZE_T

AC_CHECK_MEMBER([struct sockaddr_in6.sin6_len],
		[AC_DEFINE(HAVE_SOCKADDR_IN6_SIN6_LEN, [1], 
                  [Define to 1 if struct sockaddr_in6 has a member sin6_len.])], [], 
		[#include <netinet/in.h>])

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset select socket strdup strerror strnlen fls vprintf])
fi

AC_CONFIG_HEADERS([dtls_config.h tinydtls.h])

# Adds Contiki-specific definitions to the end of dtls_config.h
AH_BOTTOM([
#ifdef WITH_CONTIKI
#include "platform-specific/platform.h"
#endif])

AC_CONFIG_FILES([Makefile
                 doc/Makefile
                 doc/Doxyfile
                 tests/Makefile
                 examples/contiki/Makefile
                 platform-specific/Makefile
		 sha2/Makefile
		 aes/Makefile
		 ecc/Makefile])
AC_OUTPUT
