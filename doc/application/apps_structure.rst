.. _apps_structure:

Application Development Directory Structure
###########################################

Each application resides in a uniquely-named application
directory created by the developer, typically, in the developer's
workspace directory. The application developer also creates a
:file:`src` directory for the application's source code.

.. note::

   The Zephyr Kernel either supplies or generates all other application
   directories.

Procedures
**********

* `create_directory_structure`_

* `create_src_makefile`_

.. _create_directory_structure:

Creating an Application and Source Code Directory using the CLI
===============================================================

Create one directory for your application and another for the application's
source code; this makes it easier to organize directories and files in the
structure that the kernel expects.

Before You Begin
----------------

* The environment variable must be set for each console terminal using
  :ref:`set_environment_variables`.

Steps
-----

1. Create an application directory structure outside of the kernel's
   installation directory tree. Often this is your workspace directory.

 a) In a Linux console, navigate to a location where you want your
    applications to reside.

 b) Create the application's directory, enter:

   .. code-block:: console

      $ mkdir application_name

   .. note::

      This directory and the path to it, are referred to in the documentation
      as :file:`~/appDir`.

2. Create a source code directory in your :file:`~/appDir`, enter:

   .. code-block:: console

      $ mkdir src

   The source code directory :file:`~/appDir/src` is created.

   .. code-block:: console

      -- appDir
         |-- src

.. _create_src_makefile:

Creating an Application Makefile
================================

Create an application Makefile to define basic information such as the kernel
type, microkernel or nanokernel, and the platform configuration used by the
application. The build system uses the Makefile to build an image with both
the application and the kernel libraries called either
:file:`microkernel.elf` or :file:`nanokernel.elf`.

Before You Begin
----------------

* Be familiar with the standard GNU Make language.

* Be familiar with the platform configuration used for your application
  and, if it is a custom platform configuration, where it is located.

* Set the environment variable for each console terminal using
  :ref:`set_environment_variables`.

Steps
-----

1. In the :file:`appDir` directory, create a Makefile. Enter:

   .. code-block:: bash

      $ touch Makefile

2. Open the :file:`Makefile` and add the following mandatory
   entries using any standard text editor.

   .. note::

      Ensure that there is a space after each ``=``.

   a) Add the kernel type on a new line:

      .. code-block:: make

         KERNEL_TYPE = micro|nano

      Either micro or nano, short for microkernel or
      nanokernel respectively.

   b) Add the name of the platform configuration for your application on a
      new line:

      .. code-block:: make

         BOARD ?= platform_configuration_name

      The supported platforms can be found in :ref:`platform`.

   c) Add the name of the default kernel configuration file for your
      application on a new line:

      .. code-block:: make

         CONF_FILE = prj.conf

      The default name is :file:`prj.conf`. If you are not using the default
      name, this entry must match the filename of the :file:`.conf` file you
      are using.

   d) For microkernel applications, add the name of the MDEF for your
      application:

      .. code-block:: make

         MDEF_FILE = prj.mdef

      The default name is :file:`prj.mdef`. If you are not using the default
      name, this entry must match the filename of the :file:`.mdef` file you
      are using.

   e) Include the mandatory :file:`Makefile` fragments on a new
      line:

      .. code-block:: make

         include ${ZEPHYR_BASE}/Makefile.inc

3. Save and close the :file:`Makefile`.

Example Makefile
----------------

.. code-block:: make

   KERNEL_TYPE = micro
   BOARD ?= qemu_x86
   CONF_FILE = prj.conf
   MDEF_FILE = prj.mdef
   include ${ZEPHYR_BASE}/Makefile.inc
