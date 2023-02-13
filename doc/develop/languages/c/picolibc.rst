.. _c_library_picolibc:

Picolibc
########

`Picolibc`_ is a complete C library implementation written for the
embedded systems, targeting `C17 (ISO/IEC 9899:2018)`_ and `POSIX 2018
(IEEE Std 1003.1-2017)`_ standards. Picolibc is an external open
source project which is provided for Zephyr as a module, and included
as part of the :ref:`toolchain_zephyr_sdk` in precompiled form for
each supported architecture (:file:`libc.a`).

.. note::
   Picolibc is also available for other 3rd-party toolchains, such as
   :ref:`toolchain_gnuarmemb`.

Zephyr implements the “API hook” functions that are invoked by the C
standard library functions in the Picolibc. These hook functions are
implemented in :file:`lib/libc/picolibc/libc-hooks.c` and translate
the library internal system calls to the equivalent Zephyr API calls.

.. _`Picolibc`: https://github.com/picolibc/picolibc
.. _`C17 (ISO/IEC 9899:2018)`: https://www.iso.org/standard/74528.html
.. _`POSIX 2018 (IEEE Std 1003.1-2017)`: https://pubs.opengroup.org/onlinepubs/9699919799/functions/printf.html

Picolibc Module
===============

When built as a Zephyr module, there are several configuration knobs
available to adjust the feature set in the library, balancing what the
library supports versus the code size of the resulting
functions. Because the standard C++ library must be compiled for the
target C library, the Picolibc module cannot be used with applications
which use the standard C++ library. Building the Picolibc module will
increase the time it takes to compile the application.

The Picolibc module can be enabled by selecting
:kconfig:option:`CONFIG_PICOLIBC_USE_MODULE` in the application
configuration file.

Toolchain Picolibc
==================

Starting with version 0.16, the Zephyr SDK includes precompiled
versions of Picolibc for every target architecture, along with
precompiled versions of libstdc++.

The toolchain version of Picolibc can be enabled by de-selecting
:kconfig:option:`CONFIG_PICOLIBC_USE_MODULE` in the application
configuration file.

Formatted Output
****************

Picolibc supports all standard C formatted input and output functions,
including :c:func:`printf`, :c:func:`fprintf`, :c:func:`sprintf` and
:c:func:`sscanf`.

Picolibc formatted input and output function implementation supports
all format specifiers defined by the C17 and POSIX 2018 standards with
the following exceptions:

* Floating point format specifiers (e.g. ``%f``) require
  :kconfig:option:`CONFIG_PICOLIBC_IO_FLOAT`.

* Long long format specifiers (e.g. ``%lld``) require
  :kconfig:option:`CONFIG_PICOLIBC_IO_LONG_LONG`. This option is
  automatically enabled with :kconfig:option:`CONFIG_PICOLIBC_IO_FLOAT`.

Printk, cbprintf and friends
****************************

When using Picolibc, Zephyr formatted output functions are
implemented in terms of stdio calls. This includes:

 * printk, snprintk and vsnprintk
 * cbprintf and cbvprintf
 * fprintfcb, vfprintfcb, printfcb, vprintfcb, snprintfcb and vsnprintfcb

When using tagged args
(:kconfig:option:`CONFIG_CBPRINTF_PACKAGE_SUPPORT_TAGGED_ARGUMENTS` and
:c:macro:`CBPRINTF_PACKAGE_ARGS_ARE_TAGGED`), calls to cbpprintf will
not use Picolibc, so formatting of output using those code will differ
from Picolibc results as the cbprintf functions are not completely
C/POSIX compliant.

Math Functions
**************

Picolibc provides full C17/`IEEE STD 754-2019`_ support for float,
double and long double math operations, except for long double
versions of the Bessel functions.

.. _`IEEE STD 754-2019`: https://ieeexplore.ieee.org/document/8766229

Thread Local Storage
********************

Picolibc uses Thread Local Storage (TLS) (where supported) for data
which is supposed to remain local to each thread, like
:c:var:`errno`. This means that TLS support is enabled when using
Picolibc. As all TLS variables are allocated out of the thread stack
area, this can affect stack size requirements by a few bytes.

C Library Local Variables
*************************

Picolibc uses a few internal variables for things like heap
management. These are collected in a dedicated memory partition called
:c:var:`z_libc_partition`. Applications using
:kconfig:option:`CONFIG_USERSPACE` and memory domains must ensure that
this partition is included in any domain active during Picolibc calls.

Dynamic Memory Management
*************************

Picolibc implements an internal heap allocator to manage the memory
blocks used by the standard dynamic memory management interface
functions (for example, :c:func:`malloc` and :c:func:`free`).

The only interface between the Picolibc dynamic memory management
functions and the Zephyr-side libc hooks is the :c:func:`sbrk`
function, which is used by Picolibc to manage the size of the
memory pool reserved for its internal heap allocator.

The :c:func:`sbrk` hook function, implemented in
:file:`libc-hooks.c`, handles the memory pool size change requests
from Picolibc and ensures that the Picolibc internal heap allocator
memory pool size does not exceed the amount of available memory space
by returning an error when the system is out of memory.

When userspace is enabled, the Picolibc internal heap allocator memory
pool is placed in a dedicated memory partition called
:c:var:`z_malloc_partition`, which can be accessed from the user mode
threads.

The amount of memory space available for the Picolibc heap is set by
the :kconfig:option:`CONFIG_PICOLIBC_HEAP_SIZE`, or the amount of free
memory available, whichever is smallest. Set
:kconfig:option:`CONFIG_PICOLIBC_HEAP_SIZE` to -1 to always use the
amount of free memory available.

* When MMU is enabled (:kconfig:option:`CONFIG_MMU` is selected), the
  amount of free memory available is determined at runtime with the
  :c:func:`k_mem_free_get` function.

* When MPU is enabled and the MPU requires power-of-two partition
  size, then :kconfig:option:`CONFIG_PICOLIBC_HEAP_SIZE` (if not -1)
  must be set to a power of two value.

* Otherwise, the amount of free memory available is equal to the
  amount of unallocated memory in the SRAM region as determined at
  compile time.

The standard dynamic memory management interface functions implemented
by Picolibc are thread safe and may be simultaneously called by
multiple threads.
