.. _c_library_v2:

Standard C Library
##################

The kernel currently provides only the minimal subset of the standard C library
required to meet the needs of Zephyr and its subsystems and features, primarily
in the areas of string manipulation and display.

Applications that require a more extensive C library can either submit
contributions that enhance the existing library or substitute with a
replacement library.

The Zephyr SDK and other supported toolchains comes with a bare-metal C library
based on ``newlib`` that can be used with Zephyr by selecting the
:option:`CONFIG_NEWLIB_LIBC` in the application configuration file. Part of the
support for ``newlib`` is a set of hooks available under
:file:`lib/libc/newlib/libc-hooks.c` which integrates the c library with basic
kernel services.


Minimal C Library
*****************

The minimal C library is part of Zephyr and provides a minimal set of C
functions needed by Zephyr.

The following functions are implemented in the minimal C
library included with Zephyr:

.. rst-class:: rst-columns

   - abs()
   - atoi()
   - bsearch()
   - calloc()
   - free()
   - gmtime()
   - gmtime_r()
   - isalnum()
   - isalpha()
   - isdigit()
   - isgraph()
   - isprint()
   - isspace()
   - isupper()
   - isxdigit()
   - localtime()
   - malloc()
   - memchr()
   - memcmp()
   - memcpy()
   - memmove()
   - memset()
   - mktime()
   - rand()
   - realloc()
   - snprintf()
   - sprintf()
   - strcat()
   - strchr()
   - strcmp()
   - strcpy()
   - strlen()
   - trncat()
   - strncmp()
   - strncpy()
   - strrchr()
   - strstr()
   - strtol()
   - trtoul()
   - time()
   - tolower()
   - toupper()
   - vsnprintf()
   - vsprintf()

