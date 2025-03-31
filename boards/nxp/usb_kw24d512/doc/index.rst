.. zephyr:board:: usb_kw24d512

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

.. zephyr:board-supported-hw::

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

USB
===

The KW2xD SoC has a USB OTG (USBOTG) controller that supports both
device and host functions. Only USB device function is supported in Zephyr
at the moment. The USB-KW24D512 board has a USB Type A connector and
can only be used in device mode.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Configuring a Debug Probe
=========================

A debug probe is used for both flashing and debugging the board. This board is
configured by default to use the :ref:`jlink-external-debug-probe`.

:ref:`jlink-external-debug-probe`
---------------------------------

Install the :ref:`jlink-debug-host-tools` and make sure they are in your search
path.

Attach a J-Link 10-pin connector to J1.

Configuring a Console
=====================

The console is available using `Segger RTT`_.

Connect a USB cable from your PC to J5.

Once you have started a debug session, run telnet:

.. code-block:: console

    Trying 127.0.0.1...
    Connected to localhost.
    Escape character is '^]'.
    SEGGER J-Link V6.44 - Real time terminal output
    SEGGER J-Link ARM V10.1, SN=600111924
    Process: JLinkGDBServerCLExe

Flashing
========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: usb_kw24d512
   :goals: flash

The Segger RTT console is only available during a debug session. Use ``attach``
to start one:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: usb_kw24d512
   :goals: attach

Run telnet as shown earlier, and you should see the following message in the
terminal:

.. code-block:: console

   ***** Booting Zephyr OS v1.14.0-rc1 *****
   Hello World! usb_kw24d512

Debugging
=========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: usb_kw24d512
   :goals: debug

Run telnet as shown earlier, step through the application in your debugger, and
you should see the following message in the terminal:

.. code-block:: console

   ***** Booting Zephyr OS v1.14.0-rc1 *****
   Hello World! usb_kw24d512

.. include:: ../../common/board-footer.rst
   :start-after: nxp-board-footer

.. _USB-KW24D512 Website:
   https://www.nxp.com/products/processors-and-microcontrollers/arm-based-processors-and-mcus/kinetis-cortex-m-mcus/w-serieswireless-conn.m0-plus-m4/ieee-802.15.4-packet-sniffer-usb-dongle-form-factor:USB-KW24D512

.. _USB-KW24D512 Hardware Reference Manual:
   https://www.nxp.com/webapp/Download?colCode=USB-KW2XHWRM

.. _KW2xD Website:
   https://www.nxp.com/products/wireless/thread/kinetis-kw2xd-2.4-ghz-802.15.4-wireless-radio-microcontroller-mcu-based-on-arm-cortex-m4-core:KW2xD

.. _KW2xD Datasheet:
   https://www.nxp.com/docs/en/data-sheet/MKW2xDxxx.pdf

.. _KW2xD Reference Manual:
   https://www.nxp.com/webapp/Download?colCode=MKW2XDXXXRM

.. _Segger RTT:
   https://www.segger.com/products/debug-probes/j-link/technology/about-real-time-transfer/
