.. _formatted_output:

Formatted Output
################

Applications as well as Zephyr itself requires infrastructure to format
values for user consumption.  The standard C99 library ``*printf()``
functionality fulfills this need for streaming output devices or memory
buffers, but in an embedded system devices may not accept streamed data
and memory may not be available to store the formatted output.

Internal Zephyr API traditionally provided this both for
:c:func:`printk` and for Zephyr's internal minimal libc, but with
separate internal interfaces.  Logging, tracing, shell, and other
applications made use of either these APIs or standard libc routines
based on build options.

The :c:func:`cbprintf` public APIs convert C99 format strings and
arguments, providing output produced one character at a time through a
callback mechanism, replacing the original internal functions and
providing support for almost all C99 format specifications.  Existing
use of ``s*printf()`` C libraries in Zephyr can be converted to
:c:func:`snprintfcb()` to avoid pulling in libc implementations.

Several Kconfig options control the set of features that are enabled,
allowing some control over features and memory usage:

* :kconfig:option:`CONFIG_CBPRINTF_FULL_INTEGRAL`
  or :kconfig:option:`CONFIG_CBPRINTF_REDUCED_INTEGRAL`
* :kconfig:option:`CONFIG_CBPRINTF_FP_SUPPORT`
* :kconfig:option:`CONFIG_CBPRINTF_FP_A_SUPPORT`
* :kconfig:option:`CONFIG_CBPRINTF_FP_ALWAYS_A`
* :kconfig:option:`CONFIG_CBPRINTF_N_SPECIFIER`

:kconfig:option:`CONFIG_CBPRINTF_LIBC_SUBSTS` can be used to provide functions
that behave like standard libc functions but use the selected cbprintf
formatter rather than pulling in another formatter from libc.

In addition :kconfig:option:`CONFIG_CBPRINTF_NANO` can be used to revert back to
the very space-optimized but limited formatter used for :c:func:`printk`
before this capability was added.

.. _cbprintf_packaging:

Cbprintf Packaging
******************

Typically, strings are formatted synchronously when a function from ``printf``
family is called. However, there are cases when it is beneficial that formatting
is deferred. In that case, a state (format string and arguments) must be captured.
Such state forms a self-contained package which contains format string and
arguments. Additionally, package may contain copies of strings which are
part of a format string (format string or any ``%s`` argument). Package primary
content resembles va_list stack frame thus standard formatting functions are
used to process a package. Since package contains data which is processed as
va_list frame, strict alignment must be maintained. Due to required padding,
size of the package depends on alignment. When package is copied, it should be
copied to a memory block with the same alignment as origin.

Package can have following variants:

* **Self-contained** - non read-only strings appended to the package. String can be
  formatted from such package as long as there is access to read-only string
  locations. Package may contain information where read-only strings are located
  within the package. That information can be used to convert packet to fully
  self-contained package.
* **Fully self-contained** - all strings are appended to the package. String can be
  formatted from such package without any external data.
* **Transient**- only arguments are stored. Package contain information
  where pointers to non read-only strings are located within the package. Optionally,
  it may contain read-only string location information. String can be formatted
  from such package as long as non read-only strings are still valid and read-only
  strings are accessible. Alternatively, package can be converted to **self-contained**
  package or **fully self-contained** if information about read-only string
  locations is present in the package.

Package can be created using two methods:

* runtime - using :c:func:`cbprintf_package` or :c:func:`cbvprintf_package`. This
  method scans format string and based on detected format specifiers builds the
  package.
* static - types of arguments are detected at compile time by the preprocessor
  and package is created as simple assignments to a provided memory. This method
  is significantly faster than runtime (more than 15 times) but has following
  limitations: requires ``_Generic`` keyword (C11 feature) to be supported by
  the compiler and can only create a package that is known to have no string
  arguments (``%s``). :c:macro:`CBPRINTF_MUST_RUNTIME_PACKAGE` can be used to
  determine at compile time if static packaging can be applied. Macro determines
  need for runtime packaging based on presence of char pointers in the argument
  list so there are cases when it will be false positive, e.g. ``%p`` with char
  pointer.

Several Kconfig options control behavior of the packaging:

* :kconfig:option:`CONFIG_CBPRINTF_PACKAGE_LONGDOUBLE`
* :kconfig:option:`CONFIG_CBPRINTF_STATIC_PACKAGE_CHECK_ALIGNMENT`

Cbprintf package conversion
===========================

It is possible to convert package to a variant which contains more information, e.g
**transient** package can be converted to **self-contained**. Conversion to
**fully self-contained** package is possible if :c:macro:`CBPRINTF_PACKAGE_ADD_RO_STR_POS`
flag was used when package was created.

:c:func:`cbprintf_package_copy` is used to calculate space needed for the new
package and to copy and convert a package.

Cbprintf package format
=======================

Format of the package contains paddings which are platform specific. Package consists
of header which contains size of package (excluding appended strings) and number of
appended strings. It is followed by the arguments which contains alignment paddings
and resembles *va_list* stack frame. Finally, package optionally contains appended
strings. Each string contains 1 byte header which contains index of the location
where address argument is stored. During packaging address is set to null and
before string formatting it is updated to point to the current string location
within the package. Updating address argument must happen just before string
formatting since address changes whenever package is copied.

+------------------+-------------------------------------------------------------------------+
| Header           | 1 byte: Argument list size including header and *fmt* (in 32 bit words) |
|                  +-------------------------------------------------------------------------+
| sizeof(void \*)  | 1 byte: Number of strings appended to the package                       |
|                  +-------------------------------------------------------------------------+
|                  | 1 byte: Number of read-only string argument locations                   |
|                  +-------------------------------------------------------------------------+
|                  | 1 byte: Number of transient string argument locations                   |
|                  +-------------------------------------------------------------------------+
|                  | platform specific padding to sizeof(void \*)                            |
+------------------+-------------------------------------------------------------------------+
| Arguments        | Pointer to *fmt* (or null if *fmt* is appended to the package)          |
|                  +-------------------------------------------------------------------------+
|                  | (optional padding for platform specific alignment)                      |
|                  +-------------------------------------------------------------------------+
|                  | argument 0                                                              |
|                  +-------------------------------------------------------------------------+
|                  | (optional padding for platform specific alignment)                      |
|                  +-------------------------------------------------------------------------+
|                  | argument 1                                                              |
|                  +-------------------------------------------------------------------------+
|                  | ...                                                                     |
+------------------+-------------------------------------------------------------------------+
| String location  | Indexes of words within the package where read-only strings are located |
| information      +-------------------------------------------------------------------------+
| (optional)       | Indexes of words within the package where transient strings are located |
+------------------+-------------------------------------------------------------------------+
| Appended         | 1 byte: Index within the package to the location of associated argument |
| strings          +-------------------------------------------------------------------------+
| (optional)       | Null terminated string                                                  |
|                  +-------------------------------------------------------------------------+
|                  | ...                                                                     |
+------------------+-------------------------------------------------------------------------+

.. warning::

  If :kconfig:option:`CONFIG_MINIMAL_LIBC` is selected in combination with
  :kconfig:option:`CONFIG_CBPRINTF_NANO` formatting with C standard library
  functions like ``printf`` or ``snprintf`` is limited.  Among other
  things the ``%n`` specifier, most format flags, precision control, and
  floating point are not supported.

API Reference
*************

.. doxygengroup:: cbprintf_apis
