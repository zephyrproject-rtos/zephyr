.. zephyr:board:: frdm_mcxn947

Overview
********

FRDM-MCXN947 are compact and scalable development boards for rapid prototyping of
MCX N94 and N54 MCUs. They offer industry standard headers for easy access to the
MCUs I/Os, integrated open-standard serial interfaces, external flash memory and
an on-board MCU-Link debugger. MCX N Series are high-performance, low-power
microcontrollers with intelligent peripherals and accelerators providing multi-tasking
capabilities and performance efficiency.

Hardware
********

- MCX-N947 Dual Arm Cortex-M33 microcontroller running at 150 MHz
- 2MB dual-bank on chip Flash
- 512 KB RAM
- External Quad SPI flash over FlexSPI
- USB high-speed (Host/Device) with on-chip HS PHY. HS USB Type-C connectors
- 10x LP Flexcomms each supporting SPI, I2C, UART
- 2x FlexCAN with FD, 2x I3Cs, 2x SAI
- 1x Ethernet with QoS
- On-board MCU-Link debugger with CMSIS-DAP
- Arduino Header, FlexIO/LCD Header, SmartDMA/Camera Header, mikroBUS

For more information about the MCX-N947 SoC and FRDM-MCXN947 board, see:

- `MCX-N947 SoC Website`_
- `MCX-N947 Datasheet`_
- `MCX-N947 Reference Manual`_
- `FRDM-MCXN947 Website`_
- `FRDM-MCXN947 User Guide`_
- `FRDM-MCXN947 Board User Manual`_
- `FRDM-MCXN947 Schematics`_

Supported Features
==================

.. zephyr:board-supported-hw::

Dual Core samples
*****************

+-----------+----------------------+-------------------------------+
| Core      | Flash Region         | Comment                       |
+===========+======================+===============================+
| CPU0      | Full flash memory    | Primary core with bootloader  |
|           | (including partition | access and application in     |
|           | slot0_partition)     | slot0_partition               |
+-----------+----------------------+-------------------------------+
| CPU1      | slot1_partition only | Secondary core restricted to  |
|           |                      | its dedicated partition       |
+-----------+----------------------+-------------------------------+

+----------+------------------+-----------------------+
| Memory   | Region           | Comment               |
+==========+==================+=======================+
| srama    | RAM (320KB)      | CPU0 ram              |
+----------+------------------+-----------------------+
| sramg    | RAM (64KB)       | CPU1 ram              |
+----------+------------------+-----------------------+
| sramh    | RAM (32KB)       | Shared memory         |
+----------+------------------+-----------------------+

.. note::
   The actual memory addresses are defined in the device tree and can be viewed in the
   generated map files after building. CPU0 accesses the full flash memory starting from
   its base address, while CPU1 is restricted to the slot1_partition region.

Targets available
==================

The default configuration file
:zephyr_file:`boards/nxp/frdm_mcxn947/frdm_mcxn947_mcxn947_cpu0_defconfig`
only enables the first core. CPU0 is the only target that can run standalone.

CPU1 does not work without CPU0 enabling it.

To enable CPU1, create System Build application project and enable the
second core with config :kconfig:option:`CONFIG_SECOND_CORE_MCUX`.

Please have a look at some already enabled samples:

- :zephyr_file:`samples/subsys/ipc/ipc_service/static_vrings`
- :zephyr_file:`samples/subsys/ipc/openamp`
- :zephyr_file:`samples/drivers/mbox`
- :zephyr_file:`samples/drivers/mbox_data`

Connections and IOs
===================

The MCX-N947 SoC has 6 gpio controllers and has pinmux registers which
can be used to configure the functionality of a pin.

+------------+-----------------+----------------------------+
| Name       | Function        | Usage                      |
+============+=================+============================+
| P0_PIO1_8  | UART            | UART RX cpu0               |
+------------+-----------------+----------------------------+
| P1_PIO1_9  | UART            | UART TX cpu0               |
+------------+-----------------+----------------------------+
| P4_PIO4_3  | UART            | UART RX cpu1               |
+------------+-----------------+----------------------------+
| P4_PIO4_2  | UART            | UART TX cpu1               |
+------------+-----------------+----------------------------+

System Clock
============

The MCX-N947 SoC is configured to use PLL0 running at 150MHz as a source for
the system clock.

Serial Port
===========

The FRDM-MCXN947 SoC has 10 FLEXCOMM interfaces for serial communication.
Flexcomm 4 is configured as UART for the console.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Configuring a Debug Probe
=========================

A debug probe is used for both flashing and debugging the board. This board is
configured by default to use the MCU-Link CMSIS-DAP Onboard Debug Probe.

Using LinkServer
----------------

