.. _Kbuild:

Kbuild User Guide
#################

The |project| build system is based on the Kbuild system used in the
Linux kernel (version 3.19-rc7). This way the |codename| embraces
the recursive model proposed by Linux and the configuration model
proposed by Kconfig.

The main difference between the Linux Kbuild system and the |codename|
build system is that the build is project centered. The consequence
is the need to define a project or application. The project drives
the build of the operating system.
For that reason, the build structure follows the Kbuild
architecture but it requires a project directory. The project
directory hosts the project's definition and code. Kbuild build the
kernel around the project directory.

Scope
*****

This document provides the details of the Kbuild implementation. It is
intended for application developers wanting to use the |project| as a
development platform and for kernel developers looking to modify or
expand the kernel functionality.

Kconfig Structure
*****************

Introduction
============
Kconfig files describe the configuration symbols supported in the build
system, the logical organization and structure to group the symbols in
menus and sub-menus, and the relationships between the different
configuration symbols that govern the valid configuration combinations.

The Kconfig files are distributed across the build directory tree. These
are common characteristics followed by several examples to
organize the Kconfig files and to add new symbols configuration menus:

   * The Kconfig file at the root directory holds general
     configuration options like the ARCH symbol and the KERNEL VERSION.
     These symbols are defined and apply to the entire build system.

   * The Kconfig file at the kernel directory holds the general
     configuration related to the micro and the nanokernel.
     These are some configuration options hosted here:
     **Kernel Type** and **Enhanced Security**.

   * The Kconfig file at the driver directory organizes the inclusion of
     the different Kconfig files for every driver supported in the system.

   * The Kconfig file at the misc directory contains the configuration
     symbols that affect different aspects of the build system. For
     example, the **Custom Compiler Options** and the **Minimal Libc**
     are general build options that apply to the build system.
     **Debugging Options** and **System Monitoring Options** are also
     examples of build options that apply to the entire system.

   * The Kconfig file at the net directory includes the different
     Kconfig symbols that define the configuration options for the
     communications stack code.

Dependencies
============

Dependencies allow you to define the valid and invalid configuration
combinations in the system.
Each dependency is a rule that a particular symbol must
follow to be either selected or allowed to have a specific value.
For example, the following configuration symbol states a dependency on
another one.

.. code-block:: kconfig

   config B bool

   config A depends on B

This means that the symbol A does not exist as a configuration option
in the system unless B is set to true.

The complete set of dependency rules defines the valid configuration
combinations that the system can be set to.

Experimental Symbols
--------------------

The Kconfig file located at :file:`misc/` defines the
**EXPERIMENTAL** symbols.
This symbol controls which configuration symbols are
considered experimental from the development standpoint.

This specific symbol hides the options marked as experimental from the
end user. Experimental options are prone to error and users must
specifically enable the **EXPERIMENTAL** symbol to enable the options
listed with the experimental option.

The following convention applies: Always make new experimental
symbols dependent on the **EXPERIMENTAL** symbol.

.. code-block:: kconfig

   config NEW_EXP_SYMBOLS depends on EXPERIMENTAL


Default Configurations
======================

The default configuration files define the default configuration
for a specific kernel on a specific platform, for example:
file:`arch/arm/configs`, :file:`arch/x86/configs`
and :file:`arch/arc/configs`.

The file name of a default configuration file contains the
type of kernel and the platform that is defined within the file.
All the defconfig files must end with the suffix defconfig.
For example, :file:`micro_quark_defconfig` contains the configuration
information for the microkernel architecture for the quark platform and
:file:`nano_generic_pc_pentium4` contains the configuration
information for the nanokernel architecture for the generic PC platform
with the Pentium processor platform variant.

The sanity checks use the default configuration files to dynamically
define what configuration corresponds to the platform that is being tested.

The :file:`Makefile.inc` file uses them as well to provide a
clean interface for the users that want to define new projects by
using environment variables to define the kernel type and the platform.

The build system provides the target defconfig. This target takes
the specified defconfig file and sets it as the current
:file:`.config` file for the current project. For example:

.. code-block:: bash

   $ make defconfig micro_quark_defconfig

The command takes the default configuration for the microkernel
architecture and the quark platform.

Merging Configuration Snippets
==============================

Configuration file snippets can be merged with the current project
configuration.
The user may provide a configuration file (:file:`prj.conf`) that
describes a small set of configuration symbols.
This set must correspond with the specific configuration symbols that
differ from the default configuration.
The **initconfig** target pulls the default configuration file
described by the project and merges the default configuration with the
configuration snippet.
For example, the sample application **hello world** overrides the
base configuration with the configuration snippet :file:`prj.conf`.

.. caution::
   Note that invalid configurations, or configurations that do not comply
   with the dependencies stated in the Kconfig files, are ignored by the
   merge process. If you are adding configuration symbols through a
   configuration snippet, ensure that the the complete sequence of symbols
   complies with the dependency rules stated in the Kconfig files.

