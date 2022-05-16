.. _libc_api:

C standard library
##################

.. contents::
    :local:
    :depth: 2

The `C standard library`_ is an integral part of any C program, and Zephyr
provides two implementations for the application to choose from.

The first one, named "minimal libc" is part of the Zephyr code base and provides
the minimal subset of the standard C library required to meet the needs of
Zephyr and its subsystems and features, primarily in the areas of string
manipulation and display. It is very low footprint and is suitable for projects
that do not rely on less frequently used portions of the ISO C standard library.
Its implementation can be found in :file:`lib/libc/minimal` in the main zephyr
tree.

The second one is `newlib`_, a complete C library implementation written for
embedded systems. Newlib is separate open source project and is not included in
source code form with Zephyr. Instead, the :ref:`zephyr_sdk` comes with a
precompiled library for each supported architecture (:file:`libc.a` and
:file:`libm.a`). Other 3rd-party toolchains, such as :ref:`toolchain_gnuarmemb`,
also bundle newlib as a precompiled library.
Newlib can be enabled by selecting the :kconfig:option:`CONFIG_NEWLIB_LIBC` in the
application configuration file. Part of the support for ``newlib`` is a set of
hooks available under :file:`lib/libc/newlib/libc-hooks.c` which integrates
the C standard library with basic kernel services.


.. _`C standard library`: https://en.wikipedia.org/wiki/C_standard_library
.. _`newlib`: https://sourceware.org/newlib/

API Reference
*************

Error numbers
=============

Error numbers are used throughout Zephyr APIs to signal error conditions as
return values from functions. They are typically returned as the negative value
of the integer literals defined in this section, and are defined in the
:file:`errno.h` header file.
A subset of the error numbers are defined in the `POSIX errno.h specification`_,
and others have been added to it from other sources.

A conscious effort is made in Zephyr to keep the values of system error numbers
consistent between the different implementations of the C standard library. The
version of :file:`errno.h` that is in the main zephyr tree, `errno.h`_, is
checked against newlib's own list to ensure that the error numbers are kept
aligned.

Below is a list of the error number definitions. For the actual numeric values
please refer to `errno.h`_.

.. doxygengroup:: system_errno

.. _`POSIX errno.h specification`: https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/errno.h.html
.. _`errno.h`: https://github.com/zephyrproject-rtos/zephyr/blob/main/lib/libc/minimal/include/errno.h