Linkserver is the default runner for this board, and supports the factory
default MCU-Link firmware. Follow the instructions in
:ref:`mcu-link-cmsis-onboard-debug-probe` to reprogram the default MCU-Link
firmware. This only needs to be done if the default onboard debug circuit
firmware was changed. To put the board in ``DFU mode`` to program the firmware,
short jumper J21.

Using J-Link
------------

There are two options. The onboard debug circuit can be updated with Segger
J-Link firmware by following the instructions in
:ref:`mcu-link-jlink-onboard-debug-probe`.
To be able to program the firmware, you need to put the board in ``DFU mode``
by shortening the jumper J21.
The second option is to attach a :ref:`jlink-external-debug-probe` to the
10-pin SWD connector (J23) of the board. Additionally, the jumper J19 must
be shortened.
For both options use the ``-r jlink`` option with west to use the jlink runner.

.. code-block:: console

   west flash -r jlink

Configuring a Console
=====================

Connect a USB cable from your PC to J17, and use the serial terminal of your choice
(minicom, putty, etc.) with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Flashing
========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: frdm_mcxn947/mcxn947/cpu0
   :goals: flash

Open a serial terminal, reset the board (press the RESET button), and you should
see the following message in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build v3.6.0-479-g91faa20c6741 ***
   Hello World! frdm_mcxn947/mcxn947/cpu0

Building a dual-core image
--------------------------

The dual-core samples are run using ``frdm_mcxn947/mcxn947/cpu0`` target.

Images built for ``frdm_mcxn947/mcxn947/cpu1`` will be loaded from flash
and executed on the second core when :kconfig:option:`CONFIG_SECOND_CORE_MCUX` is selected.

For an example of building for both cores with System Build, see
:zephyr_file:`samples/subsys/ipc/ipc_service/static_vrings`

Here is an example for the :zephyr:code-sample:`mbox_data` application.

.. zephyr-app-commands::
   :app: zephyr/samples/drivers/mbox_data
   :board: frdm_mcxn947/mcxn947/cpu0
   :goals: flash
   :west-args: --sysbuild

Debugging
=========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: frdm_mcxn947/mcxn947/cpu0
   :goals: debug

Open a serial terminal, step through the application in your debugger, and you
should see the following message in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build v3.6.0-479-g91faa20c6741 ***
   Hello World! frdm_mcxn947/mcxn947/cpu0

Debugging a dual-core image
---------------------------

For dual core builds, the secondary core should be placed into a loop,
then a debugger can be attached.
As a reference please see (`AN13264`_, section 4.2.3 for more information).
The reference is for the RT1170 but similar technique can be also used here.

Using QSPI board variant
========================
The FRDM-MCXN947 board includes an external QSPI flash.  The MCXN947 can boot and
XIP directly from this flash using the FlexSPI interface.  The QSPI variant
enables building applications and code to execute from the QSPI.

Programming the ROM bootloader for external QSPI
------------------------------------------------
By default, the MCXN947 bootloader in ROM will boot using internal flash.  But
the MCU can be programmed to boot from external memory on the FlexSPI interface.
Before using the QSPI board variant, the board should be programmed to boot from
QSPI using the steps below.

To configure the ROM bootloader, the Protected Flash Region (PFR) must be
programmed.  Programming the PFR is done using NXP's ROM bootloader tools.
Some simple steps are provided in NXP's
`MCUXpresso SDK example hello_world_qspi_xip readme`_.  The binary to program
with blhost is found at `bootfromflexspi.bin`_.  A much more detailed explanation
is available at this post `Running code from external memory with MCX N94x`_.
The steps below program the FRDM-MCXN947 board.  Note that these steps interface
to the ROM bootloader through the UART serial port, but USB is another option.

1. Disconnect any terminal from the UART serial port, since these steps use that
   serial port.
#. Connect a USB Type-C cable to the host computer and J17 on the board, in the
   upper left corner.  This powers the board, connects the debug probe, and
   connects the UART serial port used for the ``blhost`` command.
#. Place the MCU in ISP mode.  On the FRDM-MCXN947 board, the ISP button
   can be used for this.  Press and hold the ISP button SW3, on the bottom right
   corner of the board.  Press and release the Reset button SW1 on the upper left
   corner of the board.  The MCU has booted into ISP mode.  Release the ISP
   button.
#. Run the ``blhost`` command:

.. tabs::

   .. group-tab:: Ubuntu

      This step assumes the MCU serial port is connected to `/dev/ttyACM0`

      .. code-block:: shell

         blhost -t 2000 -p /dev/ttyACM0,115200 -j -- write-memory 0x01004000 bootfromflexspi.bin

   .. group-tab:: Windows

      Change `COMxx` to match the COM port number connected to the MCU serial port.

      .. code-block:: shell

         blhost -t 2000 -p COMxx -j -- write-memory 0x01004000 bootfromflexspi.bin

