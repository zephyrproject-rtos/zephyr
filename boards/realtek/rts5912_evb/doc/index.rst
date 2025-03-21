.. zephyr:board:: rts5912_evb

Overview
********

The RTS5912 EVB is a development platform to evaluate the Realtek RTS5912 embedded controller.

Hardware
********

- Realtek-M300 Processor (compatible to Cortex-M33)
- Memory:

   - 384 KB SRAM
   - 64 KB ROM
   - 512 KB Flash(MCM)
   - 256 B Battery SRAM
- PECI interface 3.1
- FAN, PWM and TACHO pins
- 6x I2C instances
- eSPI header
- 1x PS/2 ports
- Keyboard interface headers

For more information about the evb board please see `RTS5912_EVB_Schematics`_ and `RTS5912_DATASHEET`_

The board is powered through the +5V USB Type-C connector or adaptor.

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

Building
========

#. Build :zephyr:code-sample:`hello_world` application as you would normally do.

#. The file ``zephyr.rts5912.bin`` will be created if the build system can build successfully.
   This binary image can be found under file "build/zephyr/".

Flashing
========

#. Connect Dediprog into header ``J81`` and ``J82``.
#. Use Dediprog SF600 programmer to write the binary into the external flash ``U10`` at the address 0x0.
#. Power off the board.
#. Set the strap pin ``GPIO108`` to high and power on the board.

Debugging
=========

Using SWD or JTAG with ULINPRO.

References
**********

.. target-notes::

.. _RTS5912_EVB_Schematics:
    https://github.com/JasonLin-RealTek/Realtek_EC/blob/main/RTS5912_EVB_Schematic_Ver%201.1_20240701_1407.pdf

.. _RTS5912_DATASHEET:
   https://github.com/JasonLin-RealTek/Realtek_EC/blob/main/RTS5912_datasheet_brief.pdf
