.. _kbuild_project:

Developing an Application Project Using Kbuild
**********************************************

The build system is a project centric system. The |codename|
must have a defined project that allows the build of the kernel
binary.

A project can be organized in three independent directories: the
kernel base directory, the project directory, and the project source
code directory.

The kernel base directory is the |codename| base directory. This directory
hosts the kernel source code, the configuration options and the kernel
build definitions.

The project directory is the directory that puts together the kernel
project with the developer project. This directory hosts the definition
of the developers' projects, for example: specific configuration options
and file location of the source code.

The project source code directory hosts the source code of the
developers' projects.

The Application Project
=======================

A common application project is composed of the following files:

* Makefile: Configures the project and integrates the
  developer's project with the |codename| Kbuild system.

* Configuration file: Allows the user
  to override the standard configuration.

* MDEF file: Defines the set of kernel objects instantiated by the
  project. Required only by microkernel architectures.

* The :file:`src/` directory: Hosts by default the project source code.
   This directory can be overridden and defined in a different
   directory.

   * Makefile: Adds the developer's source code into the Kbuild
     recursion model.

The project source code can be organized in subdirectories.
Each directory should follow the Kbuild Makefile conventions.

Project Definition
==================

The application project is defined through the project Makefile.
The project is integrated into the Kbuild system by including the
Makefile.inc file provided.

.. code-block:: make

   include $(ZEPHYR_BASE)/Makefile.inc

The following predefined variables configure the development project:

* :makevar:`ARCH`: Specifies the build architecture for the |codename|
  The architectures currently supported are x86, arm and arc. The build
  system sets C flags, selects the default configuration and the
  toolchain tools. The default value is x86.

* :makevar:`ZEPHYR_BASE`: Sets the path to the |project| base directory.
  This variable is usually set by the :file:`timo_env.sh` script.
  It can be used to get the |project| base directory (as used in the
  Makefile.inc inclusion) or it can be overridden to select an
  specific instance of the |codename|.

* :makevar:`PROJECT_BASE`: Provides the developer
  project directory path. It is set by the :file:`Makefile.inc`,

* :makevar:`SOURCE_DIR`: Overrides the default value for the
  developer source code directory. The developer source code directory
  is set to :file:`$(PROJECT_BASE/)src/` by default. This directory
  name should end with backslash **'/'**.

* :makevar:`PLATFORM_CONFIG`:Selects the platform of the default
  configuration used by the project build.

* :makevar:`KERNEL_TYPE`: Select the kernel type of the default
  configuration used by the project build. It indicates if this is
  a nanokernel or microkernel architecture. The supported values
  are **nano** and **micro**.

* :makevar:`MDEF_FILE`: Indicates the name of the MDEF file. It is
  required for microkernel architectures only.

* :makevar:`CONF_FILE`: Indicates the name of a configuration
  snippet file. This file includes the kconfig values that are
  overridden from the default configuration.

* :makevar:`O`: Optional. Indicates the output directory used by Kconfig.
  The output directory stores all the Kconfig generated files.
  The default output directory is set to the
  :file:`$(PROJECT_BASE)/output` directory.
