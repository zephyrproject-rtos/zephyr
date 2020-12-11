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

* :option:`CONFIG_CBPRINTF_FULL_INTEGRAL`
  or :option:`CONFIG_CBPRINTF_REDUCED_INTEGRAL`
* :option:`CONFIG_CBPRINTF_FP_SUPPORT`
* :option:`CONFIG_CBPRINTF_FP_A_SUPPORT`
* :option:`CONFIG_CBPRINTF_FP_ALWAYS_A`
* :option:`CONFIG_CBPRINTF_N_SPECIFIER`

:option:`CONFIG_CBPRINTF_LIBC_SUBSTS` can be used to provide functions
that behave like standard libc functions but use the selected cbprintf
formatter rather than pulling in another formatter from libc.

In addition :option:`CONFIG_CBPRINTF_NANO` can be used to revert back to
the very space-optimized but limited formatter used for :c:func:`printk`
before this capability was added.

.. warning::

  If :option:`CONFIG_MINIMAL_LIBC` is selected in combination with
  :option:`CONFIG_CBPRINTF_NANO` formatting with C standard library
  functions like ``printf`` or ``snprintf`` is limited.  Among other
  things the ``%n`` specifier, most format flags, precision control, and
  floating point are not supported.

API Reference
*************

.. doxygengroup:: cbprintf_apis
   :project: Zephyr
