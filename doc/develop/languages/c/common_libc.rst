.. _c_library_common:

Common C library code
#####################

Zephyr provides some C library functions that are designed to be used in
conjunction with multiple C libraries. These either provide functions not
available in multiple C libraries or are designed to replace functionality
in the C library with code better suited for use in the Zephyr environment

Time function
*************

This provides an implementation of the standard C function, :c:func:`time`,
relying on the Zephyr function, :c:func:`clock_gettime`. This function can
be enabled by selecting :kconfig:option:`COMMON_LIBC_TIME`.

Dynamic Memory Management
*************************

The common dynamic memory management implementation can be enabled by
selecting the :kconfig:option:`CONFIG_COMMON_LIBC_MALLOC` in the
application configuration file.

The common C library internally uses the :ref:`kernel memory heap API
<heap_v2>` to manage the memory heap used by the standard dynamic memory
management interface functions such as :c:func:`malloc` and :c:func:`free`.

The internal memory heap is normally located in the ``.bss`` section. When
userspace is enabled, however, it is placed in a dedicated memory partition
called ``z_malloc_partition``, which can be accessed from the user mode
threads. The size of the internal memory heap is specified by the
:kconfig:option:`CONFIG_COMMON_LIBC_MALLOC_ARENA_SIZE`.

The default heap size for applications using the common C library is zero
(no heap). For other C library users, if there is an MMU present, then the
default heap is 16kB. Otherwise, the heap uses all available memory.

There are also separate controls to select :c:func:`calloc`
(:kconfig:option:`COMMON_LIBC_CALLOC`) and :c:func:`reallocarray`
(:kconfig:option:`COMMON_LIBC_REALLOCARRAY`). Both of these are enabled by
default as that doesn't impact memory usage in applications not using them.

The standard dynamic memory management interface functions implemented by
the common C library are thread safe and may be simultaneously called by
multiple threads. These functions are implemented in
:file:`lib/libc/common/source/stdlib/malloc.c`.

Error numbers
*************

Error numbers are used throughout Zephyr APIs to signal error conditions as
return values from functions. They are typically returned as the negative value
of the integer literals defined in this section, and are defined in the
:file:`errno.h` header file.

A subset of the error numbers defined in the `POSIX errno.h specification`_ and
other de-facto standard sources have been added to the common libc resources.

A conscious effort is made in Zephyr to keep the values of the common libc
error numbers consistent with the different implementations of the C standard
libraries supported by Zephyr. The minimal libc :file:`errno.h` is checked
against that of the :ref:`Newlib <c_library_newlib>` to ensure that the error
numbers are kept aligned.

Below is a list of the error number definitions. For the actual numeric values
please refer to :file:`lib/libc/common/include/errno.h`.

.. doxygengroup:: system_errno

.. _`POSIX errno.h specification`: https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/errno.h.html
