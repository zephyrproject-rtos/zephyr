.. zephyr:board:: pocketbeagle_2

Overview
********

PocketBeagle 2 is a computational platform powered by TI AM62x SoC (there are two
revisions, AM6232 and AM6254).

See the `PocketBeagle 2 Product Page`_ for details.

Hardware
********
PocketBeagle 2 features the TI AM62x SoC based around an Arm Cortex-A53 multicore
cluster with an Arm Cortex-M4F microcontroller, Imagination Technologies AXE-1-16
graphics processor (from revision A1) and TI programmable real-time unit subsystem
microcontroller cluster coprocessors.

Additionally, PocketBeagle 2 also contains an MSPM0L1105 SoC which serves as EEPROM and ADC.

Zephyr is enabled to run on:

- Arm Cortex-A53 cores on AM62x,
- Arm Cortex-M4F core on AM62x, and
- Arm Cortex-M0+ core on MSPM0L1105.

The following listed hardware specifications are used:

- Dual ARM Cortex-A53 cores
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

A53 Cores
^^^^^^^^^

This board configuration uses single serial communication channel with the MAIN domain UART
(MAIN_UART6, i.e. debug port).

M4F Core
^^^^^^^^

This board configuration uses a single serial communication channel with the
MCU domain UART (MCU_UART0, i.e. P2.05 as RX and P2.07 as TX).

SD Card
*******

A53 Cores
=========

Download BeagleBoard.org's official `BeagleBoard Imaging Utility`_ to create bootable
SD-card with the Zephyr image. Optionally, the Zephyr SD Card images can be downloaded from
`bb-zephyr-images`_.

M4F Core
========

Download BeagleBoard.org's official `BeagleBoard Imaging Utility`_ to create bootable
SD-card with the Linux distro image. This will boot Linux on the A53 application
cores. These cores will then load the Zephyr binary on the M4 core using remoteproc.

MSPM0L1105
==========

Download BeagleBoard.org's official `BeagleBoard Imaging Utility`_ to create bootable
SD-card with the Linux distro image. This will boot Linux on the A53 application
cores. We can then flash MSPM0L1105 firmware from Linux using BSL over I2C. The BeagleBoard.org
distro images ship with a driver that supports `Firmware Upload API`_ for MSPM0L1105.

Flashing
********

A53 Core
========

The testing requires the binary to be copied to the BOOT partition in SD card.

To test the A53 core, we build the :zephyr:code-sample:`hello_world` sample with the following command.

.. zephyr-app-commands::
   :board: pocketbeagle_2/am6232/a53
   :zephyr-app: samples/hello_world
   :goals: build

We now copy this binary onto the SD card in the :file:`/boot/` directory and name it as
:file:`zephyr.bin`.

.. code-block:: console

   # Mount the SD card at sdcard for example
   sudo mount /dev/sdX sdcard
   # copy the bin to the /boot/
   sudo cp --remove-destination zephyr.bin sdcard/boot/zephyr.bin

The SD card can now be used for booting.

The binary will run and print Hello world to the debug port.

M4F Core
========

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

MSPM0L1105
==========

.. note::
   On PocketBeagle 2 MSPM0L1105 is used as EEPROM and ADC. So flashing any custom firmware will
   break this functionality.

.. note::
   Flashing new firmware will also clear the EEPROM contents. So please make backup of EEPROM data
   before attempting to flash firmware to MSPM0L1105.

To test the A53 cores, we build the :zephyr:code-sample:`minimal` sample with the following command.

.. zephyr-app-commands::
   :board: pocketbeagle_2/mspm0l1105
   :zephyr-app: samples/basic/minimal
   :goals: build

This builds the program and the binary is present in the :file:`build/zephyr` directory as
:file:`zephyr.bin`.

We now flash this binary using FW Upload API.

.. code-block:: console

   echo 1 > /sys/class/firmware/mspm0l1105/loading
   dd if=zephyr.bin of=/sys/class/firmware/mspm0l1105/data
   echo 0 > /sys/class/firmware/mspm0l1105/loading

Debugging
*********

M4F Core
========

The board supports debugging M4 core from the A53 cores running Linux. Since the target needs
superuser privilege, openocd needs to be launched separately for now:

.. code-block:: console

   sudo openocd -f board/ti_am625_swd_native.cfg


Start debugging

.. zephyr-app-commands::
   :board: pocketbeagle_2/am6232/m4
   :goals: debug

References
**********

* `PocketBeagle 2 Product Page`_
* `Documentation <https://docs.beagleboard.org/boards/pocketbeagle-2/index.html>`_

.. _PocketBeagle 2 Product Page:
   https://www.beagleboard.org/boards/pocketbeagle-2

.. _BeagleBoard Imaging Utility:
   https://github.com/beagleboard/bb-imager-rs/releases

.. _bb-zephyr-images:
   https://github.com/beagleboard/bb-zephyr-images/releases

.. _Firmware Upload API:
   https://www.kernel.org/doc/html/latest/driver-api/firmware/fw_upload.html
