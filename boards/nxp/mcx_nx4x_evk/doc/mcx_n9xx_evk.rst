.. zephyr:board:: mcx_n9xx_evk

Overview
********

MCX-N9XX-EVK is a full featured evaluation kit for prototyping of MCX N94 / N54
MCUs. They offer industry standard headers for access to the MCU’s I/Os,
integrated open-standard serial interfaces and an on-board MCU-Link debugger
with power measurement capability.  MCX N Series are high-performance, low-power
microcontrollers with intelligent peripherals and accelerators providing
multi-tasking capabilities and performance efficiency.

Hardware
********

- MCX-N947 Dual Arm Cortex-M33 microcontroller running at 150 MHz
- 2MB dual-bank on chip Flash
- 512 KB RAM
- External Quad SPI flash over FlexSPI
- USB high-speed (Host/Device) with on-chip HS PHY.
- USB full-speed (Host/Device) with on-chip FS PHY.
- 10x LP Flexcomms each supporting SPI, I2C, UART
- FlexCAN with FD, I3Cs, SAI
- 1x Ethernet with QoS
- On-board MCU-Link debugger with CMSIS-DAP
- Arduino Header, FlexIO/LCD Header, mikroBUS, M.2

For more information about the MCX-N947 SoC and MCX-N9XX-EVK board, see:

- `MCX-N947 SoC Website`_
- `MCX-N947 Datasheet`_
- `MCX-N947 Reference Manual`_
- `MCX-N9XX-EVK Website`_
- `MCX-N9XX-EVK Board User Manual`_
- `MCX-N9XX-EVK Schematics`_

Supported Features
==================

.. zephyr:board-supported-hw::

Shields for Supported Features
==============================
Some features in the table above are tested with Zephyr shields.  These shields
are tested on this board:
- :ref:`lcd_par_s035` - supports the Display interface.  This board uses the
MIPI_DBI interface of the shield, connected to the FlexIO on-chip peripheral.

Dual Core samples
*****************

+-----------+-------------------+----------------------+
| Core      | Boot Address      | Comment              |
+===========+===================+======================+
| CPU0      | 0x10000000[1856K] | primary core flash   |
+-----------+-------------------+----------------------+
| CPU1      | 0x101d0000[192K]  | secondary core flash |
+-----------+-------------------+----------------------+

+----------+------------------+-----------------------+
| Memory   | Address[Size]    | Comment               |
+==========+==================+=======================+
| srama    | 0x20000000[320k] | CPU0 ram              |
+----------+------------------+-----------------------+
| sramg    | 0x20050000[64k]  | CPU1 ram              |
+----------+------------------+-----------------------+
| sramh    | 0x20060000[32k]  | Shared memory         |
+----------+------------------+-----------------------+

Targets available
==================

The default configuration file
:zephyr_file:`boards/nxp/mcx_nx4x_evk/mcx_n9xx_evk_mcxn947_cpu0_defconfig`
only enables the first core. CPU0 is the only target that can run standalone.

CPU1 does not work without CPU0 enabling it.

To enable CPU1, create System Build application project and enable the
second core with config :kconfig:option:`CONFIG_SECOND_CORE_MCUX`.

Please have a look at some already enabled samples:

- :zephyr:code-sample:`ipc-static-vrings`
- :zephyr:code-sample:`openamp`
- :zephyr:code-sample:`mbox`
- :zephyr:code-sample:`mbox_data`

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

The MCX-N9XX-EVK SoC has 10 FLEXCOMM interfaces for serial communication.
Flexcomm 4 is configured as UART for the console.

Ethernet
========
To use networking samples with the Ethernet jack, change jumper JP13 to pins 2-3.

Programming and Debugging
*************************

Build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Configuring a Debug Probe
=========================

A debug probe is used for both flashing and debugging the board. This board is
configured by default to use the MCU-Link CMSIS-DAP Onboard Debug Probe.

Using LinkServer
----------------

LinkServer is the default runner for this board, and supports the factory
default MCU-Link firmware. Follow the instructions in
:ref:`mcu-link-cmsis-onboard-debug-probe` to reprogram the default MCU-Link
firmware. This only needs to be done if the default onboard debug circuit
firmware was changed. To put the board in ``ISP mode`` to program the firmware,
short jumper JP24.

Using J-Link
------------

There are two options. The onboard debug circuit can be updated with Segger
J-Link firmware by following the instructions in
:ref:`mcu-link-jlink-onboard-debug-probe`.
To be able to program the firmware, you need to put the board in ``ISP mode``
by shortening the jumper JP24.
The second option is to attach a :ref:`jlink-external-debug-probe` to the
20-pin SWD connector (J11) of the board. Additionally, the jumper JP6 must
be shorted.
For both options use the ``-r jlink`` option with west to use the jlink runner.

.. code-block:: console

   west flash -r jlink

Configuring a Console
=====================

Connect a USB cable from your PC to J5, and use the serial terminal of your choice
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
   :board: mcx_n9xx_evk/mcxn947/cpu0
   :goals: flash

