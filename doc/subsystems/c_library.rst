.. _c_library_v2:

Standard C Library
##################

The kernel currently provides only the minimal subset of the standard C library
required to meet the kernel's own needs, primarily in the areas of string
manipulation and display.

Applications that require a more extensive C library can either submit
contributions that enhance the existing library or substitute with a replacement
library.

The Zephyr SDK and other supported toolchains comes with a bare-metal C library
based on ``newlib`` that can be used with Zephyr by selecting the
:option:`CONFIG_NEWLIB_LIBC` in the application configuration file. Part of the
support for ``newlib`` is a set of hooks available under
:file:`lib/libc/newlib/libc-hooks.c` which integrates the c library with basic
kernel services.
