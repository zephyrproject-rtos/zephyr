.. zephyr:board:: sk_am62a

Overview
********

The SK-AM62A board configuration is used by Zephyr applications that run on the
TI AM62A platform.

The ``sk_am62a/am62a7/mcu_r5f0_0`` board configuration provides support for the ARM
Cortex-R5F MCU core.

The board configuration also enables support for the semihosting debugging console.

See the `TI AM62A Product Page`_ for details.

Hardware
********
The SK-AM62A EVM features the AM62A SoC, which is composed of a quad Cortex-A53
cluster and a single Cortex-R5F core in the MCU domain. Zephyr is ported to run on
the R5F core and the following listed hardware specifications are used:

- ARM Cortex-R5F
   - 64KB of SRAM

- Memory
   - 4GB of DDR4

- Debug
   - XDS110 based JTAG

Supported Features
==================

.. zephyr:board-supported-hw::

Devices
========
System Clock
------------

This board configuration uses a system clock frequency of 400 MHz.

DDR RAM
-------

The board has 4GB of DDR RAM available. This board configuration
allocates Zephyr:

- 1MB for IPC (VirtIO / Vrings)
- 4KB for Linux RemoteProc resource table
- 15MB for general usage

Serial Port
-----------

This board configuration uses a single serial communication channel with the
MCU domain UART (MCU_UART0).

SD Card
*******

Download TI's official `WIC`_ and flash the WIC file with an etching software
onto an SD card. This will boot Linux on the A53 application cores of the EVM.
These cores will then load the zephyr binary on the R5F core using remoteproc.

Flashing
********

The board can using remoteproc, and uses the OpenAMP resource table to accomplish this.

The testing requires the binary to be copied to the SD card to allow the A53 cores to load it while booting using remoteproc.

To test the R5F core, we build the :zephyr:code-sample:`hello_world` sample with the following command.

.. code-block:: console

   # From the root of the Zephyr repository
   west build -p -b sk_am62a/am62a7/mcu_r5f0_0 samples/hello_world

This builds the program and the binary is present in the :file:`build/zephyr` directory as
:file:`zephyr.elf`.

We now copy this binary onto the SD card in the :file:`/lib/firmware` directory and name it as
:file:`am62a-mcu-r5f0_0-fw`.

.. code-block:: console

   # Mount the SD card at sdcard for example
   sudo mount /dev/sdX sdcard
   # copy the elf to the /lib/firmware directory
   sudo cp --remove-destination zephyr.elf sdcard/lib/firmware/am62a-mcu-r5f0_0-fw

The SD card can now be used for booting. The binary will now be loaded onto the R5F core on boot.

To allow the board to boot using the SD card, set the boot pins to the SD Card boot mode. Refer to `AM62A Starter Kit EVM Quick Start Guide`_.

After changing the boot mode, the board should go through the boot sequence on powering up.
The binary will run and print Hello world to the MCU_UART0 port.

Debugging
*********

The board is equipped with an XDS110 JTAG debugger. To debug a binary, utilize the ``debug`` build target:

.. zephyr-app-commands::
   :app: <my_app>
   :board: sk_am62a/am62a7/mcu_r5f0_0
   :maybe-skip-config:
   :goals: debug

References
**********

.. _TI AM62A Product Page:
   https://www.ti.com/product/AM62A7

.. _WIC:
   https://dr-download.ti.com/software-development/software-development-kit-sdk/MD-D37Ls3JjkT/11.01.07.05/tisdk-edgeai-image-am62a-evm-11.01.07.05.rootfs.wic.xz

.. _AM62A Starter Kit EVM Quick Start Guide:
   https://dev.ti.com/tirex/content/tirex-product-tree/am62ax-devtools/docs/am62ax_skevm_quick_start_guide.html
