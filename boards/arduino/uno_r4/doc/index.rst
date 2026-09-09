.. zephyr:board:: arduino_uno_r4

Overview
********

The Arduino UNO R4 Minima/WiFi is a development board featuring the Renesas RA4M1 SoC
in the Arduino form factor and is compatible with traditional Arduino.

Hardware
********

- Renesas RA4MA1 Processor (ARM Cortex-M4 at 48 MHz)
- 256 KiB flash memory and 32 KiB of RAM
- One user LEDs
- One reset button
- One WiFi Transceiver (Arduino UNO R4 WiFi only)
- One 12x8 LED Matrix (Arduino UNO R4 WiFi only)
- Built-in CMSIS-DAP debug adapter (Arduino UNO R4 WiFi only)

Supported Features
==================

.. zephyr:board-supported-hw::

Serial Console
==============

On the Arduino UNO R4 WiFi the console is routed over SCI9, which the on-board
ESP32-S3 bridges to the USB-C connector as a USB CDC ACM port. No external
adapter is needed -- open the same port you flash through, at 115200 baud.

Do not open that port at 1200 or 2400 baud: the bridge firmware treats those
line rates as a request to reset the RA4M1 or to enter its DFU bootloader.

The UART on the D0/D1 header (SCI2) remains available. To use it as the console
instead, override the choice in an application overlay:

.. code-block:: devicetree

   / {
       chosen {
           zephyr,console = &uart2;
           zephyr,shell-uart = &uart2;
       };
   };

On the Arduino UNO R4 Minima the console is on SCI2 (D0/D1); that board has no
ESP32-S3 and so no USB bridge.

Programming and debugging
*************************

.. zephyr:board-supported-runners::

Debug adapter
=============

A debug adapter is required to flash and debug programs.
Arduino UNO R4 WiFi has a built-in debug adapter that
you can use for flashing and debugging.

In the Arduino UNO R4 Minima case, You need to prepare
debug adapter separately. A 5V-compatible CMSIS-DAP adapter
adapts to this board.


Building & Flashing
===================

You can build and flash with ``west flash`` command (See
:ref:`build_an_application` and
:ref:`application_run` for more details).

Here is an example for building and flashing the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: arduino_uno_r4@minima
   :goals: build flash

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: arduino_uno_r4@wifi
   :goals: build flash

Debugging
=========

Debugging can be done with ``west debug`` command.
The following command is debugging the :zephyr:code-sample:`blinky` application.
Also, see the instructions specific to the debug server that you use.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: arduino_uno_r4@minima
   :maybe-skip-config:
   :goals: debug

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: arduino_uno_r4@wifi
   :maybe-skip-config:
   :goals: debug


Using pyOCD
-----------

Various debug adapters, including cmsis-dap probes, can debug the Arduino UNO R4 with pyOCD.
The default configuration uses the pyOCD for debugging.
You must install CMSIS-Pack when flashing or debugging Arduino UNO R4 Minima with pyOCD.
If not installed yet, execute the following command to install CMSIS-Pack for Arduino UNO R4.

.. code-block:: console

   pyocd pack install r7fa4m1ab


Restoring Arduino Bootloader
============================

If you corrupt the Arduino bootloader, you can restore it with the following command.

.. code-block:: console

   wget https://raw.githubusercontent.com/arduino/ArduinoCore-renesas/main/bootloaders/UNO_R4/dfu_minima.hex
   pyocd flash -e sector -a 0x0 -t r7fa4m1ab dfu_minima.hex

.. code-block:: console

   wget https://raw.githubusercontent.com/arduino/ArduinoCore-renesas/main/bootloaders/UNO_R4/dfu_wifi.hex
   pyocd flash -e sector -a 0x0 -t r7fa4m1ab dfu_wifi.hex
