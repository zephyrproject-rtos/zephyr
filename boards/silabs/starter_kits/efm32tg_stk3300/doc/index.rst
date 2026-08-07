.. zephyr:board:: efm32tg_stk3300

Overview
********

The EFM32 Tiny Gecko Starter Kit EFM32TG-STK3300 contains a MCU from the
EFM32TG family built on ARM® Cortex®-M3 processor with excellent low
power capabilities.

Hardware
********

- EFM32TG840F32 MCU with 32 kB flash and 4 kB RAM
- Advanced Energy Monitoring provides real-time information about the energy
  consumption of an application or prototype design.
- 160 segment Energy Micro LCD
- 2 user buttons, 1 LED and a touch slider
- Ambient Light Sensor, Inductive-capacitive metal sensor and touch sensor
- On-board Segger J-Link USB debugger

For more information about the EFM32TG SoC and EFM32TG-STK3300 board:

- `EFM32TG Website`_
- `EFM32TG Datasheet`_
- `EFM32TG Reference Manual`_
- `EFM32TG-STK3300 Website`_
- `EFM32TG-STK3300 User Guide`_
- `EFM32TG-STK3300 Schematics`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The EFM32TG SoC has six gpio controllers (PORTA to PORTF), but only three are
currently enabled (PORTB, PORTC and PORTD) for the EFM32TG-STK3300 board.

In the following table, the column Name contains Pin names. For example, PE2
means Pin number 2 on PORTE, as used in the board's datasheets and manuals.

+------+-----------+---------------------------------------+
| Name | Function  | Usage                                 |
+======+===========+=======================================+
| PD7  | GPIO      | LED0                                  |
+------+-----------+---------------------------------------+
| PD8  | GPIO      | Push Button PB0                       |
+------+-----------+---------------------------------------+
| PB11 | GPIO      | Push Button PB1                       |
+------+-----------+---------------------------------------+
| PD0  | USART1_TX | UART Console USART1_TX #1 (EXP Pin 4) |
+------+-----------+---------------------------------------+
| PD1  | USART1_RX | UART Console USART1_RX #1 (EXP Pin 6) |
+------+-----------+---------------------------------------+

System Clock
============

The EFM32TG SoC is configured to use the 32 MHz external oscillator on the
board.

Serial Port
===========

The EFM32TG SoC has two USARTs and one Low Energy UART (LEUART).
USART1 is used for the console. It is exposed to the EXP Header on the
board (TX: Pin 4, RX: Pin 6).

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

.. note::
   Before using the kit the first time, you should update the J-Link firmware
   in Simplicity Studio or with *JLinkConfig*.

Flashing
========

The EFM32TG-STK3300 includes an `J-Link`_ debug adaptor built into the
board. It is used to flash and debug the EFM32TG on the board.

Flashing an application to EFM32-STK3300
----------------------------------------

The sample application :zephyr:code-sample:`hello_world` is used for this example.
Build the Zephyr kernel and application:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: efm32tg_stk3300
   :goals: build

Connect the EFM32TG-STK3300 to your host computer using the USB port.
Flash the device:

.. code-block:: console

   west flash

Connect the board with
Use a Serial-to-USB cable to connect the host computer with the board.
The serial port is exposed on the EXP Header (TX: Pin 4, RX: Pin 6).

Open a serial terminal (minicom, putty, etc.) with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Reset the board and you should be able to see on the corresponding Serial Port
the following message:

.. code-block:: console

   Hello World! efm32tg_stk3300/efm32tg840f32


.. _EFM32TG-STK3300 Website:
   https://www.silabs.com/products/development-tools/mcu/32-bit/efm32-tiny-gecko-starter-kit

.. _EFM32TG-STK3300 User Guide:
   https://www.silabs.com/documents/public/user-guides/ug420-efm32tg-stk3300.pdf

.. _EFM32TG-STK3300 Schematics:
   https://www.silabs.com/documents/public/schematic-files/BRD2100A-A04-schematic.pdf

.. _EFM32TG Website:
   https://www.silabs.com/products/mcu/32-bit/efm32-tiny-gecko

.. _EFM32TG Datasheet:
   https://www.silabs.com/documents/public/data-sheets/EFM32TG840.pdf

.. _EFM32TG Reference Manual:
   https://www.silabs.com/documents/public/reference-manuals/EFM32TG-RM.pdf

.. _J-Link:
   https://www.segger.com/jlink-debug-probes.html
