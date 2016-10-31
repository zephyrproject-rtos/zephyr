.. _kconfig:

The Kconfig File Structure
**************************

Introduction
============

The Kconfig files describe: the configuration symbols supported in the build
system, the logical organization and structure to group the symbols in menus
and sub-menus, and the relationships between the different configuration
symbols that govern the valid configuration combinations.

The Kconfig files are distributed across the build directory tree. The files
are organized based on their common characteristics and on what new symbols
they add to the configuration menus. For example:

* The Kconfig file at the root directory contains the general
  configuration options like :option:`CONFIG_ARCH` and
  ``CONFIG_KERNEL VERSION``. These symbols are defined for and
  apply to the entire build system.

* The Kconfig file at the :file:`kernel` directory contains the general
  configuration related to the core kernel.

* The Kconfig file at the :file:`drivers` directory organizes the inclusion of
  the various Kconfig files needed for each supported driver in the system.

* The Kconfig file at the :file:`misc` directory contains the
  configuration symbols that affect different aspects of the build
  system. For example, the *Custom Compiler Options* and the
  ``Minimal Libc`` are general build options that apply to the build
  system.  *Debugging Options* and *System Monitoring Options* are
  also examples of build options that apply to the entire system.

* The Kconfig file at the :file:`net` directory contains the different symbols
  that define the configuration options for the communications stack code.

* The Kconfig file at the :file:`crypto` directory contains the different
  symbols that define the configuration options for the cryptography algorithms
  supported.

Dependencies
============

Dependencies allow you to define the valid and invalid configuration
combinations in the system.  Each dependency is a rule that a particular symbol
must follow to be either selected or allowed to have a specific value. For
example, the configuration symbol *B* states a dependency on another one, *A*.

.. code-block:: kconfig

   config B bool

   config A depends on B

The symbol A does not exist as a configuration option in the system unless B is
set to true.

The complete set of dependency rules defines the valid configuration
combinations that the system can use.


Default Configurations
======================

The default configuration files define the default configuration for a specific
kernel on a specific SoC. For example:

* :file:`arch/arm/configs`,
* :file:`arch/x86/configs` and
* :file:`arch/arc/configs`.

All the default configuration files must end with the suffix _defconfig. For
example, the :file:`galileo_defconfig` file contains the configuration
information for the galileo board.

The :file:`Makefile.inc` file uses defconfig files to provide a clean interface
to developers using environment variables to define the kernel type and the
board of new projects. Developers can provide the build system with a
target's defconfig. The build system takes the specified defconfig file and
sets it as the current :file:`.config` file for the current project. For
example:

.. code-block:: console

   $ make galileo_defconfig

The command takes the default configuration for the architecture
and the galileo board configuration to compile the kernel.

.. _configuration_snippets:

Merging Configuration Fragments
===============================

Configuration file fragment can be merged with the current project
configuration during the build.

Developers can provide a configuration file that defines a small subset of
configuration options.  The subset must contain the specific configuration
options that differ from the default configuration.

The **initconfig** target pulls the default configuration file and merges it
with the local configuration fragments. For example, the sample application **hello
world** overrides the base configuration with the configuration fragment in
:file:`prj.conf`.

.. caution::
   Invalid configurations, or configurations that do not comply with
   the dependencies stated in the Kconfig files, are ignored by the merge process.
   When adding configuration options through a configuration fragment, ensure that
   the complete sequence complies with the dependency rules defined in the
   Kconfig files.
