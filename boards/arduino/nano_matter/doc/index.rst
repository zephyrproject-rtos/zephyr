.. zephyr:board:: arduino_nano_matter

Overview
********

The Nano Matter merges the well-known Arduino way of making complex technology more accessible with the
powerful MGM240S from Silicon Labs, to bring Matter closer to the maker world, in one of the
smallest form factors in the market.

It enables 802.15.4 (Thread®) and Bluetooth® Low Energy connectivity, to interact with Matter-compatible devices
with a user-friendly software layer ready for quick prototyping.

The Nano Matter features a compact and efficient architecture powered by the
MGM240S (32-bit Arm® Cortex®-M33) from Silicon Labs, a high-performance wireless module optimized for
the needs of battery and line-powered IoT devices for 2.4 GHz mesh networks.

Hardware
********

- MGM240SD22VNA2 Mighty Gecko SiP
- CPU core: ARM Cortex®-M33 with FPU
- Flash memory: 1536 kB
- RAM: 256 kB
- Transmit power: up to +20 dBm
- Operation frequency: 2.4 GHz
- Crystals for LFXO (32.768 kHz) and HFXO (39 MHz).
- User RGB LED
- User button

For more information about the EFR32MG24 SoC and the Arduino Nano Matter, refer to these
documents:

- `MGM240S Website`_
- `EFR32MG24 Datasheet`_
- `EFR32xG24 Reference Manual`_
- `Nano Matter User Manual`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

In the following table, the column **Name** contains Pin names. For example, PA2
means Pin number 2 on PORTA, as used in the board's datasheets and manuals.

+-------+-------------+------------------+
| Name  | Function    | Usage            |
+=======+=============+==================+
| PC1   | GPIO        | LED0             |
+-------+-------------+------------------+
| PC2   | GPIO        | LED1             |
+-------+-------------+------------------+
| PC3   | GPIO        | LED2             |
+-------+-------------+------------------+
| PA0   | GPIO        | Button           |
+-------+-------------+------------------+
| PC4   | USART0_TX   | UART Console TX  |
+-------+-------------+------------------+
| PC5   | USART0_RX   | UART Console RX  |
+-------+-------------+------------------+

System Clock
============

The MGM240S SiP is configured to run at 78 MHz using DPLL and the 39 MHz internal oscillator.

Serial Port
===========

The MGM240S SiP has one USART and two EUSARTs.
USART0 is connected to the board controller and is used for the console.

Programming and Debugging
*************************

The Arduino Nano Matter contains an SAMD11 with CMSIS-DAP, allowing flashing, debugging, logging, etc. over
the USB port. Doing so requires a version of OpenOCD that includes support for the flash on the MG24
MCU. Until those changes are included in stock OpenOCD, the version bundled with Arduino can be
used, or can be installed from the `OpenOCD Arduino Fork`_. When flashing, debugging, etc. you may
need to include ``--openocd=/usr/local/bin/openocd
--openocd-search=/usr/local/share/openocd/scripts/`` options to the command.

Flashing
========

Connect the Arduino Nano Matter board to your host computer using the USB port. A USB CDC ACM serial port
should appear on the host, that can be used to view logs from the flashed application.

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: arduino_nano_matter
   :goals: flash

Open a serial terminal (minicom, putty, etc.) connecting to the UCB CDC ACM serial port.

Reset the board and you should see the following message in the terminal:

.. code-block:: console

   Hello World! arduino_nano_matter


.. _Nano Matter User Manual:
   https://docs.arduino.cc/tutorials/nano-matter/user-manual/

.. _MGM240S Website:
   https://www.silabs.com/wireless/zigbee/efr32mg24-series-2-modules/device.mgm240sd22vna

.. _EFR32MG24 Datasheet:
   https://www.silabs.com/documents/public/data-sheets/efr32mg24-datasheet.pdf

.. _EFR32xG24 Reference Manual:
   https://www.silabs.com/documents/public/reference-manuals/brd4187c-rm.pdf

.. _OpenOCD Arduino Fork:
   https://github.com/facchinm/OpenOCD/tree/arduino-0.12.0-rtx5
