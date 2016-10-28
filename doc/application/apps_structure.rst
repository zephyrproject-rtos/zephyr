.. _apps_structure:

Create an Application Directory
###############################

Each application resides in a uniquely-named application
directory created by the developer, typically in the developer's
workspace directory. The application developer also creates a
:file:`src` directory for the application's source code.

.. contents:: Procedures
   :local:
   :depth: 1

Creating an Application and Source Code Directory
=================================================

Create one directory for your application and a sub-directory for the
application's source code; this makes it easier to organize directories
and files in the structure that the kernel expects.

Before You Begin
----------------

* Ensure the Zephyr environment variables are set for each console terminal;
  see :ref:`apps_common_procedures`.

Steps
-----

1. Create an application directory structure outside of the kernel's
   installation directory tree. Often this is your workspace directory.

 a) In a console terminal, navigate to a location where you want your
    application to reside.

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

Creating an Application Makefile
================================

Create an application Makefile to define basic information,
such as the board configuration used by the application.
The build system uses the Makefile to build a :file:`zephyr.elf` image
that contains both the application and the kernel libraries.

Before You Begin
----------------

* Be familiar with the standard GNU Make language.

* Be familiar with the board configuration used for your application
  and, if it is a custom board configuration, where it is located.

* Ensure the Zephyr environment variables are set for each console terminal;
  see :ref:`apps_common_procedures`.

Steps
-----

1. In the :file:`appDir` directory, create a Makefile. Enter:

   .. code-block:: bash

      $ touch Makefile

2. Open the :file:`Makefile` and add the following mandatory
   entries using any standard text editor.

   .. note::

      Ensure that there is a space after each ``=``.

   a) Add the name of the default board configuration for your application on a
      new line:

      .. code-block:: make

         BOARD ?= board_configuration_name

      The supported boards can be found in :ref:`board`.

   b) Add the name of the default kernel configuration file for your
      application on a new line:

      .. code-block:: make

         CONF_FILE ?= kernel_configuration_name

      The default kernel configuration file entry may be omitted if the file
      is called :file:`prj.conf`. It may also be omitted if the default board
      configuration's kernel settings are sufficient for your application.

   c) Include the mandatory :file:`Makefile` fragments on a new line:

      .. code-block:: make

         include ${ZEPHYR_BASE}/Makefile.inc

3. Save and close the :file:`Makefile`.

Example Makefile
----------------

.. code-block:: make

   BOARD ?= qemu_x86
   CONF_FILE ?= prj.conf
   include ${ZEPHYR_BASE}/Makefile.inc
