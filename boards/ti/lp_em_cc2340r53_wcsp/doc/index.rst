.. zephyr:board:: lp_em_cc2340r53_wcsp

Overview
********

The Texas Instruments CC2340R53 WCSP LaunchPad™ (LP_EM_CC2340R53_WCSP) is a
development kit featuring the CC2340R53 WCSP (Wafer-Level Chip Scale Package)
mounted on an LP_EM_CC2340R5 base board.

See the `TI CC2340R53 LaunchPad Product Page`_ for details.

Hardware
********

The CC2340R53 WCSP LaunchPad™ development kit features the CC2340R53 wireless MCU
in the YBG package variant. The board is equipped with two LEDs, two push buttons
and BoosterPack connectors for expansion.

The CC2340R53 wireless MCU has a 48 MHz Arm® Cortex®-M0+ SoC and an
integrated 2.4 GHz transceiver supporting multiple protocols including Bluetooth®
Low Energy and IEEE® 802.15.4.

See the `TI CC2340R53 Product Page`_ for additional details.

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

All I/O signals are accessible from the BoosterPack connectors. Pin function
aligns with the LaunchPad standard.

+-------+-----------+---------------------+
| Pin   | Function  | Usage               |
+=======+===========+=====================+
| DIO6  | UART0_TX  | UART TX             |
+-------+-----------+---------------------+
| DIO8  | GPIO      | Green LED / I2C SDA |
+-------+-----------+---------------------+
| DIO11 | SPI_CSN   | SPI CS              |
+-------+-----------+---------------------+
| DIO12 | SPI_PICO  | SPI PICO            |
+-------+-----------+---------------------+
| DIO18 | SPI_CLK   | SPI CLK / Button 1  |
+-------+-----------+---------------------+
| DIO20 | UART0_RX  | UART RX             |
+-------+-----------+---------------------+
| DIO21 | SPI_POCI  | SPI POCI / Red LED  |
+-------+-----------+---------------------+
| DIO24 | GPIO      | I2C SCL / Button 2  |
+-------+-----------+---------------------+

Shared pin configuration
========================

Due to the limited pin count of the package, several peripheral functions
share pins with LEDs and buttons. By default, SPI and I2C are disabled in the
devicetree to allow the LEDs and buttons to function.

The following pins have multiple functions that conflict:

+-------+------------------+------------------------+
| Pin   | Function         | Jumper required        |
+=======+==================+========================+
| DIO8  | Green LED        | P2 pins 5-6            |
+-------+------------------+------------------------+
| DIO8  | I2C SDA          | P2 pins 3-5            |
+-------+------------------+------------------------+
| DIO18 | SPI SCLK         | P6 pins 3-5            |
+-------+------------------+------------------------+
| DIO18 | Button 1         | P6 pins 5-6            |
+-------+------------------+------------------------+
| DIO21 | Red LED          | P2 pins 2-1            |
+-------+------------------+------------------------+
| DIO21 | SPI POCI         | P2 pins 2-4            |
+-------+------------------+------------------------+
| DIO24 | Button 2         | P6 pins 2-1            |
+-------+------------------+------------------------+
| DIO24 | I2C SCL          | P6 pins 2-4            |
+-------+------------------+------------------------+

To enable SPI or I2C:

1. Install the appropriate jumpers on headers P2 and P6.
2. Note that the corresponding LED or button will no longer function.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The LP_EM_CC2340R53_WCSP requires an external debug probe such as the LP-XDS110 or
LP-XDS110ET.

Flashing
========

Applications for the ``CC2340R53 WCSP LaunchPad`` board configuration can be
built and flashed in the usual way (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: lp_em_cc2340r53_wcsp
   :goals: build flash

Debugging
=========

You can debug an application in the usual way. Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: lp_em_cc2340r53_wcsp
   :goals: debug

References
**********

CC2340R53 LaunchPad Quick Start Guide:
  https://www.ti.com/lit/pdf/swru588

.. _TI CC2340R53 LaunchPad Product Page:
   https://www.ti.com/tool/LP-EM-CC2340R5

.. _TI CC2340R53 Product Page:
   https://www.ti.com/product/CC2340R5
