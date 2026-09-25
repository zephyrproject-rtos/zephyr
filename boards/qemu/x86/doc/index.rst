.. zephyr:board:: qemu_x86

Overview
********

The X86 QEMU board configuration is used to emulate the X86 architecture.

This board configuration provides support for a 32-bit Atom-class x86 CPU and
the following devices:

* HPET
* Advanced Programmable Interrupt Controller (APIC)
* NS16550 UART
* E1000 Ethernet, CAN and VirtIO input devices on an emulated PCI bus
* Simulated flash and EEPROM
* QEMU ramfb display

The ``qemu_x86/atom/*`` variants exercise the same machine with a particular
memory-management configuration (no MMU, 32-bit page tables, KPTI disabled,
execute-in-place, or a relocated virtual address space); ``qemu_x86_tiny``
uses the same devicetree with only 384 KiB of RAM to test demand paging.

Hardware
********

Supported Features
==================

.. zephyr:board-supported-hw::

Devices
=======

HPET System Clock Support
-------------------------

The configuration uses an HPET clock frequency of 25 MHz.

Serial Port
-----------

The board configuration uses a single serial communication channel that
uses the NS16550 serial driver operating in polling mode. To override, enable
the UART_INTERRUPT_DRIVEN Kconfig option, which allows the system to be
interrupt-driven.

If SLIP networking is enabled (see below), an additional serial port will be
used for it.

Known Problems or Limitations
=============================

The following platform features are unsupported:

* Isolated Memory Regions
* Serial port in Direct Memory Access (DMA) mode
* Serial Peripheral Interface (SPI) flash
* General-Purpose Input/Output (GPIO)
* Inter-Integrated Circuit (I2C)
* Ethernet
* Supervisor Mode Execution Protection (SMEP)

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``qemu_x86`` board configuration can be built and run in
the usual way for emulated boards (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Flashing
========

While this board is emulated and you can't "flash" it, you can use this
configuration to run basic Zephyr applications and kernel tests in the QEMU
emulated environment. For example, with the :zephyr:code-sample:`synchronization` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: qemu_x86
   :goals: run

This will build an image with the synchronization sample app, boot it using
QEMU, and display the following console output:

.. code-block:: console

        ***** BOOTING ZEPHYR OS v1.8.99 - BUILD: Jun 27 2017 13:09:26 *****
        threadA: Hello World from x86!
        threadB: Hello World from x86!
        threadA: Hello World from x86!
        threadB: Hello World from x86!
        threadA: Hello World from x86!
        threadB: Hello World from x86!
        threadA: Hello World from x86!
        threadB: Hello World from x86!
        threadA: Hello World from x86!
        threadB: Hello World from x86!

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.

Debugging
=========

Refer to the detailed overview about :ref:`application_debugging`.

Networking
==========

The board supports SLIP networking over an emulated serial port
(``CONFIG_NET_SLIP_TAP=y``). The detailed setup is described in
:ref:`networking_with_qemu`.

It is also possible to use the QEMU built-in Ethernet adapter to connect
to the host system. This is faster than using SLIP and is also the preferred
way. See :ref:`networking_with_eth_qemu` for details.
