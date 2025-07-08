.. zephyr:board:: frdm_mcxw23

Overview
********

The FRDM-MCXW23 is a compact and scalable development board featuring
the MCX W23 Bluetooth Low Energy (LE) 5.3 wireless MCU. The board is
ideal for prototyping battery-powered, miniaturized devices within portable
medical, home appliance and automation, and asset tracking applications.
Equipped with an on-board accelerometer, temperature sensor, data flash
and LED's, it operates from a 3V coincell with integrated buck DCDC or from
the USB bus power. The board offers easy evaluation and rapid prototyping of
the MCX W23 wireless MCU.

Hardware
********

- MCXW23 Arm® Cortex®-M33 microcontroller running at up to 32 MHz
- 1 MB flash and 128 KB SRAM on-chip
- 40HVQFN package
- On-board MCU-Link debugger with CMSIS-DAP
- Tri-color LED
- Reset, ISP, wake, and user buttons for easy testing of software functionality
- NXP FXLS8974CFR3 accelerometer
- P3T1755 temperature sensor
- 64Mbit (8MB) on board NOR FLASH
- Arduino Header, mikroBUS, Pmod

For more information about the MCXW236 SoC and FRDM-MCXW23 board, see:

- `MCXW23 SoC Website`_
- `MCXW23 Datasheet`_
- `MCXW23 Reference Manual`_
- `FRDM-MCXW23 Website`_
- `FRDM-MCXW23 User Manual`_
- `FRDM-MCXW23 Development Board Design Files`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The MCXW23 SoC has IOCON registers, which can be used to configure
the functionality of a pin.

+---------+-----------------+----------------------------+
| Name    | Function        | Usage                      |
+=========+=================+============================+
| PIO0_20 | GPIO            | ISP SW3                    |
+---------+-----------------+----------------------------+
| PIO0_6  | SPI             | SPI MOSI                   |
+---------+-----------------+----------------------------+
| PIO0_2  | USART           | USART RX                   |
+---------+-----------------+----------------------------+
| PIO0_3  | USART           | USART TX                   |
+---------+-----------------+----------------------------+
| PIO0_5  | SPI             | SPI SSEL0                  |
+---------+-----------------+----------------------------+
| PIO0_8  | SPI             | SPI SSEL1                  |
+---------+-----------------+----------------------------+
| PIO0_10 | SPI             | SPI SCK                    |
+---------+-----------------+----------------------------+
| PIO0_7  | SPI             | SPI MISO                   |
+---------+-----------------+----------------------------+
| PIO0_1  | GPIO            | RED LED                    |
+---------+-----------------+----------------------------+
| PIO0_4  | GPIO            | BLUE_LED                   |
+---------+-----------------+----------------------------+
| PIO0_0  | GPIO            | GREEN LED                  |
+---------+-----------------+----------------------------+
| PIO0_19 | GPIO            | USER LED                   |
+---------+-----------------+----------------------------+
| PIO0_18 | GPIO            | USR SW2                    |
+---------+-----------------+----------------------------+
| PIO0_21 | GPIO            | Wakeup SW5                 |
+---------+-----------------+----------------------------+
| PIO0_14 | I2C             | I2C SCL                    |
+---------+-----------------+----------------------------+
| PIO0_13 | I2C             | I2C SDA                    |
+---------+-----------------+----------------------------+
| PIO0_15 | GPIO            | FXLS8974CFR3 INT1          |
+---------+-----------------+----------------------------+
| PIO0_16 | GPIO            | FXLS8974CFR3 INT2          |
+---------+-----------------+----------------------------+
| PIO0_16 | GPIO            | P3T1755 INT2               |
+---------+-----------------+----------------------------+

System Clock
============

The MCXW23 SoC is configured to use FRO running at 32 MHz as a system clock source.

Serial Port
===========

The MCXW23 SoC has 3 FLEXCOMM interfaces for serial
communication. One is configured as USART for the console, one is
configured for I2C, and the other one is configured for SPI.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Build and flash applications as usual (see :ref:`build_an_application`
and :ref:`application_run` for more details).

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
   :board: frdm_mcxw23
   :goals: flash

Open a serial terminal, reset the board (press the RESET button), and you should
see the following message in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build v4.2.0-2105-g48f2ffda26de ***
   Hello World! frdm_mcxw23/mcxw236

Debugging
=========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: frdm_mcxw23
   :goals: debug

Open a serial terminal, step through the application in your debugger, and you
should see the following message in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build v4.2.0-2105-g48f2ffda26de ***
   Hello World! frdm_mcxw23/mcxw236

.. include:: ../../common/board-footer.rst
   :start-after: nxp-board-footer

.. _MCXW23 SoC Website:
   https://www.nxp.com/products/MCX-W23

.. _MCXW23 Datasheet:
    https://www.nxp.com/docs/en/data-sheet/MCXW23.pdf

.. _MCXW23 Reference Manual:
   https://www.nxp.com/webapp/Download?colCode=MCXW23RM

.. _FRDM-MCXW23 Website:
   https://www.nxp.com/design/design-center/development-boards-and-designs/FRDM-MCXW23

.. _FRDM-MCXW23 User Manual:
   https://www.nxp.com/webapp/Download?colCode=UM12359

.. _FRDM-MCXW23 Development Board Design Files:
   https://www.nxp.com/downloads/en/design-support/FRDM-MCXW23-Design-Files.zip