Open a serial terminal, reset the board (press the RESET button), and you should
see the following message in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build vX.X.X ***
   Hello World! mcx_n9xx_evk/mcxn947/cpu0

Building a dual-core image
==========================

The dual-core samples are run using ``mcx_n9xx_evk/mcxn947/cpu0`` target.

Images built for ``mcx_n9xx_evk/mcxn947/cpu1`` will be loaded from flash
and executed on the second core when :kconfig:option:`CONFIG_SECOND_CORE_MCUX` is selected.

For an example of building for both cores with System Build, see
:zephyr:code-sample:`ipc-static-vrings`

Here is an example for the :zephyr:code-sample:`mbox_data` application.

.. zephyr-app-commands::
   :app: zephyr/samples/drivers/mbox_data
   :board: mcx_n9xx_evk/mcxn947/cpu0
   :goals: flash
   :west-args: --sysbuild

Debugging
=========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: mcx_n9xx_evk/mcxn947/cpu0
   :goals: debug

Open a serial terminal, step through the application in your debugger, and you
should see the following message in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build vX.X.X ***
   Hello World! mcx_n9xx_evk/mcxn947/cpu0

Debugging a dual-core image
---------------------------

For dual core builds, the secondary core should be placed into a loop,
then a debugger can be attached.
As a reference please see (`AN13264`_, section 4.2.3 for more information).
The reference is for the RT1170 but similar technique can be also used here.

Using QSPI board variant
========================
The MCX-N9XX-EVK board includes an external QSPI flash.  The MCXN947 can boot and
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
The steps below program the MCX-N9XX-EVK board.  Note that these steps interface
to the ROM bootloader through the UART serial port, but USB is another option.

1. Disconnect any terminal from the UART serial port, since these steps use that
   serial port.
#. Connect a micro USB cable to the host computer and J5 on the board, in the
   upper left corner.  This powers the board, connects the debug probe, and
   connects the UART serial port used for the ``blhost`` command.
#. Place the MCU in ISP mode.  On the MCX-N9XX-EVK board, the ISP button
   can be used for this.  Press and hold the ISP button SW3, on the bottom right
   corner of the board.  Press and release the Reset button SW1 on the lower left
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
   :board: mcx_n9xx_evk//cpu0/qspi
   :goals: flash

MCUboot can also be used with the QSPI variant.  By default, this places the
MCUboot bootloader in the ``boot-partition`` in QSPI flash, with the application
images.  The ROM bootloader will boot first and load MCUboot in the QSPI, which
will load the app.  This example builds and loads the :zephyr:code-sample:`blinky`
sample with MCUboot using Sysbuild:

.. zephyr-app-commands::
   :app: zephyr/samples/basic/blinky
   :board: mcx_n9xx_evk//cpu0/qspi
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

.. include:: ../../common/segger-ecc-systemview.rst
   :start-after: segger-ecc-systemview

.. include:: ../../common/board-footer.rst
   :start-after: nxp-board-footer

.. _MCX-N947 SoC Website:
   https://www.nxp.com/products/processors-and-microcontrollers/arm-microcontrollers/general-purpose-mcus/mcx-arm-cortex-m/mcx-n-series-microcontrollers/mcx-n94x-54x-highly-integrated-multicore-mcus-with-on-chip-accelerators-intelligent-peripherals-and-advanced-security:MCX-N94X-N54X

.. _MCX-N947 Datasheet:
   https://www.nxp.com/docs/en/data-sheet/MCXNx4xDS.pdf

.. _MCX-N947 Reference Manual:
   https://www.nxp.com/webapp/Download?colCode=MCXNX4XRM

.. _MCX-N9XX-EVK Website:
   https://www.nxp.com/design/design-center/development-boards-and-designs/MCX-N9XX-EVK

.. _MCX-N9XX-EVK Board User Manual:
   https://www.nxp.com/webapp/Download?colCode=UM12036

.. _MCX-N9XX-EVK Schematics:
   https://www.nxp.com/webapp/Download?colCode=SPF-55276

.. _AN13264:
   https://www.nxp.com/docs/en/application-note/AN13264.pdf

.. _MCUXpresso SDK example hello_world_qspi_xip readme:
   https://github.com/nxp-mcuxpresso/mcuxsdk-examples/blob/main/_boards/mcxn9xxevk/demo_apps/hello_world_qspi_xip/example_board_readme.md

.. _bootfromflash.bin:
   https://github.com/nxp-mcuxpresso/mcuxsdk-examples/blob/main/_boards/mcxn9xxevk/demo_apps/hello_world_qspi_xip/cm33_core0/bootfromflash.bin

.. _bootfromflexspi.bin:
   https://github.com/nxp-mcuxpresso/mcuxsdk-examples/blob/main/_boards/mcxn9xxevk/demo_apps/hello_world_qspi_xip/cm33_core0/bootfromflexspi.bin

.. _Running code from external memory with MCX N94x:
   https://community.nxp.com/t5/MCX-Microcontrollers-Knowledge/Running-code-from-external-memory-with-MCX-N94x/ta-p/1792204