Makefile Structure
******************

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

*   There must be a single Makefile per directory. Without a
     Makefile in the directory it is not considered a source code
     directory.

*   The scope of every Makefile is restricted to the content of that
     directory. A Makefile can only make a direct reference to its own
     files and subdirectories. Any file outside the directory has
     to be referenced in its home directory Makefile.

*   Makefiles list the object files that are included in the link
     process. The Kbuild finds the source file that generates the
     object file by matching the file name.

*   Parent directories add their child directories into the recursion
     model.

*   The root Makefile adds the directories in the kernel base
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

Creating a New Project
**********************

The build system is a project centric system. The |codename|
kernel must have a defined project that allows the build of the kernel
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

Development Project
===================

A common development project is composed of the following files:

* Makefile: Configures the project and integrates the
  developer's project with the |codename| Kbuild system.

* Configuration file: Allows the user
  to override the standard configuration.

* MDEF file: Defines the set of kernel objects instantiated by the
  project. Required only by microkernel architectures.

* :file:`src/` directory: Hosts by default the project source code.
   This directory can be overridden and defined in a different
   directory.

   * Makefile: Adds the developer's source code into the Kbuild
     recursion model.

The project source code can be organized in subdirectories.
Each directory should follow the Kbuild Makefile conventions.

Project Definition
==================

The development project is defined through the project Makefile.
The project is integrated into the kbuild system by
including the Makefile.inc file provided.

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

* :makevar:`BSP`:Selects the platform of the default
  configuration used by the project build. platform indicates the Board
  Support Package. The list of supported platform depends on the
  architecture selected. This is used along with `KERNEL_TYPE` to select
  the appropriate defconfig for the kernel.

* :makevar:`KERNEL_TYPE`: Select the kernel type of the default
  configuration used by the project build. It indicates if this is
  a nanokernel or microkernel architecture. The supported values for
  are **nano** and **micro**.

* :makevar:`MDEF_FILE`: Indicates the name of the MDEF file. It i
  required for microkernel architectures only.

* :makevar:`CONF_FILE`: Indicates the name of a configuration
  snippet file. This file includes the kconfig values that are
  overridden from the default configuration.

Supported Targets
*****************

This is the list of supported build system targets:

Clean targets
=============

* **clean:** Removes most generated files but keep the config
  information.

* **distclean:** Removes editor backup and patch files.

* **pristine:**  Alias name for distclean.

* **mrproper:**  Removes all generated files + various backup files and
  host tools files.

Configuration targets
=====================

* **config:**  Updates current config utilizing a line-oriented program.

* **nconfig:** Updates current config utilizing a ncurses menu based
  program.

* **menuconfig:** Updates current config utilizing a menu based program.

* **xconfig:** Updates current config utilizing a QT based front-end.

* **gconfig:** Updates current config utilizing a GTK based front-end.

* **oldconfig:** Updates current config utilizing a provided .config as
  base.

* **silentoldconfig:** Same as oldconfig, but quietly, additionally
  update dependencies.

* **defconfig:** New configuration with default from ARCH supplied
  defconfig.

* **savedefconfig:** Saves current config as ./defconfig (minimal
  configuration).

* **allnoconfig:** New configuration file where all options are
  answered with no.

* **allyesconfig:** New configuration file where all options are
  accepted with yes.

* **alldefconfig:** New configuration file with all symbols set to
  default.

* **randconfig:** New configuration file with random answer to all
  options.

* **listnewconfig:** Lists new options.

* **olddefconfig:** Same as silentoldconfig but sets new symbols to
  their default value.

* **tinyconfig:** Configures the tiniest possible kernel.

Other generic targets
=====================

* **all:** Builds zephyr target.

* **zephyr:** Builds the bare kernel.

* **qemu:** Builds the bare kernel and runs the emulation with qemu.

x86 Supported default configuration files
=========================================

* **micro_quark_defconfig** Builds for microkernel Quark.

* **micro_generic_pc_defconfig:** Builds for microkernel generic PC.

* **micro_generic_pc_atom_n28xx_defconfig:** Builds for microkernel
  generic PC atom n28xx processor.

* **micro_generic_pc_minuteia_defconfig:** Builds for microkernel
  generic PC minuteia processor.

* **micro_generic_pc_pentium4_defconfig:** Builds for microkernel
  generic PC Pentium4.

* **nano_quark_defconfig:** Builds for nanokernel Quark.

* **nano_generic_pc_defconfig:** Builds for nanokernel generic PC.

* **nano_generic_pc_atom_n28xx_defconfig:** - Builds for nanokernel
  generic PC atom n28xx.

* **nano_generic_pc_minuteia_defconfig:** Builds for nanokernel
  generic PC minuteia.

* **nano_generic_pc_pentium4_defconfig**: Builds for nanokernel
  generic PC Pentium4.


