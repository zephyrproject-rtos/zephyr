.. _kbuild_targets:

Supported Targets
*****************

The build system supports the following targets:

Clean targets
=============

   **clean**: Removes most generated files keeping the configuration information.

   **distclean**: Removes editor backup and patch files.

   **pristine**:  Alias name for distclean.

   **mrproper**:  Removes all generated files, various backup files, and host tools files.

Configuration targets
=====================

   **config**:  Updates the current configuration using a line-oriented program.

   **nconfig**: Updates the current configuration using a ncurses menu based program.

   **menuconfig**: Updates the current configuration using a menu based program.

   **xconfig**: Updates the current configuration using a QT based front-end.

   **gconfig**: Updates the current configuration using a GTK based front-end.

   **oldconfig**: Updates the current configuration using a provided :file:`.config` as base.

   **silentoldconfig**: Works as oldconfig but quietly. Additionally, it updates the dependencies.

   **defconfig**: Creates a new configuration with the default defconfig supplied by ARCH.

   **savedefconfig**: Saves the current configuration as the minimal configuration under
   :file:`./defconfig`.

   **allnoconfig**: Creates a configuration file where all options are answered with no.

   **allyesconfig**: Creates a configuration file where all options are accepted with yes.

   **alldefconfig**: Creates a configuration file with all symbols set to default.

   **randconfig**: Creates a configuration file with random answer to all options.

   **listnewconfig**: Lists new options.

   **olddefconfig**: Works as silentoldconfig while setting new symbols to their default value.

   **tinyconfig**: Configures the smallest possible kernel.

Generic Targets
=====================

   **all**: Builds zephyr target.

   **zephyr**: Builds the bare kernel.

   **qemu**: Builds the bare kernel and runs the emulation with Qemu.

x86 Supported Default Configuration Files
=========================================

   **micro_galileo_defconfig**: Builds a microkernel for Galileo.

   **micro_basic_atom_defconfig**: Builds a microkernel for an Atom processor on Qemu.

   **micro_basic_minuteia_defconfig**: Builds a microkernel for minute IA processor on Qemu.

   **nano_galileo_defconfig**: Builds a nanokernel for Galileo.

   **nano_basic_atom_defconfig**: Builds a nanokernel for an Atom processor on Qemu.

   **nano_basic_minuteia_defconfig**: Builds a nanokernel for minute IA processor on Qemu.

arm Supported Default Configuration Files
=========================================

   **micro_fsl_frdm_k64f_defconfig**: Builds a microkernel for FSL FRDM K64F.

   **micro_basic_cortex_m3_defconfig**: Builds a microkernel for Cortex-M3 processor on Qemu.

   **nano_fsl_frdm_k64f_defconfig**: Builds a nanokenrel FSL FRDM K64F.

   **nano_basic_cortex_m3_defconfig**: Builds a nanokernel for Cortex-M3 processor on Qemu.

Make Modifiers
==============

   **make V=0|1 [targets]**: Modifies verbosity of the project.

     **0**: Quiet build, default.

     **1**: Verbose build.

     **2**: Gives the reason for rebuilding the target.

   **make O=dir [targets]** Places all output files in :file:`dir`, including :file:`.config.`.

Other Environment Variables
===========================

* :envvar:`USE_CCACHE` If set to 1, the build system uses the :program:`ccache` utility to speed
  up builds. :program:`ccache` must be installed on your development system. See the
  `CCACHE documentation`_.

.. _CCACHE documentation: https://ccache.samba.org/