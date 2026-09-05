.. zephyr:board:: aries_v3

Overview
********

The C-DAC ARIES v3.0 is a development board built around the `THEJAS32`_
System-on-Chip from the Centre for Development of Advanced Computing (C-DAC).
THEJAS32 is based on the VEGA ET1031, a 32-bit (RV32IM) single-core, in-order,
3-stage pipeline RISC-V processor developed under India's Digital India RISC-V
(DIR-V) programme.

Hardware
********

- VEGA ET1031 (RV32IM) core running at up to 100 MHz
- 256 KB internal SRAM (250 KB available to applications, the boot ROM
  reserves the top 6 KB)
- 2 MB external SPI flash
- 3x UART, 4x SPI, 3x I2C, 8x PWM, 32x GPIO, ADC
- On-board RGB LED
- JTAG debug interface

Supported Features
==================

.. zephyr:board-supported-hw::

SPI, I2C, PWM and ADC are present on the SoC but not yet supported in Zephyr.

Connections and IOs
===================

The Zephyr console runs on ``uart0`` at 115200 baud, which is wired to the
board's USB-C connector through a USB-UART bridge.

The three dies of the on-board RGB LED are driven, active low, by GPIO22
(green, ``led0``), GPIO23 (blue, ``led1``) and GPIO24 (red, ``led2``).

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Flashing
========

The board is flashed over the same USB-C serial port used for the console:
the boot ROM receives a raw binary over UART using the XMODEM protocol and
executes it from SRAM. ``west flash`` uses the open-source `vegadude`_
utility, which must be available on the :envvar:`PATH`.

To put the board in UART boot mode, leave the BOOT SEL jumper open and press
the reset button, then build and flash an application. Here is an example for
:zephyr:code-sample:`hello_world`.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: aries_v3
   :goals: build flash

Then attach a serial terminal to see the console output:

.. code-block:: console

   $ picocom /dev/ttyUSB0 -b 115200

C-DAC's ``vegaxmodem`` (UART boot) and ``vegaflasher`` (SPI flash boot) tools
can also be used to upload :file:`zephyr.bin` manually.

References
**********

.. target-notes::

.. _`THEJAS32`: https://vegaprocessors.in/thejas32-soc.php

.. _`vegadude`: https://github.com/rnayabed/vegadude
