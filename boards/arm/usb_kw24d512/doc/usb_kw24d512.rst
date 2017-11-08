.. _usb_kw24d512:

NXP USB-KW24D512
################

Overview
********

The USB-KW24D512 is an evaluation board in a convenient USB dongle
form factor based on the NXP MKW24D512 System-in-Package (SiP) device
(KW2xD wireless MCU series).
MKW24D512 wireless MCU provides a low-power, compact device with
integrated IEEE 802.15.4 radio. The board can be used as a packet sniffer,
network node, border router or as a development board.

Hardware
********

- Kinetis KW2xD-2.4 GHz 802.15.4 Wireless Radio Microcontroller
  (50 MHz, 512 KB flash memory, 64 KB RAM, low-power, crystal-less USB)
- USB Type A Connector
- Two blue LEDs
- One user push button
- One reset button
- Integrated PCB Folded F-type antenna
- 10-pin (0.05”) JTAG debug port for target MCU

For more information about the KW2xD SiP and USB-KW24D512 board:

- `KW2xD Website`_
- `KW2xD Datasheet`_
- `KW2xD Reference Manual`_
- `USB-KW24D512 Website`_
- `USB-KW24D512 Hardware Reference Manual`_

Supported Features
==================

The USB-KW24D512 board configuration supports the following hardware features:

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
| I2C       | on-chip    | i2c                                 |
+-----------+------------+-------------------------------------+
| SPI       | on-chip    | spi                                 |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port-polling;                |
|           |            | serial port-interrupt               |
+-----------+------------+-------------------------------------+
| FLASH     | on-chip    | soc flash                           |
+-----------+------------+-------------------------------------+

The default configuration can be found in the defconfig file:

	``boards/arm/usb_kw24d512/usb_kw24d512_defconfig``

Other hardware features are not currently supported by the port.

Connections and IOs
===================

The KW2xD SoC has five pairs of pinmux/gpio controllers.

+-------+-----------------+--------------------------------------+
| Name  | Function        | Usage                                |
+=======+=================+======================================+
| PTA1  | UART0_RX        | UART Console                         |
+-------+-----------------+--------------------------------------+
| PTA2  | UART0_TX        | UART Console                         |
+-------+-----------------+--------------------------------------+
| PTC4  | GPIO            | SW1                                  |
+-------+-----------------+--------------------------------------+
| PTD4  | GPIO            | Blue LED (D2)                        |
+-------+-----------------+--------------------------------------+
| PTD5  | GPIO            | Blue LED (D3)                        |
+-------+-----------------+--------------------------------------+
| PTB10 | SPI1_PCS0       | internal connected to MCR20A         |
+-------+-----------------+--------------------------------------+
| PTB11 | SPI1_SCK        | internal connected to MCR20A         |
+-------+-----------------+--------------------------------------+
| PTB16 | SPI1_SOUT       | internal connected to MCR20A         |
+-------+-----------------+--------------------------------------+
| PTB17 | SPI1_SIN        | internal connected to MCR20A         |
+-------+-----------------+--------------------------------------+
| PTB19 | GPIO            | internal connected to MCR20A (Reset) |
+-------+-----------------+--------------------------------------+
| PTB3  | GPIO            | internal connected to MCR20A (IRQ_B) |
+-------+-----------------+--------------------------------------+
| PTC0  | GPIO            | internal connected to MCR20A (GPIO5) |
+-------+-----------------+--------------------------------------+

System Clock
============

USB-KW24D512 contains 32 MHz oscillator crystal, which is connected to the
clock pins of the radio transceiver. The MCU is configured to
use the 4 MHz external clock from the transceiver with the on-chip PLL
to generate a 48 MHz system clock.

Serial Port
===========

The KW2xD SoC has three UARTs. One is configured and can be used for the
console, but it uses the same pins as the JTAG interface and is only
accessible via the JTAG SWD connector.

Programming and Debugging
*************************

Currently only the J-Link tools and the `Segger J-Link debug probe`_ are
supported. The J-Link probe should be connected to the JTAG-SWD connector on
the USB-KW24D512 board. To use the Segger J-Link tools, follow the instructions
in the :ref:`nxp_opensda_jlink` page.

Flashing
========

The Segger J-Link firmware does not support command line flashing, therefore
the usual ``flash`` build target is not supported.

Debugging
=========

This example uses the :ref:`hello_world` sample with the
:ref:`nxp_opensda_jlink` tools. This builds the Zephyr application,
invokes the J-Link GDB server, attaches a GDB client, and programs the
application to flash. It will leave you at a gdb prompt.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: usb_kw24d512
   :goals: debug

In a second terminal, open telnet:

.. code-block:: console

     $ telnet localhost 19021
     Trying 127.0.0.1...
     Connected to localhost.
     Escape character is '^]'.
     SEGGER J-Link V6.16j - Real time terminal output
     SEGGER J-Link ARM V6.0, SN=xxxxxxxx
     Process: JLinkGDBServer

Continue program execution in GDB, then in the telnet terminal you should see:

.. code-block:: console

     ***** BOOTING ZEPHYR OS v1.8.99 - BUILD: Jul 26 2017 15:39:04 *****
     Hello World! arm

.. _USB-KW24D512 Website:
   http://www.nxp.com/products/microcontrollers-and-processors/arm-processors/kinetis-cortex-m-mcus/w-series-wireless-m0-plus-m4/ieee-802.15.4-packet-sniffer-usb-dongle-form-factor:USB-KW24D512

.. _USB-KW24D512 Hardware Reference Manual:
   http://www.nxp.com/docs/en/reference-manual/USB-KW2XHWRM.pdf

.. _KW2xD Website:
   http://www.nxp.com/products/wireless-connectivity/2.4-ghz-wireless-solutions/ieee-802.15.4-wireless-mcus/kinetis-kw2xd-2.4-ghz-802.15.4-wireless-radio-microcontroller-mcu-based-on-arm-cortex-m4-core:KW2xD

.. _KW2xD Datasheet:
   http://www.nxp.com/docs/en/data-sheet/MKW2xDxxx.pdf

.. _KW2xD Reference Manual:
   http://www.nxp.com/docs/en/reference-manual/MKW2xDxxxRM.pdf

.. _Segger J-Link debug probe:
    https://www.segger.com/products/debug-probes/j-link/models/j-link-base/
