.. _kbuild_kconfig::

The Kconfig File Structure
**************************

Introduction
============
The Kconfig files describe the configuration symbols supported in the build
system, the logical organization and structure to group the symbols in menus
and sub-menus, and the relationships between the different configuration
symbols that govern the valid configuration combinations.

The Kconfig files are distributed across the build directory tree. There
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

* The Kconfig file at the crypto directory includes the different
  Kconfig symbols that define the configuration options for the
  cryptography algorithms supported.

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


Default Configurations
======================

The default configuration files define the default configuration
for a specific kernel on a specific platform, for example:
file:`arch/arm/configs`, :file:`arch/x86/configs`
and :file:`arch/arc/configs`.

The file name of a default configuration file contains the
type of kernel and the platform that is defined within the file.
All the defconfig files must end with the suffix defconfig.
For example, :file:`micro_galileo_defconfig` contains the configuration
information for the microkernel architecture for the galileo platform
configuration and :file:`nano_basic_atom_defconfig` contains the configuration
information for the nanokernel architecture for the basic atom platform
configuration.

The sanity checks use the default configuration files to dynamically
define what configuration corresponds to the platform that is being tested.

The :file:`Makefile.inc` file uses them as well to provide a
clean interface for the users that want to define new projects by
using environment variables to define the kernel type and the platform.

The build system provides the target defconfig. This target takes
the specified defconfig file and sets it as the current
:file:`.config` file for the current project. For example:

.. code-block:: bash

   $ make defconfig micro_galileo_defconfig

The command takes the default configuration for the microkernel
architecture and the galileo platform configuration.

.. _configuration_snippets:

Merging Configuration Snippets
==============================

Configuration file snippets can be merged with the current project
configuration.

The user can provide a configuration file (:file:`prj.conf`) that describes
a small set of configuration symbols. This set must correspond with the
specific configuration symbols that differ from the default configuration.

The **initconfig** target pulls the default configuration file described by
the project and merges the default configuration with the configuration
snippet. For example, the sample application **hello world** overrides the
base configuration with the configuration snippet :file:`prj.conf`.

.. caution::
   Note that invalid configurations, or configurations that do not comply
   with the dependencies stated in the Kconfig files, are ignored by the
   merge process. If you are adding configuration symbols through a
   configuration snippet, ensure that the the complete sequence of symbols
   complies with the dependency rules stated in the Kconfig files.
