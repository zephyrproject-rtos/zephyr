.. zephyr:board:: qemu_x86_lakemont

Overview
********

The X86 Lakemont QEMU board configuration is used to emulate the Intel
Lakemont (Minute IA) CPU as found in the Quark family of SoCs.

Unlike the Lakemont-based hardware targets, which have neither a BIOS nor
ACPI, QEMU presents a PC-compatible machine, so this board configuration
provides the following devices:

* HPET
* Advanced Programmable Interrupt Controller (APIC)
* NS16550 UART

Hardware
********

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``qemu_x86_lakemont`` board configuration can be built
and run in the usual way for emulated boards (see :ref:`build_an_application`
and :ref:`application_run` for more details).

Flashing
========

While this board is emulated and you can't "flash" it, you can use this
configuration to run basic Zephyr applications and kernel tests in the QEMU
emulated environment. For example, with the :zephyr:code-sample:`synchronization` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: qemu_x86_lakemont
   :goals: run

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.

Debugging
=========

Refer to the detailed overview about :ref:`application_debugging`.
