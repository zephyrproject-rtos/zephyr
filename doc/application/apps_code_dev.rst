.. _apps_code_dev:

Add Application-Specific Code
#############################

Application-specific source code is typically added to an application
by placing it in the application directory itself (:file:`~/appDir`).
However, in some cases an application may also need to modify
or enhance the kernel itself.

.. contents:: Procedures
   :local:
   :depth: 1

Adding Source Code to an Application Directory
**********************************************

Understanding Source Code Requirements
======================================

Application-specific source code files are normally added to the application's
:file:`src` directory. If the application adds a large number of files
the developer can group them into sub-directories under :file:`src`,
to whatever depth is needed. The developer must supply a :file:`Makefile`
for the :file:`src` directory, as well as for each sub-directory under
:file:`src`.

.. note::

   These Makefiles are distinct from the top-level application Makefile
   that appears in :file:`~/appDir`.

Application-specific source code should not use symbol name prefixes
that have been reserved by the kernel for its own use.
For more information, see
`Naming Conventions <https://wiki.zephyrproject.org/view/Coding_conventions#Naming_Conventions>`_.

Understanding Makefile Requirements
===================================

The following requirements apply to all Makefiles in the :file:`src`
directory, including any sub-directories it may have.

* During the build process, Kbuild identifies the source files to compile
  into object files by matching the file names identified in
  the application's Makefile(s).

  .. important::

    A source file that is not referenced by any Makefile is not included
    when the application is built.

* A Makefile cannot directly reference source code. It can only
  reference object files (:file:`.o` files) produced from source code files.

* A Makefile can only reference files in its own directory or in the
  sub-directories of that directory.

* A Makefile may reference multiple files from a single-line entry.
  However, a separate line must be used to reference each directory.

Before You Begin
-----------------

* Ensure you have created an application directory, as described
  in :ref:`apps_structure`.

* The Zephyr environment variable must be set for each console
  terminal using :ref:`apps_common_procedures`.

* Many useful code examples cam be found at :file:`\$ZEPHYR_BASE/samples`.

Steps
-----

#. Create a directory structure for your source code in :file:`src`
   and add your source code files to it.

#. Create a :file:`Makefile` in the :file:`src` directory. Then create
   an additional :file:`Makefile` in each of the sub-directories under
   the :file:`src` directory, if any.

   a) Use the following syntax to add file references:

      .. code-block:: make

         obj-y += file1.o file2.o

   b) Use the following syntax to add directory references:

      .. code-block:: make

         obj-y += directory_name/**

Example src Makefile
--------------------

This example is taken from the Philosopher's Sample. To
examine this file in context, navigate to:
:file:`\$ZEPHYR_BASE/samples/philosophers/src`.

.. code-block:: make

   obj-y = main.o


Modifying and Enhancing the Kernel
**********************************

Subsystem Naming Conventions
============================

When enhancing an existing kernel subsystem be sure to follow
any existing naming conventions.
For more information, see
`Naming Conventions <https://wiki.zephyrproject.org/view/Coding_conventions#Naming_Conventions>`_.

When creating a new subsystem, the subsystem can define its own naming
conventions for symbols. However, naming conventions should be implemented
with a unique namespace prefix (e.g. bt\_ for BlueTooth, or net\_ for IP) to
limit possible clashes with applications. Naming within a subsystem
should continue to follow prefix conventions identified above; this
keeps consistent interface for all users.

Include Paths Usage Guidelines
==============================

Do not add unnecessary include paths to system or default include paths,
as this can lead to ambiguous references when two or more directories
contain a file with the same name.
The only include path into :file:`\$ZEPHYR_BASE/include` should be
:file:`\$ZEPHYR_BASE/include` itself.

Source files should use :code:`#include` directives that specify the full path
from a common include directory. For example, you should always use
directives of the form :code:`#include <path/to/[header].h>`, and not
use directives of the form :code:`#include <[header].h>`.

The following example illustrates the kind of problems that can arise when
unnecessary default include paths are specified.

* Observe that the kernel contains both :file:`include/pci.h` and
  :file:`include/drivers/pci.h`.

* If both the :file:`include` and :file:`include/drivers` directories
  are added to the default include paths (e.g.
  :code:`gcc -Iinclude/drivers -Iinclude [...]`), then the directive
  :code:`#include <pci.h>` becomes ambiguous.

The solution is to avoid adding :file:`include/drivers` to the default
include paths. Source files can then reference either :code:`#include <pci.h>`
or :code:`#include <drivers/pci.h>`, as needed.
