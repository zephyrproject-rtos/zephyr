.. zephyr:code-sample:: tlc59731
   :name: TLC59731 RGB LED controller
   :relevant-api: led_interface

   Control 1 RGB LED connected to an TLC59731 driver chip.

Overview
********

This sample controls 1 LED connected to a TI TLC59731 driver on the
STM32WB5MM_DK board, using the following pattern:

 1. turn on/off the each single RGB LED for 1000ms
 2. set brightness of the whole RGB LED
 3. set different RGB colors for the LED

Refer to the `TLC59731 Manual`_ for the RGB LED connections and color channel
mappings used by this sample.

Building and Running
********************

Build the application for the `stm32wb5mm_dk` board with the following
commands, and make sure that `JP5` jumper (RGB LED chip-select)is connected
on the board.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/led_tlc59731
   :board: stm32wb5mm_dk
   :goals: build
   :compact:

For flashing the application, refer to the Flashing section of the
 `stm32wb5mm_dk` board documentation.

.. _TLC59731 Manual: https://www.ti.com/lit/ds/symlink/tlc59731.pdf