Successful programming should look something like this:

.. code-block:: console

   $ blhost -t 2000 -p /dev/ttyACM0,115200 -j -- write-memory 0x01004000 bootfromflexspi.bin
   {
      "command": "write-memory",
      "response": [
         256
      ],
      "status": {
         "description": "0 (0x0) Success.",
         "value": 0
      }
   }

5. Reset the board with SW1 to exit ISP mode.  Now the MCU is ready to boot from
   QSPI.

The ROM bootloader can be configured to boot from internal flash again.  Repeat
the steps above to program the PFR, and program the file `bootfromflash.bin`_.

Build, flash, and debug with the QSPI variant
---------------------------------------------

Once the PFR is programmed to boot from QSPI, the normal Zephyr steps to build,
flash, and debug can be used with the QSPI board variant.  Here are some examples.

Here is an example for the :zephyr:code-sample:`hello_world` application:

.. zephyr-app-commands::
   :app: zephyr/samples/hello_world
   :board: frdm_mcxn947//cpu0/qspi
   :goals: flash

MCUboot can also be used with the QSPI variant.  By default, this places the
MCUboot bootloader in the ``boot-partition`` in QSPI flash, with the application
images.  The ROM bootloader will boot first and load MCUboot in the QSPI, which
will load the app.  This example builds and loads the :zephyr:code-sample:`blinky`
sample with MCUboot using Sysbuild:

.. zephyr-app-commands::
   :app: zephyr/samples/basic/blinky
   :board: frdm_mcxn947//cpu0/qspi
   :west-args: --sysbuild
   :gen-args: -DSB_CONFIG_BOOTLOADER_MCUBOOT=y
   :goals: flash

Open a serial terminal, reset the board with the SW1 button, and the console
will print:

.. code-block:: console

   *** Booting MCUboot vX.Y.Z ***
   *** Using Zephyr OS build vX.Y.Z ***
   I: Starting bootloader
   I: Image index: 0, Swap type: none
   I: Bootloader chainload address offset: 0x14000
   I: Image version: v0.0.0
   I: Jumping to the first image slot
   *** Booting Zephyr OS build vX.Y.Z ***
   LED state: OFF
   LED state: ON

Troubleshooting
===============

.. include:: ../../common/segger-ecc-systemview.rst.inc

.. include:: ../../common/board-footer.rst.inc

.. _MCX-N947 SoC Website:
   https://www.nxp.com/products/processors-and-microcontrollers/arm-microcontrollers/general-purpose-mcus/mcx-arm-cortex-m/mcx-n-series-microcontrollers/mcx-n94x-54x-highly-integrated-multicore-mcus-with-on-chip-accelerators-intelligent-peripherals-and-advanced-security:MCX-N94X-N54X

.. _MCX-N947 Datasheet:
   https://www.nxp.com/docs/en/data-sheet/MCXNx4xDS.pdf

.. _MCX-N947 Reference Manual:
   https://www.nxp.com/webapp/Download?colCode=MCXNX4XRM

.. _FRDM-MCXN947 Website:
   https://www.nxp.com/design/design-center/development-boards/general-purpose-mcus/frdm-development-board-for-mcx-n94-n54-mcus:FRDM-MCXN947

.. _FRDM-MCXN947 User Guide:
   https://www.nxp.com/document/guide/getting-started-with-frdm-mcxn947:GS-FRDM-MCXNXX

.. _FRDM-MCXN947 Board User Manual:
   https://www.nxp.com/webapp/Download?colCode=UM12018

.. _FRDM-MCXN947 Schematics:
   https://www.nxp.com/webapp/Download?colCode=90818-MCXN947SH

.. _AN13264:
   https://www.nxp.com/docs/en/application-note/AN13264.pdf

.. _MCUXpresso SDK example hello_world_qspi_xip readme:
   https://github.com/nxp-mcuxpresso/mcuxsdk-examples/blob/main/_boards/frdmmcxn947/demo_apps/hello_world_qspi_xip/example_board_readme.md

.. _bootfromflash.bin:
   https://github.com/nxp-mcuxpresso/mcuxsdk-examples/blob/main/_boards/frdmmcxn947/demo_apps/hello_world_qspi_xip/cm33_core0/bootfromflash.bin

.. _bootfromflexspi.bin:
   https://github.com/nxp-mcuxpresso/mcuxsdk-examples/blob/main/_boards/frdmmcxn947/demo_apps/hello_world_qspi_xip/cm33_core0/bootfromflexspi.bin

.. _Running code from external memory with MCX N94x:
   https://community.nxp.com/t5/MCX-Microcontrollers-Knowledge/Running-code-from-external-memory-with-MCX-N94x/ta-p/1792204
