.. zephyr:board:: lp_em_cc2755r7_bg

Overview
********

The Texas Instruments CC2755R7_BG LaunchPad™ (LP_EM_CC2755R7_BG) is a
development kit for the SimpleLink™ multi-Standard CC2755R7_BG wireless MCU.

.. note::

   There is no dedicated CC2755R7_BG LaunchPad product on ti.com. This board
   definition is intended to be used with a :zephyr:board:`lp_em_cc2755r10_bg`
   LaunchPad that has a CC2755R7 SoC mounted in place of the CC2755R10 SoC.
   Refer to the CC2755R10_BG LaunchPad documentation on ti.com for the
   hardware design, schematics and user guide.

See the `TI CC2755R7_BG Product Page`_ for details on the wireless MCU.

Hardware
********

The CC2755R7_BG LaunchPad™ development kit features the CC2755R7_BG wireless MCU
(mounted on a CC2755R10_BG LaunchPad). The board is equipped with two LEDs, two
push buttons and BoosterPack connectors for expansion.

The CC2755R7_BG wireless MCU has a 96 MHz Arm® Cortex®-M33 SoC and an
integrated 2.4 GHz transceiver supporting multiple protocols including Bluetooth
® Low Energy and IEEE® 802.15.4.

The CC2755R7_BG wireless MCU supports additional protocols such as Matter, Thread, and Zigbee.

See the `TI CC2755R7_BG Product Page`_ for additional details.

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

All I/O signals are accessible from the BoosterPack connectors. Pin function
aligns with the LaunchPad standard.

+-------+-----------+-------------------------+
| Pin   | Function  | Usage                   |
+=======+===========+=========================+
| DIO0  | GPIO      |  Button 2               |
+-------+-----------+-------------------------+
| DIO1  | UART_TX   | UART TX                 |
+-------+-----------+-------------------------+
| DIO2  | UART_RX   | UART RX                 |
+-------+-----------+-------------------------+
| DIO3  | SPI_CLK   | SPI CLK                 |
+-------+-----------+-------------------------+
| DIO4  | SPI_POCI  | SPI POCI                |
+-------+-----------+-------------------------+
| DIO5  | SPI_PICO  | SPI PICO                |
+-------+-----------+-------------------------+
| DIO6  | GPIO      |                         |
+-------+-----------+-------------------------+
| DIO7  | SPI_CSN   | SPI CS                  |
+-------+-----------+-------------------------+
| DIO8  | GPIO      |                         |
+-------+-----------+-------------------------+
| DIO9  | SWD_IO    | SWD IO                  |
+-------+-----------+-------------------------+
| DIO10 | SWD_CK    | SWD CK                  |
+-------+-----------+-------------------------+
| DIO11 | GPIO      |                         |
+-------+-----------+-------------------------+
| DIO12 | GPIO      | Green LED               |
+-------+-----------+-------------------------+
| DIO13 | GPIO      |                         |
+-------+-----------+-------------------------+
| DIO14 | GPIO      |                         |
+-------+-----------+-------------------------+
| DIO15 | GPIO      |                         |
+-------+-----------+-------------------------+
| DIO16 | GPIO      | RED LED                 |
+-------+-----------+-------------------------+
| DIO17 | ANALOG_IO | CAN TX / A8             |
+-------+-----------+-------------------------+
| DIO18 | ANALOG_IO | CAN RX / A7             |
+-------+-----------+-------------------------+
| DIO19 | ANALOG_IO | I2S SDO / A6            |
+-------+-----------+-------------------------+
| DIO20 | ANALOG_IO | I2C SDI / A5 / Button 1 |
+-------+-----------+-------------------------+
| DIO21 | ANALOG_IO | A4                      |
+-------+-----------+-------------------------+
| DIO22 | ANALOG_IO |   A3                    |
+-------+-----------+-------------------------+
| DIO23 | GPIO      | X32P                    |
+-------+-----------+-------------------------+
| DIO24 | GPIO      | X32N                    |
+-------+-----------+-------------------------+
| DIO25 | ANALOG_IO | A2                      |
+-------+-----------+-------------------------+
| DIO26 | GPIO      |                         |
+-------+-----------+-------------------------+
| DIO27 | I2C_SCL   | I2C SCL / A1            |
+-------+-----------+-------------------------+
| DIO28 | I2C_SDA   | I2C SDA / A0            |
+-------+-----------+-------------------------+
| DIO29 | ANALOG_IO | A10                     |
+-------+-----------+-------------------------+
| DIO30 | ANALOG_IO | A9                      |
+-------+-----------+-------------------------+

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The LP_EM_CC2755R7_BG requires an external debug probe such as the LP-XDS110 or
LP-XDS110ET.

Flashing
========

Applications for the ``CC2755R7_BG LaunchPad`` board configuration can be built and
flashed in the usual way (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: lp_em_cc2755r7_bg
   :goals: build flash

Debugging
=========

You can debug an application in the usual way. Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: lp_em_cc2755r7_bg
   :goals: debug

References
**********

TI CC2755R10_BG LaunchPad Quick Start Guide:
   https://www.ti.com/lit/ug/swru635/swru635.pdf

.. _TI CC2755R7_BG Product Page:
   https://www.ti.com/product/CC2755R7
