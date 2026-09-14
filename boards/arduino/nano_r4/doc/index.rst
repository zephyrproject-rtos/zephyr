.. zephyr:board:: arduino_nano_r4

Overview
********

The Arduino NANO R4 is a development board featuring the Renesas RA4M1 SoC
in the Arduino Nano form factor and is compatible with traditional Arduino Nano.

Hardware
********

- Renesas RA4MA1 Processor (ARM Cortex-M4 at 48 MHz)
- 256 KiB flash memory and 32 KiB of RAM
- One user LEDs
- One set of PWM RGB LEDs
- One reset button

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and debugging
*************************

.. zephyr:board-supported-runners::

Debug adapter
=============

A debug adapter is required to flash and debug programs.

You need to prepare debug adapter separately.
A 5V-compatible CMSIS-DAP adapter adapts to this board.

Alternatively you can use [Renesas Flash Programming Tool](https://www.renesas.com/en/software-tool/renesas-flash-programmer-programming-gui#overview).
Although it is a propriatory software and requiers creating a
user account to download it. This tool however, lets you flash
over USB Type-C.


Building & Flashing
===================

You can build and flash with ``west flash`` command (See
:ref:`build_an_application` and
:ref:`application_run` for more details).

Here is an example for building and flashing the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: arduino_nano_r4
   :goals: build flash

To flash over USB Type-C using the Renesas Flash Programmer:

First, you need to put your Nano R4 board into bootloader programming mode.
Locate the ``BOOT`` and ``GND`` pins on your Nano R4 board.
These are clearly labeled on the bottom side of the board.
Use a jumper wire or tweezers to create a connection between the ``BOOT`` and ``GND`` pins.
This connection must remain in place during the initial connection to your computer.

While maintaining the ``BOOT-GND`` connection, connect your Nano R4 board to your computer using
the USB-C cable. Once connected, press the RESET button on the board once. The onboard orange
LED should turn OFF, and only the green Power LED should remain ON.

Then you can flash using the cli:

.. code-block:: console

   ./rfp-cli -device RA --port /dev/ttyACM0 -p build/zephyr/zephyr.hex


Make sure not to use the ``-e`` or ``-a`` options which will erase the arduino bootloader.

Debugging
=========

Debugging can be done with ``west debug`` command.
The following command is debugging the :zephyr:code-sample:`blinky` application.
Also, see the instructions specific to the debug server that you use.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: arduino_nano_r4
   :maybe-skip-config:
   :goals: debug

Using pyOCD
-----------

Various debug adapters, including cmsis-dap probes, can debug the Arduino Nano R4 with pyOCD.
The default configuration uses the pyOCD for debugging.
You must install CMSIS-Pack when flashing or debugging Arduino Nano R4 with pyOCD.
If not installed yet, execute the following command to install CMSIS-Pack for Arduino Nano R4.

.. code-block:: console

   pyocd pack install r7fa4m1ab


Restoring Arduino Bootloader
============================

If you corrupt the Arduino bootloader, you can restore it with the following command.

.. code-block:: console

   wget https://raw.githubusercontent.com/arduino/ArduinoCore-renesas/main/bootloaders/NANOR4/dfu_nano.hex
   pyocd flash -e sector -a 0x0 -t r7fa4m1ab dfu_nano.hex


Or to recover over USB Type-C using the Renesas Flash Programmer, follow the official [Arduino Guide](https://docs.arduino.cc/tutorials/nano-r4/bootloader-flashing/).
