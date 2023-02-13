.. _qemu_x86:

X86 Emulation (QEMU)
####################

Overview
********

The X86 QEMU board configuration is used to emulate the X86 architecture.

This board configuration provides support for an x86 Minute IA (Lakemont) CPU
and the following devices:

* HPET
* Advanced Programmable Interrupt Controller (APIC)
* NS16550 UART


Hardware
********

Supported Features
==================

This configuration supports the following hardware features:

+--------------+------------+-----------------------+
| Interface    | Controller | Driver/Component      |
+==============+============+=======================+
| HPET         | on-chip    | system clock          |
+--------------+------------+-----------------------+
| APIC         | on-chip    | interrupt controller  |
+--------------+------------+-----------------------+
| NS16550      | on-chip    | serial port           |
| UART         |            |                       |
+--------------+------------+-----------------------+

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

Applications for the ``qemu_x86`` board configuration can be built and run in
the usual way for emulated boards (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Flashing
========

While this board is emulated and you can't "flash" it, you can use this
configuration to run basic Zephyr applications and kernel tests in the QEMU
emulated environment. For example, with the :ref:`synchronization_sample`:

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

For qemu_x86_64 platform, it also supports to use UEFI bootable method
to run Zephyr applications and kernel tests, but you need to set up
some environment configurations as follows:

* Please install uefi-run in your system environment according to this
  reference link https://github.com/Richard-W/uefi-run.

* Please install OVMF in your system environment according to this
  reference link https://github.com/tianocore/tianocore.github.io/wiki/OVMF.

* Set system environment variable OVMF_FD_PATH,
  for example: export OVMF_FD_PATH=/usr/share/edk2.git/ovmf-x64/OVMF_CODE-pure-efi.fd

For example, with the test "sample.basic.helloworld.uefi":

.. code-block:: console

        export OVMF_FD_PATH=/usr/share/edk2.git/ovmf-x64/OVMF_CODE-pure-efi.fd
        west build -b qemu_x86_64 -p auto samples/hello_world/ -DCONF_FILE=prj_uefi.conf
        west build -t run

This will build an image with the hello_world sample app, boot it on
qemu_x86_64 using UEFI, and display the following console output:

.. code-block:: console

        UEFI Interactive Shell v2.2
        EDK II
        UEFI v2.70 (EDK II, 0x00010000)
        Mapping table
              FS0: Alias(s):F0a:;BLK0:
                  PciRoot(0x0)/Pci(0x1,0x1)/Ata(0x0)
             BLK1: Alias(s):
                  PciRoot(0x0)/Pci(0x1,0x1)/Ata(0x0)
        Press ESC in 1 seconds to skip startup.nsh or any other key to continue.
        Starting UEFI application...
        *** Zephyr EFI Loader ***
        Zeroing 524544 bytes of memory at 0x105000
        Copying 32768 data bytes to 0x1000 from image offset
        Copying 20480 data bytes to 0x100000 from image offset 32768
        Copying 540416 data bytes to 0x185100 from image offset 53248
        Jumping to Entry Point: 0x112b (48 31 c0 48 31 d2 48)
        *** Booting Zephyr OS build zephyr-v2.6.0-1472-g61810ec36d28  ***
        Hello World! qemu_x86_64

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
