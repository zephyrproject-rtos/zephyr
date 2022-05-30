.. _language_cpp:

C++ Language Support
####################

C++ is a general-purpose object-oriented programming language that is based on
the C language.

Enabling C++ Support
********************

Zephyr supports applications written in both C and C++. However, to use C++ in
an application you must configure Zephyr to include C++ support by selecting
the :kconfig:option:`CONFIG_CPLUSPLUS` in the application configuration file.

To enable C++ support, the compiler toolchain must also include a C++ compiler
and the included compiler must be supported by the Zephyr build system. The
:ref:`toolchain_zephyr_sdk`, which includes the GNU C++ Compiler (part of GCC),
is supported by Zephyr, and the features and their availability documented
here assume the use of the Zephyr SDK.

When compiling a source file, the build system selects the C++ compiler based
on the suffix (extension) of the files. Files identified with either a **cpp**
or a **cxx** suffix are compiled using the C++ compiler. For example,
:file:`myCplusplusApp.cpp` is compiled using C++.

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
:kconfig:option:`CONFIG_EXCEPTIONS` must be selected in the application
configuration file.

Zephyr C++ Subsystem
********************

Zephyr C++ subsystem (:file:`subsys/cpp`) provides a minimal subset of the C++
standard library and application binary interface (ABI) functions to enable
basic C++ language support. This includes:

* ``new`` and ``delete`` operators
* virtual function stub and vtables
* static global initializers for global constructors

The scope of the C++ subsystem is strictly limited to providing the basic C++
language support, and it does not implement any `Standard Template Library
(STL)`_ classes and functions. For this reason, it is only suitable for use in
the applications that implement their own (non-standard) class library and do
rely on the Standard Template Library (STL) components.

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

For instance, when building with the :ref:`toolchain_zephyr_sdk`, the build
system can be configured to link against the GNU C++ Standard Library
(``libstdc++.a``) included in the Zephyr SDK, which is a fully featured C++
standard library that provides all features required by the ISO C++ standard
including the Standard Template Library (STL).

To enable C++ standard library, select the
:kconfig:option:`CONFIG_LIB_CPLUSPLUS` in the application configuration file.

.. _`C++ Standard Library`: https://en.wikipedia.org/wiki/C%2B%2B_Standard_Library
.. _`Standard Template Library (STL)`: https://en.wikipedia.org/wiki/Standard_Template_Library
