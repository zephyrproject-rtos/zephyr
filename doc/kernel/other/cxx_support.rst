.. _cxx_support_v2:

C++ Support for Applications
############################

The kernel supports applications written in both C and C++. However, to
use C++ in an application you must configure the kernel to include C++
support and the build system must select the correct compiler.

The build system selects the C++ compiler based on the suffix of the files.
Files identified with either a **cxx** or a **cpp** suffix are compiled using
the C++ compiler. For example, :file:`myCplusplusApp.cpp` is compiled using C++.

The kernel currently provides only a subset of C++ functionality. The
following features are *not* supported:

* Dynamic object management with the **new** and **delete** operators
* :abbr:`RTTI (runtime type information)`
* Exceptions
* Static global object destruction

While not an exhaustive list, support for the following functionality is
included:

* Inheritance
* Virtual functions
* Virtual tables
* Static global object constructors

Static global object constructors are initialized after the drivers are
initialized but before the application :c:func:`main()` function. Therefore,
use of C++ is restricted to application code.

.. note::
    Do not use C++ for kernel, driver, or system initialization code.
