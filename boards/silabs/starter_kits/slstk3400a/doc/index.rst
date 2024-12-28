.. zephyr:board:: slstk3400a

Overview
********

The EFM32 Happy Gecko Starter Kit SLSTK3400A contains a MCU from the
EFM32HG family built on ARM® Cortex®-M0+ processor with excellent low
power capabilities.

Hardware
********

- Advanced Energy Monitoring system for precise current tracking
- Real-time energy and power profiling
- ARM Cortex M0+ with 64 kB Flash and 8 kB RAM
- 128 X 128 pixel Memory LCD
- 2 user buttons, 2 user LEDs and 2 touch buttons
- 20 pin expansion header
- Silicon Labs Si7021 Relative Humidity/Temperature sensor
- USB device interface
- Integrated SEGGER J-Link USB debugger/emulator with debug out functionality


See these documents for more information

- `EFM32HG Website`_
- `EFM32HG Datasheet`_
- `EFM32HG Reference Manual`_
- `SLSTK3400A Website`_
- `SLSTK3400A User Guide`_
- `SLSTK3400A Schematics`_

Supported Features
==================

The efm32hg_slstk3400 board configuration supports the following hardware features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| SYSTICK   | on-chip    | systick                             |
+-----------+------------+-------------------------------------+
| FLASH     | on-chip    | flash memory                        |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | gpio                                |
+-----------+------------+-------------------------------------+
| USART     | on-chip    | serial port-polling;                |
|           |            | serial port-interrupt               |
+-----------+------------+-------------------------------------+

The default configuration can be found in
:zephyr_file:`boards/silabs/starter_kits/slstk3400a/slstk3400a_defconfig`

Other hardware features are currently not supported by the port.

Connections and IOs
===================

The EFM32HG SoC has six GPIO controllers (PORTA to PORTF), but only three are
currently enabled (PORTB, PORTE and PORTF) for the SLSTK3400A board.

In the following table, the column Name contains Pin names. For example, PF4
means Pin number 4 on PORTF, as used in the board's datasheets and manuals.

+-------+-------------+-------------------------------------+
| Name  | Function    | Usage                               |
+=======+=============+=====================================+
| PF4   | GPIO        | LED0                                |
+-------+-------------+-------------------------------------+
| PF5   | GPIO        | LED1                                |
+-------+-------------+-------------------------------------+
| PC9   | GPIO        | Push Button PB0                     |
+-------+-------------+-------------------------------------+
| PC10  | GPIO        | Push Button PB1                     |
+-------+-------------+-------------------------------------+
| PF7   | GPIO        | Board Controller Enable             |
|       |             | EFM_BC_EN                           |
+-------+-------------+-------------------------------------+
| PF2   | USART0_TX   | USART Console EFM_BC_TX U0_TX #4    |
+-------+-------------+-------------------------------------+
| PA9   | USART0_RX   | USART Console EFM_BC_RX U0_RX #4    |
+-------+-------------+-------------------------------------+

System Clock
============

The EFM32HG SoC is configured to use the 24 MHz external oscillator on the
board.

Serial Port
===========

The EFM32HG SoC has two USARTs, two UARTs and two Low Energy UARTs (LEUART).
USART1 is connected to the board controller and is used for the console.

Programming and Debugging
*************************

.. note::
   Before using the kit the first time, you should update the J-Link firmware
   in Simplicity Studio.

Flashing
========

The SLSTK3400 includes an `J-Link`_ serial and debug adaptor built into the
board. The adaptor provides:

- A USB connection to the host computer, which exposes a Mass Storage and a
  USB Serial Port.
- A Serial Flash device, which implements the USB flash disk file storage.
- A physical UART connection which is relayed over interface USB Serial port.

Flashing an application to EFM32-SLSTK3400A
-------------------------------------------

The sample application :zephyr:code-sample:`hello_world` is used for this example.
Build the Zephyr kernel and application:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: slstk3400a
   :goals: build

Connect the SLSTK3400A to your host computer using the USB port and
you should see a USB connection that exposes a mass storage device (STK3400)
and a USB Serial Port. Copy the generated ``zephyr.bin`` in the STK3400 drive.

Open a serial terminal (minicom, putty, etc.) with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Reset the board and you will see this message written to the serial port:

.. code-block:: console

   Hello World! slstk3400a


.. _SLSTK3400A Website:
   https://www.silabs.com/products/development-tools/mcu/32-bit/efm32-happy-gecko-starter-kit

.. _SLSTK3400A User Guide:
   https://www.silabs.com/documents/public/user-guides/ug255-stk3400-user-guide.pdf

.. _SLSTK3400A Schematics:
   https://www.silabs.com/documents/public/schematic-files/BRD2012A-B01-schematic.pdf

.. _EFM32HG Website:
   https://www.silabs.com/products/mcu/32-bit/efm32-happy-gecko

.. _EFM32HG Datasheet:
   https://www.silabs.com/documents/public/data-sheets/EFM32HG322.pdf

.. _EFM32HG Reference Manual:
   https://www.silabs.com/documents/public/reference-manuals/EFM32HG-RM.pdf

.. _J-Link:
   https://www.segger.com/jlink-debug-probes.html
