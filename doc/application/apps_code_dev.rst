.. _apps_code_dev:

Application Code Development
############################

.. contents::
   :local:
   :depth: 1

.. _develop_services:

Services
********

The |codename| services architecture has services that are
specific to the microkernel, services that are specific to the
nanokernel, and services that are common, or shared, between the
two.

Microkernel Services
====================

For a complete list of microkernel services, including a description
of each with code examples, see :ref:`microkernel`.

.. _note::

   There are more microkernel services than those defined in
   the MDEF.

Nanokernel Services
===================

For a complete list of nanokernel services, including a description
of each with code examples, see :ref:`nanokernel`.

Common Services
===============

For a complete list of services common to both the nanokernel and
microkernel, including a description of each with code examples,
see :ref:`common_kernel_services`.


Procedures and Conventions
**************************

Understanding Naming Conventions
================================

The kernel limits the use of some prefixes to internal use only. For
more information, see :ref:`naming_conventions`.

.. _src_makefiles_reqs:

Understanding src Directory Makefile Requirements
=================================================

The src directory needs a Makefile to specify how to build the application
source code. Likewise, any sub-directory within src must also have its own
Makefile. The following requirements apply to all Makefiles in the src
directory:

* A Makefile must reference only its own files and sub-directories.

* Code that references a file outside the directory cannot be included in the
  Makefile; the referenced file is included only in its own directory's
  Makefile.

* A Makefile cannot directly reference source code; it can only
  reference object files (.o files) produced by source code.

 .. _note::

   The src directory Makefiles discussed here are distinct from
   the top-level application Makefile.

.. _src_files_directories:

Adding Source Files and Makefiles to src Directories
====================================================

Source code and associated Makefiles specify the how the
application image is built. In a Makefile, multiple files may be
referenced from a single-line entry. However, a separate line must
be used to reference each directory. During the build process, Kbuild
finds the source files to generate the object files by matching the
file names identified in the Makefile.

.. _note::

   Source code without an associated Makefile is not included
   when the application is built.

Before You Begin
-----------------

* The Zephyr environment variable must be set for each console
  terminal using :ref:`apps_common_procedures`.

Steps
-----

1. Create a directory structure for your source code in :file:`src`
   and add your source code files to it.

  For many useful code examples, see :ref:`common_kernel_services`,
  :ref:`microkernel`, and :ref:`nanokernel`.

2. Create a :file:`Makefile` for each :file:`src` directory.

   a) Add the following line to each :file:`Makefile`:

      .. code-block:: make

         ccflags-y += ${PROJECTINCLUDE}


   b) Use the following syntax to add file references:

      .. code-block:: make

         obj-y += file.o file.o


   c) Use the following syntax to add directory references:

      .. code-block:: make

         obj-y += directory_name/**


Example src Makefile
--------------------

This example is taken from the Philosopher's Sample. To
examine this file in context, navigate to:
:file:`rootDir/samples/microkernel/apps/philosophers/src`.

.. code-block:: make

   ccflags-y += ${PROJECTINCLUDE}
   obj-y = phil_fiber.o phil_task.o


.. _`enhancing_kernel`:

Enhancing the Zephyr Kernel
===========================

When enhancing the Zephyr kernel, follow the subsystem naming
conventions and the :literal:`include path` usage guidelines.

Subsystem Naming Conventions
----------------------------

In general, any sub-system can define its own naming conventions for
symbols. However, naming conventions should be implemented with a
unique namespace prefix (e.g. bt\_ for BlueTooth, or net\_ for IP) to
limit possible clashes with applications. Naming within a sub-system
should continue to follow prefix conventions identified above; this
keeps consistent interface for all users.

Include Paths Usage Guidelines
------------------------------

The current build system uses a series of defs.objs to define
common pieces for a specific subsystem. For example, there
are common defines for all architectures under :file:`\$ROOT/arch/x86`,
and more specific defines for each supported board within
the architecture, such as, :file:`\$ROOT/arch/x86/generic_pc`.

Do not add every possible :literal:`include paths` in the defs.obj files.
Too many default paths will cause problems when more than one file with
the same name exists. The only :literal:`include path` into
:file:`\${vBASE}/include` should be :file:`\${vBASE}/include` itself.

Files should define includes to specific files with the complete path
:file:`#include subdirectory/header.h`. For example, when there
are two files, :file:`include/pci.h` and :file:`include/drvers/pci.h`,
and both have been set to :file:`-Iinclude/drivers` and
:file:`-Iinclude` for the compile, any code using
:file:`#include pci.h` becomes ambiguous; using the complete path
:file:`#include drivers/pci.h` is not. Not having :file:`-Iinclude/drivers`
forces users to use the second form, which is more explicit.
