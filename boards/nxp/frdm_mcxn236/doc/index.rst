.. _frdm_mcxn236:

NXP FRDM-MCXN236
################

Overview
********

FRDM-MCXN236 are compact and scalable development boards for rapid prototyping of
MCX N23X MCUs. They offer industry standard headers for easy access to the
MCUs I/Os, integrated open-standard serial interfaces, external flash memory and
an on-board MCU-Link debugger. MCX N Series are high-performance, low-power
microcontrollers with intelligent peripherals and accelerators providing multi-tasking
capabilities and performance efficiency.

.. image:: frdm_mcxn236.webp
   :align: center
   :alt: FRDM-MCXN236

Hardware
********

- MCX-N236 Arm Cortex-M33 microcontroller running at 150 MHz
- 1MB dual-bank on chip Flash
- 352 KB RAM
- USB high-speed (Host/Device) with on-chip HS PHY. HS USB Type-C connectors
- 8x LP Flexcomms each supporting SPI, I2C, UART
- 2x FlexCAN with FD, 2x I3Cs, 2x SAI
- On-board MCU-Link debugger with CMSIS-DAP
- Arduino Header, FlexIO/LCD Header, SmartDMA/Camera Header, mikroBUS

For more information about the MCX-N236 SoC and FRDM-MCXN236 board, see:

- `MCX-N236 SoC Website`_
- `MCX-N236 Datasheet`_
- `MCX-N236 Reference Manual`_
- `FRDM-MCXN236 Website`_
- `FRDM-MCXN236 User Guide`_
- `FRDM-MCXN236 Board User Manual`_
- `FRDM-MCXN236 Schematics`_

Supported Features
==================

The FRDM-MCXN236 board configuration supports the following hardware features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| SYSTICK   | on-chip    | systick                             |
+-----------+------------+-------------------------------------+
| PINMUX    | on-chip    | pinmux                              |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | gpio                                |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port-polling;                |
|           |            | serial port-interrupt               |
+-----------+------------+-------------------------------------+
| SPI       | on-chip    | spi                                 |
+-----------+------------+-------------------------------------+
| I2C       | on-chip    | i2c                                 |
+-----------+------------+-------------------------------------+
| CLOCK     | on-chip    | clock_control                       |
+-----------+------------+-------------------------------------+
| FLASH     | on-chip    | soc flash                           |
+-----------+------------+-------------------------------------+
| WATCHDOG  | on-chip    | watchdog                            |
+-----------+------------+-------------------------------------+
| VREF      | on-chip    | regulator                           |
+-----------+------------+-------------------------------------+
| ADC       | on-chip    | adc                                 |
+-----------+------------+-------------------------------------+

Targets available
==================

The default configuration file
:zephyr_file:`boards/nxp/frdm_mcxn236/frdm_mcxn236_defconfig`

Other hardware features are not currently supported by the port.

Connections and IOs
===================

The MCX-N236 SoC has 6 gpio controllers and has pinmux registers which
can be used to configure the functionality of a pin.

+------------+-----------------+----------------------------+
| Name       | Function        | Usage                      |
+============+=================+============================+
| P0_PIO1_8  | UART            | UART RX                    |
+------------+-----------------+----------------------------+
| P1_PIO1_9  | UART            | UART TX                    |
+------------+-----------------+----------------------------+

System Clock
============

The MCX-N236 SoC is configured to use PLL0 running at 150MHz as a source for
the system clock.

Serial Port
===========

The FRDM-MCXN236 SoC has 8 FLEXCOMM interfaces for serial communication.
Flexcomm 4 is configured as UART for the console.

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

Linkserver is the default runner for this board, and supports the factory
default MCU-Link firmware. Follow the instructions in
:ref:`mcu-link-cmsis-onboard-debug-probe` to reprogram the default MCU-Link
firmware. This only needs to be done if the default onboard debug circuit
firmware was changed. To put the board in ``DFU mode`` to program the firmware,
short jumper JP5.

Using J-Link
------------

There are two options. The onboard debug circuit can be updated with Segger
J-Link firmware by following the instructions in
:ref:`mcu-link-jlink-onboard-debug-probe`.
To be able to program the firmware, you need to put the board in ``DFU mode``
by shortening the jumper JP5.
The second option is to attach a :ref:`jlink-external-debug-probe` to the
10-pin SWD connector (J12) of the board. Additionally, the jumper JP7 must
be shortened.
For both options use the ``-r jlink`` option with west to use the jlink runner.

.. code-block:: console

   west flash -r jlink

Configuring a Console
=====================

Connect a USB cable from your PC to J10, and use the serial terminal of your choice
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
   :board: frdm_mcxn236
   :goals: flash

Open a serial terminal, reset the board (press the RESET button), and you should
see the following message in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build v3.6.0-4478-ge6c3a42f5f52 ***
   Hello World! frdm_mcxn236/mcxn236

Debugging
=========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: frdm_mcxn236/mcxn236
   :goals: debug

Open a serial terminal, step through the application in your debugger, and you
should see the following message in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build v3.6.0-4478-ge6c3a42f5f52 ***
   Hello World! frdm_mcxn236/mcxn236

.. _MCX-N236 SoC Website:
   https://www.nxp.com/products/processors-and-microcontrollers/arm-microcontrollers/general-purpose-mcus/mcx-arm-cortex-m/mcx-n-series-microcontrollers/mcx-n23x-highly-integrated-mcus-with-on-chip-accelerators-intelligent-peripherals-and-advanced-security:MCX-N23X

.. _MCX-N236 Datasheet:
   https://www.nxp.com/docs/en/data-sheet/MCXN23x.pdf

.. _MCX-N236 Reference Manual:
   https://www.nxp.com/docs/en/reference-manual/MCXN23xRM.pdf

.. _FRDM-MCXN236 Website:
   https://www.nxp.com/design/design-center/development-boards-and-designs/general-purpose-mcus/frdm-development-board-for-mcx-n23x-mcus:FRDM-MCXN236

.. _FRDM-MCXN236 User Guide:
   https://www.nxp.com/document/guide/getting-started-with-frdm-mcxn236:GS-FRDM-MCXN236

.. _FRDM-MCXN236 Board User Manual:
   https://www.nxp.com/docs/en/user-manual/UM12041.pdf

.. _FRDM-MCXN236 Schematics:
   https://www.nxp.com/webapp/Download?colCode=SPF-90828
