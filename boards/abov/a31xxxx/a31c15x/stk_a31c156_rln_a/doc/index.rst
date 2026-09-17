.. zephyr:board:: stk_a31c156_rln_a

Overview
********

The STK-A31C156-RLN-A is a starter kit from ABOV Semiconductor built around
the A31C156 microcontroller, part of the ABOV32 A31C15x series. The A31C156
is an Arm® Cortex®-M0+ MCU with 256 KB of flash and 32 KB of RAM,
clocked from a 48 MHz internal HSI oscillator.

Key features:

- ABOV32 A31C156 (256 KB flash, 32 KB RAM, Cortex-M0+ @ 48 MHz)
- Six user LEDs
- Two user push-buttons
- USART10 used as the console UART

Supported Features
*******************

The following hardware features are currently supported:

- GPIO
- USART (console)

.. zephyr:board-supported-hw::

Connections and IOs
********************

Default board configuration:

- USART10 TX/RX : PB0/PB1 (console, 38400 8N1)
- LED0-LED5      : PE0-PE5 (active low)
- Button 0       : PF6 (active low, internal pull-up)
- Button 1       : PF7 (active low, internal pull-up)

Programming and Debugging
**************************

Zephyr does not yet have a flash/debug runner configured for this board
(see ``board.cmake``). Build the application as usual:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: stk_a31c156_rln_a
   :goals: build

and program the resulting ``build/zephyr/zephyr.hex`` onto the board with an
external SWD programmer.

Connect a serial terminal to the console UART at 38400 baud to see output.
