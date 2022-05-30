.. _language_c:

C Language Support
##################

C is a general-purpose low-level programming language that is widely used for
writing code for embedded systems.

Zephyr is primarily written in C and natively supports applications written in
the C language. All Zephyr API functions and macros are implemented in C and
available as part of the C header files under the :file:`include` directory, so
writing Zephyr applications in C gives the developers access to the most
features.

.. _c_standards:

Language Standards
******************

Zephyr does not target a specific version of the C standards; however, the
Zephyr codebase makes extensive use of the features newly introduced in the
1999 release of the ISO C standard (ISO/IEC 9899:1999, hereinafter referred to
as C99) such as those listed below, effectively requiring the use of a compiler
toolchain that supports the C99 standard and above:

* inline functions
* standard boolean types (``bool`` in ``<stdbool.h>``)
* fixed-width integer types (``[u]intN_t`` in ``<stdint.h>``)
* designated initializers
* variadic macros
* ``restrict`` qualification

Some Zephyr components make use of the features newly introduced in the 2011
release of the ISO C standard (ISO/IEC 9899:2011, hereinafter referred to as
C11) such as the type-generic expressions using the ``_Generic`` keyword. For
example, the :c:func:`cbprintf` component, used as the default formatted output
processor for Zephyr, makes use of the C11 type-generic expressions, and this
effectively requires most Zephyr applications to be compiled using a compiler
toolchain that supports the C11 standard and above.

In summary, it is recommended to use a compiler toolchain that supports at
least the C11 standard for developing with Zephyr. It is, however, important to
note that some optional Zephyr components and external modules may make use of
the C language features that have been introduced in more recent versions of
the standards, in which case it will be necessary to use a more up-to-date
compiler toolchain that supports such standards.

.. _c_library:

Standard Library
****************

The `C Standard Library`_ is an integral part of any C program, and Zephyr
provides the support for a number of different C libraries for the applications
to choose from, depending on the compiler toolchain being used to build the
application.

.. toctree::
   :maxdepth: 2

   minimal_libc.rst
   newlib.rst

.. _`C Standard Library`: https://en.wikipedia.org/wiki/C_standard_library
