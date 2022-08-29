.. _c_library_newlib:

Newlib
######

`Newlib`_ is a complete C library implementation written for the embedded
systems. It is a separate open source project and is not included in source
code form with Zephyr. Instead, the :ref:`toolchain_zephyr_sdk` includes a
precompiled library for each supported architecture (:file:`libc.a` and
:file:`libm.a`).

.. note::
   Other 3rd-party toolchains, such as :ref:`toolchain_gnuarmemb`, also bundle
   the Newlib as a precompiled library.

Zephyr implements the "API hook" functions that are invoked by the C standard
library functions in the Newlib. These hook functions are implemented in
:file:`lib/libc/newlib/libc-hooks.c` and translate the library internal system
calls to the equivalent Zephyr API calls.

Types of Newlib
***************

The Newlib included in the :ref:`toolchain_zephyr_sdk` comes in two versions:
'full' and 'nano' variants.

Full Newlib
===========

The Newlib full variant (:file:`libc.a` and :file:`libm.a`) is the most capable
variant of the Newlib available in the Zephyr SDK, and supports almost all
standard C library features. It is optimized for performance (prefers
performance over code size) and its footprint is significantly larger than the
the nano variant.

This variant can be enabled by selecting the
:kconfig:option:`CONFIG_NEWLIB_LIBC` and de-selecting the
:kconfig:option:`CONFIG_NEWLIB_LIBC_NANO` in the application configuration
file.

Nano Newlib
===========

The Newlib nano variant (:file:`libc_nano.a` and :file:`libm_nano.a`) is the
size-optimized version of the Newlib, and supports all features that the full
variant supports except the new format specifiers introduced in C99, such as
the ``char``, ``long long`` type format specifiers (i.e. ``%hhX`` and
``%llX``).

This variant can be enabled by selecting the
:kconfig:option:`CONFIG_NEWLIB_LIBC` and
:kconfig:option:`CONFIG_NEWLIB_LIBC_NANO` in the application configuration
file.

Note that the Newlib nano variant is not available for all architectures. The
availability of the nano variant is specified by the
:kconfig:option:`CONFIG_HAS_NEWLIB_LIBC_NANO`.

When available (:kconfig:option:`CONFIG_HAS_NEWLIB_LIBC_NANO` is selected),
the Newlib nano variant is enabled by default unless
:kconfig:option:`CONFIG_NEWLIB_LIBC_NANO` is explicitly de-selected.

.. _`Newlib`: https://sourceware.org/newlib/

Formatted Output
****************

Newlib supports all standard C formatted input and output functions, including
``printf``, ``fprintf``, ``sprintf`` and ``sscanf``.

The Newlib formatted input and output function implementation supports all
format specifiers defined by the C standard with the following exceptions:

* Floating point format specifiers (e.g. ``%f``) require
  :kconfig:option:`CONFIG_NEWLIB_LIBC_FLOAT_PRINTF` and
  :kconfig:option:`CONFIG_NEWLIB_LIBC_FLOAT_SCANF` to be enabled.
* C99 format specifiers are not supported in the Newlib nano variant (i.e.
  ``%hhX`` for ``char``, ``%llX`` for ``long long``, ``%jX`` for ``intmax_t``,
  ``%zX`` for ``size_t``, ``%tX`` for ``ptrdiff_t``).

Dynamic Memory Management
*************************

Newlib implements an internal heap allocator to manage the memory blocks used
by the standard dynamic memory management interface functions (for example,
:c:func:`malloc` and :c:func:`free`).

The internal heap allocator implemented by the Newlib may vary across the
different types of the Newlib used. For example, the heap allocator implemented
in the Full Newlib (:file:`libc.a` and :file:`libm.a`) of the Zephyr SDK
requests larger memory chunks to the operating system and has a significantly
higher minimum memory requirement compared to that of the Nano Newlib
(:file:`libc_nano.a` and :file:`libm_nano.a`).

The only interface between the Newlib dynamic memory management functions and
the Zephyr-side libc hooks is the :c:func:`sbrk` function, which is used by the
Newlib to manage the size of the memory pool reserved for its internal heap
allocator.

The :c:func:`_sbrk` hook function, implemented in :file:`libc-hooks.c`, handles
the memory pool size change requests from the Newlib and ensures that the
Newlib internal heap allocator memory pool size does not exceed the amount of
available memory space by returning an error when the system is out of memory.

When userspace is enabled, the Newlib internal heap allocator memory pool is
placed in a dedicated memory partition called ``z_malloc_partition``, which can
be accessed from the user mode threads.

The amount of memory space available for the Newlib heap depends on the system
configurations:

* When MMU is enabled (:kconfig:option:`CONFIG_MMU` is selected), the amount of
  memory space reserved for the Newlib heap is set by the size of the free
  memory space returned by the :c:func:`k_mem_free_get` function or the
  :kconfig:option:`CONFIG_NEWLIB_LIBC_MAX_MAPPED_REGION_SIZE`, whichever is the
  smallest.

* When MPU is enabled and the MPU requires power-of-two partition size and
  address alignment (:kconfig:option:`CONFIG_NEWLIB_LIBC_ALIGNED_HEAP_SIZE` is
  set to a non-zero value), the amount of memory space reserved for the Newlib
  heap is set by the :kconfig:option:`CONFIG_NEWLIB_LIBC_ALIGNED_HEAP_SIZE`.

* Otherwise, the amount of memory space reserved for the Newlib heap is equal
  to the amount of free (unallocated) memory in the SRAM region.

The standard dynamic memory management interface functions implemented by the
Newlib are thread safe and may be simultaneously called by multiple threads.
