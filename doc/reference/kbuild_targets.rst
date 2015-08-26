.. _kbuild_targets:

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

* **micro_galileo_defconfig** Builds microkernel for galileo.

* **micro_basic_atom_defconfig:** Builds microkernel for atom
  processor on QEMU.

* **micro_basic_minuteia_defconfig:** Builds microkernel for minute IA
  processor on QEMU.

* **nano_galileo_defconfig:** Builds nanokernel for galileo.

* **nano_basic_atom_defconfig:** Builds nanokernel for atom
  processor on QEMU.

* **nano_basic_minuteia_defconfig:** Builds nanokernel for minute IA
  processor on QEMU.

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

Other Environment Variables
===========================

* :envvar:`USE_CCACHE=1` If set, use the :program:`ccache` utility to speed up builds.
  `ccache` must be installed on your development workstation. For more
  information see the `ccache documentation`_.

.. _CCACHE documentation: https://ccache.samba.org/

