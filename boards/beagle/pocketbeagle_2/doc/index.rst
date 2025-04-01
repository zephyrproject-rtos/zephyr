.. zephyr:board:: pocketbeagle_2

Overview
********

PocketBeagle 2 is a computational platform powered by TI AM62x SoC (there are two
revisions, AM6232 and AM6254).

The board configuration provides support for the ARM Cortex-M4F MCU core.

See the `PocketBeagle 2 Product Page`_ for details.

Hardware
********
PocketBeagle 2 features the TI AM62x SoC based around an Arm Cortex-A53 multicore
cluster with an Arm Cortex-M4F microcontroller, Imagination Technologies AXE-1-16
graphics processor (from revision A1) and TI programmable real-time unit subsystem
microcontroller cluster coprocessors.

Zephyr is ported to run on the M4F core and the following listed hardware
specifications are used:

- Low-power ARM Cortex-M4F
- Memory

   - 256KB of SRAM
   - 512MB of DDR4

Currently supported PocketBeagle 2 revisions:

- A0: Comes wth SOC AM6232

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

The board has 512MB of DDR RAM available. This board configuration
allocates Zephyr 4kB of RAM (only for resource table: 0x9CC00000 to 0x9CC00400).

Serial Port
-----------

This board configuration uses a single serial communication channel with the
MCU domain UART (MCU_UART0, i.e. P2.05 as RX and P2.07 as TX).

SD Card
*******

Download BeagleBoard.org's official `BeagleBoard Imaging Utility`_ to create bootable
SD-card with the Linux distro image. This will boot Linux on the A53 application
cores. These cores will then load the Zephyr binary on the M4 core using remoteproc.

Flashing
********

The board supports remoteproc using the OpenAMP resource table.

The testing requires the binary to be copied to the SD card to allow the A53 cores to load it while booting using remoteproc.

To test the M4F core, we build the :zephyr:code-sample:`hello_world` sample with the following command.

.. zephyr-app-commands::
   :board: pocketbeagle_2/am6232/m4
   :zephyr-app: samples/hello_world
   :goals: build

This builds the program and the binary is present in the :file:`build/zephyr` directory as
:file:`zephyr.elf`.

We now copy this binary onto the SD card in the :file:`/lib/firmware` directory and name it as
:file:`am62-mcu-m4f0_0-fw`.

.. code-block:: console

   # Mount the SD card at sdcard for example
   sudo mount /dev/sdX sdcard
   # copy the elf to the /lib/firmware directory
   sudo cp --remove-destination zephyr.elf sdcard/lib/firmware/am62-mcu-m4f0_0-fw

The SD card can now be used for booting. The binary will now be loaded onto the M4F core on boot.

The binary will run and print Hello world to the MCU_UART0 port.

Debugging
*********

The board supports debugging M4 core from the A53 cores running Linux. Since the target needs
superuser privilege, openocd needs to be launched seperately for now:

.. code-block:: console

   sudo openocd -f board/ti_am625_swd_native.cfg


Start debugging

.. zephyr-app-commands::
   :goals: debug

References
**********

* `PocketBeagle 2 Product Page`_
* `Documentation <https://docs.beagleboard.org/boards/pocketbeagle-2/index.html>`_

.. _PocketBeagle 2 Product Page:
   https://www.beagleboard.org/boards/pocketbeagle-2

.. _BeagleBoard Imaging Utility:
   https://github.com/beagleboard/bb-imager-rs/releases