arm Supported default configuration files
=========================================

* **micro_fsl_frdm_k64f_defconfig:** Builds for microkernel
  FSL FRDM K64F.
* **micro_ti_lm3s6965_defconfig:** Builds for microkernel TI LM3S6965.
* **nano_fsl_frdm_k64f_defconfig:** Builds for nanokenrel FSL FRDM K64F.
* **nano_ti_lm3s6965_defconfig:** Builds for nanokernel TI LM3S6965.

Make Modifiers
==============

* **make V=0|1 [targets]** Modifies verbosity of the project.

  * **0:** Quiet build (default).

  * **1:** Verbose build.

  * **2:** Gives the reason for rebuild of target.

* **make O=dir [targets]** Locates all output files in **dir**,
including :file:`.config.`.

Setting Up a Toolchain
**********************

The |project| gives support for the configuration of Yocto and XTools
toolchain and build tools. The environment variable
**ZEPHYR_GCC_VARIANT** informs the build systen about which buid tool
set is installed in the system and configures it as a standard
installation:

.. code-block:: bash

   $ export ZEPHYR_GCC_VARIANT = yocto

   $ export ZEPHYR_GCC_VARIANT = xtools

The supported values for the **ZEPHYR_GCC_VARIANT** variable are:
**yocto** and **xtools**.

Yocto Configuration
===================

To set up a previously installed yocto toolchain in the build system,
you need to configure the Yocto SDK installation path and the GCC
variant in the shell environment:

.. code-block:: bash

   $ export YOCTO_SDK_INSTALL_DIR = <yocto-installation-path>

   $ export ZEPHYR_GCC_VARIANT = yocto

The build system configuration is done by the file
:file:`$(ZEPHYR_BASE)/scripts/Makefile.toochain.yocto`. The build
system takes the following configuration values:

* x86 default configuration values

  * Crosscompile target: i586-poky-elf

  * Crosscompile version: 4.9.2

  * Toolchain library: gcc


* ARM default configuration values

  * Crosscompile target: arm-poky-eabi

  * Crosscompile version: 4.9.2

  * Toolchain library: gcc

* ARC default configuration values

  * Crosscompile target: arc-poky-elf

  * Crosscompile version: 4.8.3

  * Toolchain library: gcc

The cross-compile target, cross-compile version, toolchain library and
library path can be adjusted in the file
:file:`$(ZEPHYR_BASE)/scripts/Makefile.toochain.yocto` following your
installation specifics.

XTools Configuration
====================

To set up a previously installed XTools toolchain in the build system,
you need to configure the XTools installation path and the GCC
variant in the shell environment:

.. code-block:: bash

   $ export XTOOLS_TOOLCHAIN_PATH = <yocto-installation-path>

   $ export ZEPHYR_GCC_VARIANT = xtools

The build system configuration is done by the file
:file:`$(ZEPHYR_BASE)/scripts/Makefile.toochain.xtools`. The build
system takes the following configuration values:

* x86 default configuration values

  * Crosscompile target: i586-pc-elf

  * Crosscompile version: 4.9.2

  * Toolchain library: gcc

* ARM default configuration values

  * Crosscompile target: arm-none-eabi

  * Crosscompile version: 4.9.2

  * Toolchain library: gcc

The cross-compile target, cross-compile version and toolchain
can be adjusted in the file
:file:`$(ZEPHYR_BASE)/scripts/Makefile.toochain.xtools` following your
installation specifics.

Generic Toolchain Configuration
===============================

It is possible to build and install an specific toolchain and configure
the build system to work with it. The **CROSS_COMPILE**,
**TOOLCHAIN_LIBS** and **LIB_INCLUDE_DIR** need to be configured in
your environment.

.. note::

   The installed toolchain must be from the gcc family. The build tools
   should follow the convention of: prefix + command-name. For example,
   the gcc command should be named: **arm-poky-eabi-gcc**

The **CROSS_COMPILE** environment variable should be set to the
build tools prefix used for build tools commands.

.. code-block:: bash

   $ export CROSS_COMPILE = i586-elf-

.. note::
   If the command home directory is not set in the **PATH** environment
   variable, the **CROSS_COMPILE** must include the complete path as
   part of the command prefix.

The **TOOLCHAIN_LIBS** list the libraries required by the toolchain, like gcc.

.. code-block:: bash

   $ export TOOLCHAIN_LIBS = gcc

.. note::
   Notice that there  library name does not include the l prefix
   commonly found when referring to libraries (lgcc).

**LIB_INCLUDE_DIR** defines the directory path where the toolchain
libraries can be located.

.. code-block:: bash

   $ export LIB_INCLUDE_DIR = -L /opt/i586-elf/usr/lib/i586-elf/4.9

.. note::
   Notice the use of the -L command parameter, included in the value
   of the environment variable.
