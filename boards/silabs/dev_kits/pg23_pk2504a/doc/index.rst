.. zephyr:board:: pg23_pk2504a

Overview
********

The EFM32PG23 Pearl Gecko Board dev kit contains
a System-On-Chip from the EFM32PG23 family built on an
ARM Cortex®-M33 processor with excellent low power capabilities.

Hardware
********

- EFM32PG23B310F512IM48 Pearl Gecko SoC
- CPU core: ARM Cortex®-M33
- Flash memory: 512 kB
- RAM: 64 kB
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

For more information about the EFM32PG23 SoC and BRD2504A board, refer to these
documents:

- `EFM32PG23 Website`_
- `EFM32PG23 Datasheet`_
- `EFM32PG23 Reference Manual`_
- `BRD2504A User Guide`_

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
| PA5  | GPIO            | Push Button 0       |
+------+-----------------+---------------------+
| PA7  | I2C0_SDA        | Si7021 I2C Data     |
+------+-----------------+---------------------+
| PA8  | I2C0_SCL        | Si7021 I2C Clock    |
+------+-----------------+---------------------+
| PB4  | GPIO            | Push Button 1       |
+------+-----------------+---------------------+
| PB5  | EUSART0_TX      | Console Tx          |
+------+-----------------+---------------------+
| PB6  | EUSART0_RX      | Console Rx          |
+------+-----------------+---------------------+
| PC8  | GPIO/TIMER0_CC0 | LED0                |
+------+-----------------+---------------------+
| PC9  | GPIO/TIMER0_CC1 | LED1                |
+------+-----------------+---------------------+

The default configuration can be found in
:zephyr_file:`boards/silabs/dev_kits/pg23_pk2504a/pg23_pk2504a_defconfig`

System Clock
============

The EFM32PG23 SoC is configured to use the 39 MHz external oscillator on the
board.

Serial Port
===========

The EFM32PG23 SoC has one USART and three EUSARTs.
EUSART0 is connected to the board controller and is used for the console.

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
   :board: pg23_pk2504a
   :goals: build

Connect the pg23_pk2504a to your host computer using the USB port and you
should see a USB connection.

Open a serial terminal (minicom, putty, etc.) with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Reset the board and you'll see the following message on the corresponding serial port
terminal session:

.. code-block:: console

   Hello World! pg23_pk2504a

Troubleshooting
===============
If no serial output occurs, use SEGGERs RTT Viewer and update :zephyr_file:`boards/silabs/dev_kits/pg23_pk2504a/pg23_pk2504a_defconfig` with

.. code-block:: console

   CONFIG_UART_CONSOLE=n
   CONFIG_RTT_CONSOLE=y
   CONFIG_USE_SEGGER_RTT=y

.. _EFM32PG23 Website:
   https://www.silabs.com/mcu/32-bit-microcontrollers/efm32pg23-series-2#

.. _EFM32PG23 Datasheet:
   https://www.silabs.com/documents/public/data-sheets/efm32pg23-datasheet.pdf

.. _EFM32PG23 Reference Manual:
   https://www.silabs.com/documents/public/reference-manuals/efm32pg23-rm.pdf

.. _BRD2504A User Guide:
   https://www.silabs.com/documents/public/user-guides/ug515-efm32pg23-brd2504a-user-guide.pdf
