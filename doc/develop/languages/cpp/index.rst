.. _language_cpp:

C++ Language Support
####################

C++ is a general-purpose object-oriented programming language that is based on
the C language.

Enabling C++ Support
********************

Zephyr supports applications written in both C and C++. However, to use C++ in
an application you must configure Zephyr to include C++ support by selecting
the :kconfig:option:`CONFIG_CPP` in the application configuration file.

To enable C++ support, the compiler toolchain must also include a C++ compiler
and the included compiler must be supported by the Zephyr build system. The
:ref:`toolchain_zephyr_sdk`, which includes the GNU C++ Compiler (part of GCC),
is supported by Zephyr, and the features and their availability documented
here assume the use of the Zephyr SDK.

The default C++ standard level (i.e. the language enforced by the
compiler flags passed) for Zephyr apps is C++11.  Other standards are
available via kconfig choice, for example
:kconfig:option:`CONFIG_STD_CPP98`.  The oldest standard supported and
tested in Zephyr is C++98.

When compiling a source file, the build system selects the C++ compiler based
on the suffix (extension) of the files. Files identified with either a **cpp**
or a **cxx** suffix are compiled using the C++ compiler. For example,
:file:`myCplusplusApp.cpp` is compiled using C++.

The C++ standard requires the ``main()`` function to have the return type of
``int``. Your ``main()`` must be defined as ``int main(void)``. Zephyr ignores
the return value from main, so applications should not return status
information and should, instead, return zero.

.. note::
    Do not use C++ for kernel, driver, or system initialization code.

Language Features
*****************

Zephyr currently provides only a subset of C++ functionality. The following
features are *not* supported:

* Static global object destruction
* OS-specific C++ standard library classes (e.g. ``std::thread``,
  ``std::mutex``)

While not an exhaustive list, support for the following functionality is
included:

* Inheritance
* Virtual functions
* Virtual tables
* Static global object constructors
* Dynamic object management with the **new** and **delete** operators
* Exceptions
* :abbr:`RTTI (runtime type information)`
* Standard Template Library (STL)

Static global object constructors are initialized after the drivers are
initialized but before the application :c:func:`main()` function. Therefore,
use of C++ is restricted to application code.

In order to make use of the C++ exceptions, the
:kconfig:option:`CONFIG_CPP_EXCEPTIONS` must be selected in the application
configuration file.

Zephyr Minimal C++ Library
**************************

Zephyr minimal C++ library (:file:`lib/cpp/minimal`) provides a minimal subset
of the C++ standard library and application binary interface (ABI) functions to
enable basic C++ language support. This includes:

* ``new`` and ``delete`` operators
* virtual function stub and vtables
* static global initializers for global constructors

The scope of the minimal C++ library is strictly limited to providing the basic
C++ language support, and it does not implement any `Standard Template Library
(STL)`_ classes and functions. For this reason, it is only suitable for use in
the applications that implement their own (non-standard) class library and do
not rely on the Standard Template Library (STL) components.

Any application that makes use of the Standard Template Library (STL)
components, such as ``std::string`` and ``std::vector``, must enable the C++
standard library support.

C++ Standard Library
********************

The `C++ Standard Library`_ is a collection of classes and functions that are
part of the ISO C++ standard (``std`` namespace).

Zephyr does not include any C++ standard library implementation in source code
form. Instead, it allows configuring the build system to link against the
pre-built C++ standard library included in the C++ compiler toolchain.

To enable C++ standard library, select an applicable toolchain-specific C++
standard library type from the :kconfig:option:`CONFIG_LIBCPP_IMPLEMENTATION`
in the application configuration file.

For instance, when building with the :ref:`toolchain_zephyr_sdk`, the build
system can be configured to link against the GNU C++ Library (``libstdc++.a``),
which is a fully featured C++ standard library that provides all features
required by the ISO C++ standard including the Standard Template Library (STL),
by selecting :kconfig:option:`CONFIG_GLIBCXX_LIBCPP` in the application
configuration file.

The following C++ standard libraries are supported by Zephyr:

* GNU C++ Library (:kconfig:option:`CONFIG_GLIBCXX_LIBCPP`)
* ARC MetaWare C++ Library (:kconfig:option:`CONFIG_ARCMWDT_LIBCPP`)

A Zephyr subsystem that requires the features from the full C++ standard
library can select, from its config,
:kconfig:option:`CONFIG_REQUIRES_FULL_LIBCPP`, which automatically selects a
compatible C++ standard library unless the Kconfig symbol for a specific C++
standard library is selected.

.. _`C++ Standard Library`: https://en.wikipedia.org/wiki/C%2B%2B_Standard_Library
.. _`Standard Template Library (STL)`: https://en.wikipedia.org/wiki/Standard_Template_Library
