.. _c_library_minimal:

Minimal libc
############

The most basic C library, named "minimal libc", is part of the Zephyr codebase
and provides the minimal subset of the standard C library required to meet the
needs of Zephyr and its subsystems, primarily in the areas of string
manipulation and display.

It is very low footprint and is suitable for projects that do not rely on less
frequently used portions of the ISO C standard library. It can also be used
with a number of different toolchains.

The minimal libc implementation can be found in :file:`lib/libc/minimal` in the
main Zephyr tree.

Functions
*********

The minimal libc implements the minimal subset of the ISO/IEC 9899:2011
standard C library functions required to meet the needs of the Zephyr kernel,
as defined by the :ref:`Coding Guidelines Rule A.4
<coding_guideline_libc_usage_restrictions_in_zephyr_kernel>`.

Formatted Output
****************

The minimal libc does not implement its own formatted output processor;
instead, it maps the C standard formatted output functions such as ``printf``
and ``sprintf`` to the :c:func:`cbprintf` function, which is Zephyr's own
C99-compatible formatted output implementation.

For more details, refer to the :ref:`Formatted Output <formatted_output>` OS
service documentation.

Dynamic Memory Management
*************************

The minimal libc uses the malloc api family implementation provided by the
:ref:`common C library <c_library_common>`, which itself is built upon the
:ref:`kernel memory heap API <heap_v2>`.

Error numbers
*************

Error numbers are used throughout Zephyr APIs to signal error conditions as
return values from functions. They are typically returned as the negative value
of the integer literals defined in this section, and are defined in the
:file:`errno.h` header file.

A subset of the error numbers defined in the `POSIX errno.h specification`_ and
other de-facto standard sources have been added to the minimal libc.

A conscious effort is made in Zephyr to keep the values of the minimal libc
error numbers consistent with the different implementations of the C standard
libraries supported by Zephyr. The minimal libc :file:`errno.h` is checked
against that of the :ref:`Newlib <c_library_newlib>` to ensure that the error
numbers are kept aligned.

Below is a list of the error number definitions. For the actual numeric values
please refer to `errno.h`_.

.. doxygengroup:: system_errno

.. _`POSIX errno.h specification`: https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/errno.h.html
.. _`errno.h`: https://github.com/zephyrproject-rtos/zephyr/blob/main/lib/libc/minimal/include/errno.h
