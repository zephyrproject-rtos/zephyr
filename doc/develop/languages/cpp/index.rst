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

Header files and incompatibilities between C and C++
****************************************************

To interact with each other, C and C++ must share code through header
files: data structures, macros, static functions,...  While C and C++
have a large overlap, they're different languages with `known
incompatibilities`_. C is not just a C++ subset. Standard levels (e.g.:
"C+11") add another level of complexity as new features are often
inspired by and copied from the other language but many years later and
with subtle differences. Making things more complex, compilers often
offer early prototypes of features before they become
standardized. Standards can have ambiguities interpreted differently by
different compilers. Compilers can have bugs and these may need
workarounds. To help with this, many projects restrict themselves to a
limited number of toolchains. Zephyr does not.

These compatibility issues affect header files dis-proportionally.  Not
just because they have to be compatible between C and C++, but also
because they end up being compiled in a surprisingly high number of other
source files due to *indirect* inclusion and the `lack of structure and
headers organization`_ that is typical in real-world projects. So, header
files are exposed to a much larger variety of toolchains and project
configurations.
Adding more constraints, the Zephyr project has demanding policies
with respect to code style, compiler warnings, static analyzers and
standard compliance (e.g.: MISRA).

Put together, all these constraints can make writing header files very
challenging. The purpose of this section is to document some best "header
practices" and lessons learned in a Zephyr-specific context. While a lot
of the information here is not Zephyr-specific, this section is not a
substitute for knowledge of C/C++ standards, textbooks and other
references.

Testing
-------

Fortunately, the Zephyr project has an extensive test and CI
infrastructure that provides coverage baselines, catches issues early,
enforces policies and maintains such combinatorial explosions under some
control. The ``tests/lib/cpp/cxx/`` are very useful in this context
because their ``testcase.yaml`` configuration lets ``twister`` iterate
quickly over a range of ``-std`` parameters: ``-std=c++98``,
``-std=c++11``, etc.

Keep in mind unused macros are not compiled.

Designated initializers
-----------------------

Initialization macros are common in header files as they help reduce
boilerplate code. C99 added initialization of ``struct`` and ``union``
types by "designated" member names instead of a list of *bare*
expressions. Some GCC versions support designated initializers even in
their C90 mode.

When used at a simple level, designated initializers are less
error-prone, more readable and more flexible. On the other hand, C99
allowed a surprisingly large and lax set of possibilities: designated
initializers can be out of order, duplicated, "nested" (``.a.x =``),
various other braces can be omitted, designated initializers and not can
be mixed, etc.

Twenty years later, C++20 added designated initializers to C++ but in
much more restricted way; partly because a C++ ``struct`` is actually a
``class``. As described in the C++ proposal number P0329 (which compares
with C) or in any complete C++ reference, a mix is not allowed and
initializers must be in order (gaps are allowed).

Interestingly, the new restrictions in C++20 can cause ``gcc -std=c++20``
to fail to compile code that successfully compiles with
``gcc -std=c++17``. For example, ``gcc -std=c++17`` and older allow the
C-style mix of initializers and bare expressions. This fails to compile
with using ``gcc -std=c++20`` *with the same GCC version*.

Recommendation: to maximize compatibility across different C and C++
toolchains and standards, designated initializers in Zephyr header files
should follow all C++20 rules and restrictions. Non-designated, pre-C99
initialization offers more compatibility and is also allowed but
designated initialization is the more readable and preferred code
style. In any case, both styles must never be mixed in the same
initializer.

Warning: successful compilation is not the end of the incompatibility
story. For instance, the *evaluation order* of initializer expressions is
unspecified in C99! It is the (expected) left-to-right order in
C++20. Other standard revisions may vary. In doubt, do not rely on
evaluation order (here and elsewhere).

Anonymous unions
----------------

Anonymous unions (a.k.a. "unnamed" unions) seem to have been part of C++
from its very beginning. They were not officially added to C until C11.
As usual, there are differences between C and C++. For instance, C
supports anonymous unions only as a member of an enclosing ``struct`` or
``union``, empty lists ``{ }`` have always been allowed in C++ but they
require C23, etc.

When initializing anonymous members, the expression can be enclosed in
braces or not. It can be either designated or bare. For maximum
portability, when initializing *anonymous unions*:

- Do *not* enclose *designated* initializers with braces. This is
  required by C++20 and above which perceive such braces as mixing bare
  expressions with (other) designated initializers and fails to compile.

- Do enclose *bare* expressions with braces. This is required by C.
  Maybe because C is laxer and allows many initialization possibilities
  and variations, so it may need such braces to disambiguate? Note C
  does allow omitting most braces in initializer expressions - but not
  in this particular case of initializing anonymous unions with bare
  expressions.

Some pre-C11 GCC versions support some form of anonymous unions. They
unfortunately require enclosing their designated initializers with
braces which conflicts with this recommendation. This can be solved
with an ``#ifdef __STDC_VERSION__`` as demonstrated in Zephyr commit
`c15f029a7108
<https://github.com/zephyrproject-rtos/zephyr/commit/c15f029a7108>`_ and
the corresponding code review.


.. _`C++ Standard Library`: https://en.wikipedia.org/wiki/C%2B%2B_Standard_Library
.. _`Standard Template Library (STL)`: https://en.wikipedia.org/wiki/Standard_Template_Library
.. _`known incompatibilities`: https://en.wikipedia.org/wiki/Compatibility_of_C_and_C%2B%2B
..  _`lack of structure and headers organization`:
    https://github.com/zephyrproject-rtos/zephyr/issues/41543
.. _`gcc commit [C++ PATCH] P0329R4: Designated Initialization`:
    https://gcc.gnu.org/pipermail/gcc-patches/2017-November/487584.html
