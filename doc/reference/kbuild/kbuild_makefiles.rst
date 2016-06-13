.. _kbuild_makefiles:

The Makefiles
*************

Overview
========

The build system defines a set of conventions for the correct use of Makefiles
in the kernel source directories. The correct use of Makefiles is driven by the
concept of recursion.

In the recursion model, each Makefile within a directory includes the source
code and any subdirectories to the build process. Each subdirectory follows
the same principle. Developers can focus exclusively in their own work. They
integrate their module with the build system by adding a very simple Makefile
following the recursive model.

.. _makefile_conventions:

Makefile Conventions
====================

The following conventions restrict how to add modules and Makefiles to the
build system. These conventions guard the correct implementation of the
recursive model.

* Each source code directory must contain a single Makefile. Directories
  without a Makefile are not considered source code directories.

* The scope of every Makefile is restricted to the contents of that directory.
  A Makefile can only make a direct reference to its own files and subdirectories.
  Any file outside the directory has to be referenced in its home directory Makefile.

* Makefiles list the object files that are included in the link process. The
  build system finds the source file that generates the object file by matching
  the file name.

* Parent directories add their child directories into the recursion model.

* The root Makefile adds the directories in the kernel base directory into the
  recursion model.


Adding Source Files
===================
A source file is added to the build system through its home directory Makefile.
The Makefile must refer the source build indirectly, specifying the object file
that results from the source file using the :literal:`obj-y` variable. For
example, if the file that we want to add is a C file named :file:`<file>.c` the
following line should be added in the Makefile:

.. code-block:: make

   obj-y += <file>.o

.. note::

   The same method applies for assembly files with .s extension.

Source files can be added conditionally using configuration options.  For
example, if the option ``CONFIG_VAR`` is set and it implies that a source
file must be added in the compilation process, then the following line adds the
source code conditionally:

.. code-block:: none

   obj-$(CONFIG_VAR) += <file>.o

Adding Directories
==================

Add a subdirectory to the build system by editing the Makefile in its
directory.  The subdirectory is added using the :literal:`obj-y` variable. The
correct syntax to add a subdirectory into the recursion is:

.. code-block:: make

   obj-y += <directory_name>/

The backslash at the end of the directory name signals the build system that a
directory, and not a file, is being added to the recursion.

The conventions require us to add only one directory per line and never to mix
source code with directory recursion in a single :literal:`obj-y` line. This
helps keep the readability of the Makefile by making it clear when an item adds
an additional lever of recursion.

Directories can also be conditionally added:

.. code-block:: none

   obj-y-$(CONFIG_VAR) += <directory_name>/

The subdirectory must contain its own Makefile following the rules described in
:ref:`makefile_conventions`.
