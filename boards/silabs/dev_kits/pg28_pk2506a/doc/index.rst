.. zephyr:board:: pg28_pk2506a

Overview
********

The EFM32PG28 Pearl Gecko Board dev kit contains
a System-On-Chip from the EFM32PG28 family built on an
ARM Cortex®-M33 processor with excellent low power capabilities.

Hardware
********

- EFM32PG28B310F1024IM68 Pearl Gecko SoC
- CPU core: ARM Cortex®-M33
- Flash memory: 1024 kB
- RAM: 256 kB
- Key features:
  - USB connectivity
  - Advanced Energy Monitor (AEM)
  - SEGGER J-Link on-board debugger
  - Debug multiplexer supporting external hardware as well as on-board MCU
  - 4x10 segment LCD
  - User LEDs and push buttons
  - Silicon Labs' Si7021 Relative Humidity and Temperature Sensor
  - SMA connector for IADC demonstration
  - Inductive LC sensor
  - 20-pin 2.54 mm header for expansion boards
  - Breakout pads for direct access to I/O pins
  - Power sources include USB and CR2032 coin cell battery

For more information about the EFM32PG28 SoC and BRD2506A board, refer to these
documents:

- `EFM32PG28 Website`_
- `EFM32PG28 Datasheet`_
- `EFM32PG28 Reference Manual`_
- `BRD2506A User Guide`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

In the following table, the column **Name** contains Pin names. For example, PA2
means Pin number 2 on Port A, as used in the board's datasheets and manuals.

+------+-----------------+---------------------+
| Name | Function        | Usage               |
+======+=================+=====================+
| PB1  | GPIO            | Push Button 0       |
+------+-----------------+---------------------+
| PB6  | GPIO            | Push Button 1       |
+------+-----------------+---------------------+
| PC10 | GPIO/TIMER0_CC0 | LED0                |
+------+-----------------+---------------------+
| PC11 | GPIO/TIMER0_CC1 | LED1                |
+------+-----------------+---------------------+
| PD7  | EUSART1_TX      | Console Tx          |
+------+-----------------+---------------------+
| PD8  | EUSART1_RX      | Console Rx          |
+------+-----------------+---------------------+
| PD9  | I2C0_SDA        | Si7021 I2C Data     |
+------+-----------------+---------------------+
| PD10 | I2C0_SCL        | Si7021 I2C Clock    |
+------+-----------------+---------------------+

The default configuration can be found in
:zephyr_file:`boards/silabs/dev_kits/pg28_pk2506a/pg28_pk2506a_defconfig`

System Clock
============

The EFM32PG28 SoC is configured to use the 39 MHz external oscillator on the
board.

Serial Port
===========

The EFM32PG28 SoC has one USART and three EUSARTs.
EUSART1 is connected to the board controller and is used for the console.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

.. note::
   Before using the kit the first time, you should update the J-Link firmware
   in Simplicity Studio.

Flashing
========

The sample application :zephyr:code-sample:`hello_world` is used for this example.
Build the Zephyr kernel and application:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: pg28_pk2506a
   :goals: build

Connect the pg28_pk2506a to your host computer using the USB port and you
should see a USB connection.

Open a serial terminal (minicom, putty, etc.) with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Reset the board and you'll see the following message on the corresponding serial port
terminal session:

.. code-block:: console

   Hello World! pg28_pk2506a

Troubleshooting
===============
If no serial output occurs, use SEGGERs RTT Viewer and update :zephyr_file:`boards/silabs/dev_kits/pg28_pk2506a/pg28_pk2506a_defconfig` with

.. code-block:: console

   CONFIG_UART_CONSOLE=n
   CONFIG_RTT_CONSOLE=y
   CONFIG_USE_SEGGER_RTT=y

.. _EFM32PG28 Website:
   https://www.silabs.com/mcu/32-bit-microcontrollers/efm32pg28-series-2#

.. _EFM32PG28 Datasheet:
   https://www.silabs.com/documents/public/data-sheets/efm32pg28-datasheet.pdf

.. _EFM32PG28 Reference Manual:
   https://www.silabs.com/documents/public/reference-manuals/efm32pg28-rm.pdf

.. _BRD2506A User Guide:
   https://www.silabs.com/documents/public/user-guides/ug545-efm32pg28-brd2506a-user-guide.pdf
