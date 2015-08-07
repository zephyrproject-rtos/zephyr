.. _kbuild_makefiles:

The Makefiles
*************

Overview
========

Kbuild defines a set of conventions about the correct use of Makefiles
in the kernel source directories. The correct use of Makefiles is
mainly driven by the concept of recursion.

In the recursion model, each directory describes the source code that
is introduced in the build process and the subdirectories that
participate in it. Each subdirectory follows the same
principle. The developer focus exclusively in his own
work. The developer describes how his module is integrated
in the build system and plugs a very simple Makefile following the
recursive model.

Makefile Conventions
====================

Kbuild states the following conventions that restrict the different
ways that modules and Makefiles can be added into the system. This
conventions guard the correct implementation of the recursive model.

* There must be a single Makefile per directory. Without a
  Makefile in the directory it is not considered a source code
  directory.

* The scope of every Makefile is restricted to the content of that
  directory. A Makefile can only make a direct reference to its own
  files and subdirectories. Any file outside the directory has
  to be referenced in its home directory Makefile.

* Makefiles list the object files that are included in the link
  process. The Kbuild finds the source file that generates the
  object file by matching the file name.

* Parent directories add their child directories into the recursion
  model.

* The root Makefile adds the directories in the kernel base
  directory into the recursion model.


Adding source files
===================
A source file is added into the build system by editing its home
directory Makefile. The Makefile must refer the source build
indirectly, specifying the object file that results from the source
file using the :literal:`obj-y` variable. For example, if the file that we
want to add is a C file named :file:`<file>.c` the following line should be
added in the Makefile:

.. code-block:: make

   obj-y += <file>.o

.. note::

   The same applies for assembly files with .s extension.

The source files can be conditionally added using Kconfig variables.
For example, if the symbol :literal:`CONFIG_VAR` is set and this implies that
a source file must be added in the compilation process, then the
following line adds the source code conditionally:

.. code-block:: make

   obj-$(CONFIG_VAR) += <file>.o

Adding new directories
======================

A new directory is added into the build system editing its home
directory Makefile. The directory is added using the :literal:`obj-y`
variable. The syntax to indicate that we are adding a directory into
the recursion is:

.. code-block:: make

   obj-y += <directory_name>/**

The backslash at the end of the directory name denotes that the
name is a directory that is added into the recursion.

The convention require us to add only one directory per text line and
never to mix source code with directory recursion in a single
:literal:`obj-y` line. This helps keep the readability of the
Makefile by making it clear when an item adds an additional lever of
recursion.

Directories can be conditionally added as well, just like source files:

.. code-block:: make

   oby-$(CONFIG_VAR) += <directory_name>/

The new directory must contain its own Makefile following the rules
described in `Makefil Conventions`_

Adding new root directories
===========================

Root directories are the directories inside the |project|
base directory like the :file:`arch/`, :file:`kernel/` and the
:file:`driver/` directories.

The parent directory for this directories is the root Makefile. The
root Makefile is the located at the |project|'s base directory.
The root Makefile defines the variable :literal:`core-y` which lists
all the root directories that are at the root of recursion.

To add a new root directory, include the directory name in the list.
For example:

.. code-block:: make

   core-y += <directory_name/>

There is the possibility that the new directory requires an specific
variable, different from :literal:`core-y`. In order to keep the integrity
and organization of the project, a change of this sort should be
consulted with the project maintainer.