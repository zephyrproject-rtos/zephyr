.. zephyr:board:: lpcxpresso54114

Overview
********

The LPCXpresso54114 board has been developed by NXP to enable evaluation of and
prototyping with the low-power LPC54110 family of MCUs. LPCXpresso* is a
low-cost development platform available from NXP supporting NXP's ARM-based
microcontrollers. LPCXpresso is an end-to-end solution enabling embedded
engineers to develop their applications from initial evaluation to final
production.

Hardware
********

- LPC54114 dual-core (M4F and dual M0) MCU running at up to 100 MHz
- On-board high-speed USB based debug probe with CMSIS-DAP and J-Link protocol
  support, can debug the on-board LPC54114 or an external target
- External debug probe option
- Tri-color LED, target Reset, ISP & interrupt/user buttons for easy testing of
  software functionality
- Expansion options based on Arduino UNO and Pmod™, plus additional expansion
  port pins
- On-board 1.8 V and 3.3 V regulators plus external power supply option
- 8 Mb Macronix MX25R SPI flash
- Built-in MCU power consumption and supply voltage measurement
- UART, I²C and SPI port bridging from LPC54114 target to USB via the on-board
  debug probe
- FTDI UART connector

For more information about the LPC54114 SoC and LPCXPRESSO54114 board:

- `LPC54114 SoC Website`_
- `LPC54114 Datasheet`_
- `LPC54114 Reference Manual`_
- `LPCXPRESSO54114 Website`_
- `LPCXPRESSO54114 User Guide`_
- `LPCXPRESSO54114 Schematics`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The LPC54114 SoC has IOCON registers, which can be used to configure the
functionality of a pin.

+---------+-----------------+----------------------------+
| Name    | Function        | Usage                      |
+=========+=================+============================+
| PIO0_0  | USART           | USART RX                   |
+---------+-----------------+----------------------------+
| PIO0_1  | USART           | USART TX                   |
+---------+-----------------+----------------------------+
| PIO0_18 | SPI             | SPI MISO                   |
+---------+-----------------+----------------------------+
| PIO0_19 | SPI             | SPI SCK                    |
+---------+-----------------+----------------------------+
| PIO0_20 | SPI             | SPI MOSI                   |
+---------+-----------------+----------------------------+
| PIO0_25 | I2C             | I2C SCL                    |
+---------+-----------------+----------------------------+
| PIO0_26 | I2C             | I2C SDA                    |
+---------+-----------------+----------------------------+
| PIO0_29 | GPIO            | RED LED                    |
+---------+-----------------+----------------------------+
| PIO1_1  | SPI             | SPI SSEL2                  |
+---------+-----------------+----------------------------+
| PIO1_9  | GPIO            | BLUE_LED                   |
+---------+-----------------+----------------------------+
| PIO1_10 | GPIO            | GREEN LED                  |
+---------+-----------------+----------------------------+

System Clock
============

The LPC54114 SoC is configured to use the internal FRO at 48MHz as a source for
the system clock. Other sources for the system clock are provided in the SOC,
depending on your system requirements.

Serial Port
===========

The LPC54114 SoC has 8 FLEXCOMM interfaces for serial communication.  One is
configured as USART for the console and the remaining are not used.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Configuring a Debug Probe
=========================

A debug probe is used for both flashing and debugging the board. This board is
configured by default to use the LPC-Link2 CMSIS-DAP Onboard Debug Probe,
however the :ref:`pyocd-debug-host-tools` do not support this probe so you must
reconfigure the board for one of the following debug probes instead.

:ref:`lpclink2-jlink-onboard-debug-probe`
-----------------------------------------

Install the :ref:`jlink-debug-host-tools` and make sure they are in your search
path.

Follow the instructions in :ref:`lpclink2-jlink-onboard-debug-probe` to program
the J-Link firmware.

Configuring a Console
=====================

Regardless of your choice in debug probe, we will use the LPC-Link2
microcontroller as a usb-to-serial adapter for the serial console.

Connect a USB cable from your PC to J5

Use the following settings with your serial terminal of choice (minicom, putty,
etc.):

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Flashing
========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: lpcxpresso54114/lpc54114/m4
   :goals: flash

Open a serial terminal, reset the board (press the SW4 button), and you should
see the following message in the terminal:

.. code-block:: console

   ***** Booting Zephyr OS v1.14.0-rc1 *****
   Hello World! lpcxpresso54114_m4

Debugging
=========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: lpcxpresso54114/lpc54114/m4
   :goals: debug

Open a serial terminal, step through the application in your debugger, and you
should see the following message in the terminal:

.. code-block:: console

   ***** Booting Zephyr OS v1.14.0-rc1 *****
   Hello World! lpcxpresso54114_m4

.. include:: ../../common/board-footer.rst
   :start-after: nxp-board-footer

.. _LPC54114 SoC Website:
   https://www.nxp.com/products/processors-and-microcontrollers/arm-based-processors-and-mcus/lpc-cortex-m-mcus/lpc54000-series-cortex-m4-mcus/low-power-microcontrollers-mcus-based-on-arm-cortex-m4-cores-with-optional-cortex-m0-plus-co-processor:LPC541XX

.. _LPC54114 Datasheet:
   https://www.nxp.com/docs/en/data-sheet/LPC5411X.pdf

.. _LPC54114 Reference Manual:
   https://www.nxp.com/webapp/Download?colCode=UM10914

.. _LPCXPRESSO54114 Website:
   https://www.nxp.com/support/developer-resources/evaluation-and-development-boards/lpcxpresso-boards/lpcxpresso54114-board:OM13089

.. _LPCXPRESSO54114 User Guide:
   https://www.nxp.com/webapp/Download?colCode=UM10973

.. _LPCXPRESSO54114 Schematics:
   https://www.nxp.com/downloads/en/design-support/LPCX5411x_Schematic_Rev_A1.pdf
