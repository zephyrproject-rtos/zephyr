.. zephyr:board:: lp_em_cc2340r5

Overview
********

The Texas Instruments CC2340R5 LaunchPad |trade| (LP_EM_CC2340R5) is a
development kit for the SimpleLink |trade| multi-Standard CC2340R5 wireless MCU.

See the `TI CC2340R5 LaunchPad Product Page`_ for details.

Hardware
********

The CC2340R5 LaunchPad |trade| development kit features the CC2340R5 wireless MCU.
The board is equipped with two LEDs, two push buttons and BoosterPack connectors
for expansion.

The CC2340R5 wireless MCU has a 48 MHz Arm |reg| Cortex |reg|-M0+ SoC and an
integrated 2.4 GHz transceiver supporting multiple protocols including Bluetooth
|reg| Low Energy and IEEE |reg| 802.15.4.

See the `TI CC2340R5 Product Page`_ for additional details.

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
| DIO0  | GPIO      |                     |
+-------+-----------+---------------------+
| DIO1  | ANALOG_IO | A4                  |
+-------+-----------+---------------------+
| DIO2  | ANALOG_IO | A3                  |
+-------+-----------+---------------------+
| DIO5  | ANALOG_IO | A5                  |
+-------+-----------+---------------------+
| DIO6  | SPI_CSN   | SPI CS              |
+-------+-----------+---------------------+
| DIO7  | ANALOG_IO | A0                  |
+-------+-----------+---------------------+
| DIO8  | GPIO      |                     |
+-------+-----------+---------------------+
| DIO9  | GPIO      | Button 2            |
+-------+-----------+---------------------+
| DIO10 | GPIO      | Button 1            |
+-------+-----------+---------------------+
| DIO11 | SPI_CSN   | SPI CS              |
+-------+-----------+---------------------+
| DIO12 | SPI_POCI  | SPI POCI            |
+-------+-----------+---------------------+
| DIO13 | SPI_PICO  | SPI_PICO            |
+-------+-----------+---------------------+
| DIO14 | GPIO      | Red LED             |
+-------+-----------+---------------------+
| DIO15 | GPIO      | Green LED           |
+-------+-----------+---------------------+
| DIO18 | SPI_CLK   | SPI CLK             |
+-------+-----------+---------------------+
| DIO19 | GPIO      |                     |
+-------+-----------+---------------------+
| DIO20 | UART0_TX  | UART TX             |
+-------+-----------+---------------------+
| DIO21 | GPIO      |                     |
+-------+-----------+---------------------+
| DIO22 | UART0_RX  | UART RX             |
+-------+-----------+---------------------+
| DIO23 | ANALOG_IO | A8                  |
+-------+-----------+---------------------+
| DIO24 | ANALOG_IO | A7                  |
+-------+-----------+---------------------+

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The LP_EM_CC2340R5 requires an external debug probe such as the LP-XDS110 or
LP-XDS110ET.

Alternativly a J-Link could be used on the J4 header, in combination with a 3.3V
power supply over the pinheader.
Debugging and flashing is currently only tested using a J-Link on the J4 header.

To get a console, connect an external USB to UART converter with:

+----+-------+
| TX | DIO22 |
+----+-------+
| RX | DIO20 |
+----+-------+

Then you can connect to it from you PC:

.. code-block:: console

   $ microcom -p <tty_device>

Replace :code:`<tty_device>` with the port of your USB to UART converter.
For example, :code:`/dev/ttyUSB0`.

Flashing
========

Applications for the ``CC2340R5 LaunchPad`` board configuration can be built and
flashed in the usual way (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: lp_em_cc2340r5
   :goals: build flash

Debugging
=========

You can debug an application in the usual way. Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: lp_em_cc2340r5
   :goals: debug

References
**********

CC2340R5 LaunchPad Quick Start Guide:
  https://www.ti.com/lit/pdf/swru588

.. _TI CC2340R5 LaunchPad Product Page:
   https://www.ti.com/tool/LP-EM-CC2340R5

.. _TI CC2340R5 Product Page:
   https://www.ti.com/product/CC2340R5
