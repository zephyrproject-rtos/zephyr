.. zephyr:board:: qemu_x86_64

Overview
********

The X86_64 QEMU board configuration is used to emulate the x86 architecture
running in 64-bit long mode.

It uses the same emulated q35 machine as :zephyr:board:`qemu_x86`, adding a
second CPU for SMP testing and an emulated Intel PCH SMBus controller. The
``qemu_x86_64/atom/nokpti`` variant runs the same configuration with kernel
page table isolation disabled.

Hardware
********

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``qemu_x86_64`` board configuration can be built and run
in the usual way for emulated boards (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Flashing
========

While this board is emulated and you can't "flash" it, you can use this
configuration to run basic Zephyr applications and kernel tests in the QEMU
emulated environment. For example, with the :zephyr:code-sample:`synchronization` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: qemu_x86_64
   :goals: run

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.

UEFI Boot
=========

The ``qemu_x86_64`` platform also supports the UEFI bootable method to run
Zephyr applications and kernel tests, but you need to set up some environment
configurations as follows:

* Please install uefi-run in your system environment according to this
  reference link https://github.com/Richard-W/uefi-run. Note that uefi-run
  from snapstore may not work because of strict snap confinements.
  The preferred method is installing with cargo.

* Please install OVMF in your system environment according to this
  reference link https://github.com/tianocore/tianocore.github.io/wiki/OVMF.
  The easiest way is to install a special ``ovmf`` package found in many distros.
  For example, use the following command in Ubuntu:

  .. code-block:: console

     sudo apt install ovmf

* Set system environment variable OVMF_FD_PATH,
  for example:

  .. code-block:: console

     export OVMF_FD_PATH=/usr/share/OVMF/OVMF_CODE.fd

Now you can build application, for example UEFI boot test sample found under
:zephyr_file:`tests/boot/uefi`:

.. zephyr-app-commands::
   :zephyr-app: tests/boot/uefi
   :host-os: unix
   :board: qemu_x86_64
   :goals: run

This will build an image with the uefi boot test app, boot it on
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
