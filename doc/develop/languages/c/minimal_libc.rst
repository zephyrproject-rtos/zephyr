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

Dynamic Memory Management
*************************

Dynamic memory management in the minimal libc can be enabled by selecting the
:kconfig:option:`CONFIG_MINIMAL_LIBC_MALLOC` in the application configuration
file.

The minimal libc internally uses the :ref:`kernel memory heap API <heap_v2>` to
manage the memory heap used by the standard dynamic memory management interface
functions such as :c:func:`malloc` and :c:func:`free`.

The internal memory heap is normally located in the ``.bss`` section. When
userspace is enabled, however, it is placed in a dedicated memory partition
called ``z_malloc_partition``, which can be accessed from the user mode
threads. The size of the internal memory heap is specified by the
:kconfig:option:`CONFIG_MINIMAL_LIBC_MALLOC_ARENA_SIZE`.

The standard dynamic memory management interface functions implemented by the
minimal libc are thread safe and may be simultaneously called by multiple
threads. These functions are implemented in
:file:`lib/libc/minimal/source/stdlib/malloc.c`.

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
