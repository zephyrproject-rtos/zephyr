.. zephyr:board:: arduino_uno_q

Overview
********

The Arduino UNO Q is a development board featuring a Qualcomm QRB2210
processor (Quad core ARM Cortex-A53) and an STM32U585 microcontroller.
The board is designed around the Arduino form factor and is compatible
with traditional Arduino shields and accessories.
This port targets the STM32U585 microcontroller on the board.

Hardware
********

- Qualcomm QRB2210 Processor (Quad core ARM Cortex-A53)
- STM32U585 Microcontroller (ARM Cortex-M33 at 160 MHz)
- 2 Mbyte of Flash memory and 786 Kbytes of RAM
- 2 RGB user LEDs
- One 13x8 LED Matrix
- Internal UART and SPI buses connected to the QRB2210
- Built-in CMSIS-DAP debug adapter (through QRB2210)

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and debugging
*************************

.. zephyr:board-supported-runners::

Debug adapter
=============

The QRB2210 microprocessor can act as an SWD debug adapter for the STM32U585.
This is supported by the ``openocd`` binary available in the board, and its
functionality can be made available to the computer via ``adb`` port forwarding
commands.

Flashing
========

The board configuration uses a UNO Q-specific OpenOCD bridge script which
starts the board-side debug server through ``adb`` and forwards the GDB port
automatically. From a USB-connected computer, build and flash the
:zephyr:code-sample:`blinky` application with:

.. code-block:: console

   west build -b arduino_uno_q samples/basic/blinky
   west flash -r openocd

Debugging
=========

Debugging can be done with the usual ``west debug`` command. The board-specific
OpenOCD bridge starts the debug server on the board over ``adb`` before
connecting GDB:

.. code-block:: console

   west build -b arduino_uno_q samples/basic/blinky
   west debug -r openocd

Restoring the Arduino sketch loader
===================================

The Arduino UNO Q comes with a pre-installed application that acts as a loader
for user sketches, and is shipped as part of the Arduino Zephyr cores. If you
overwrite this application, you can restore it later by issuing the following
command from an USB-connected computer:

.. code-block:: console

   adb shell arduino-cli burn-bootloader -b arduino:zephyr:unoq -P jlink

The same ``arduino-cli`` command can also be directly used on the board, when
in standalone mode:

.. code-block:: console

   arduino-cli burn-bootloader -b arduino:zephyr:unoq -P jlink
