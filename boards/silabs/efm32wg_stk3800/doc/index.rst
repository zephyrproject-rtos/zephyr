.. _efm32wg_stk3800:

EFM32WG-STK3800
###############

Overview
********

The EFM32 Wonder Gecko Starter Kit EFM32WG-STK3800 contains a MCU from the
EFM32WG family built on ARM® Cortex®-M4F processor with excellent low
power capabilities.

.. figure:: efm32wg_stk3800.jpg
   :align: center
   :alt: EFM32WG-STK3800

   EFM32WG-STK3800 (image courtesy of Silicon Labs)


Hardware
********

- Advanced Energy Monitoring provides real-time information about the energy
  consumption of an application or prototype design.
- 32MByte parallel NAND Flash
- 160 segment Energy Micro LCD
- 2 user buttons, 2 LEDs and a touch slider
- Ambient Light Sensor and Inductive-capacitive metal sensor
- On-board Segger J-Link USB debugger

For more information about the EFM32WG SoC and EFM32WG-STK3800 board:

- `EFM32WG Website`_
- `EFM32WG Datasheet`_
- `EFM32WG Reference Manual`_
- `EFM32WG-STK3800 Website`_
- `EFM32WG-STK3800 User Guide`_
- `EFM32WG-STK3800 Schematics`_

Supported Features
==================

The efm32wg_stk3800 board configuration supports the following hardware features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| MPU       | on-chip    | memory protection unit              |
+-----------+------------+-------------------------------------+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| SYSTICK   | on-chip    | systick                             |
+-----------+------------+-------------------------------------+
| FLASH     | on-chip    | flash memory                        |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | gpio                                |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port-polling;                |
|           |            | serial port-interrupt               |
+-----------+------------+-------------------------------------+

The default configuration can be found in
:zephyr_file:`boards/silabs/efm32wg_stk3800/efm32wg_stk3800_defconfig`

Other hardware features are currently not supported by the port.

Connections and IOs
===================

The EFM32WG SoC has six gpio controllers (PORTA to PORTF), but only three are
currently enabled (PORTB, PORTE and PORTF) for the EFM32WG-STK3800 board.

In the following table, the column Name contains Pin names. For example, PE2
means Pin number 2 on PORTE, as used in the board's datasheets and manuals.

+-------+-------------+-------------------------------------+
| Name  | Function    | Usage                               |
+=======+=============+=====================================+
| PE2   | GPIO        | LED0                                |
+-------+-------------+-------------------------------------+
| PE3   | GPIO        | LED1                                |
+-------+-------------+-------------------------------------+
| PB9   | GPIO        | Push Button PB0                     |
+-------+-------------+-------------------------------------+
| PB10  | GPIO        | Push Button PB1                     |
+-------+-------------+-------------------------------------+
| PF7   | GPIO        | Board Controller Enable             |
|       |             | EFM_BC_EN                           |
+-------+-------------+-------------------------------------+
| PE0   | UART0_TX    | UART Console EFM_BC_TX U0_TX #1     |
+-------+-------------+-------------------------------------+
| PE1   | UART0_RX    | UART Console EFM_BC_RX U0_RX #1     |
+-------+-------------+-------------------------------------+

System Clock
============

The EFM32WG SoC is configured to use the 48 MHz external oscillator on the
board.

Serial Port
===========

The EFM32WG SoC has three USARTs, two UARTs and two Low Energy UARTs (LEUART).
UART0 is connected to the board controller and is used for the console.

Programming and Debugging
*************************

.. note::
   Before using the kit the first time, you should update the J-Link firmware
   from `J-Link-Downloads`_

Flashing
========

The EFM32WG-STK3800 includes an `J-Link`_ serial and debug adaptor built into the
board. The adaptor provides:

- A USB connection to the host computer, which exposes a Mass Storage and a
  USB Serial Port.
- A Serial Flash device, which implements the USB flash disk file storage.
- A physical UART connection which is relayed over interface USB Serial port.

Flashing an application to EFM32-STK3800
----------------------------------------

The sample application :ref:`hello_world` is used for this example.
Build the Zephyr kernel and application:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: efm32wg_stk3800
   :goals: build

Connect the EFM32WG-STK3800 to your host computer using the USB port and you
should see a USB connection which exposes a Mass Storage (STK3800) and a
USB Serial Port. Copy the generated zephyr.bin in the STK3800 drive.

Open a serial terminal (minicom, putty, etc.) with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Reset the board and you should be able to see on the corresponding Serial Port
the following message:

.. code-block:: console

   Hello World! arm


.. _EFM32WG-STK3800 Website:
   http://www.silabs.com/products/development-tools/mcu/32-bit/efm32-wonder-gecko-starter-kit

.. _EFM32WG-STK3800 User Guide:
   http://www.silabs.com/documents/public/user-guides/efm32wg-stk3800-ug.pdf

.. _EFM32WG-STK3800 Schematics:
   http://www.silabs.com/documents/public/schematic-files/BRD2400A_A00.pdf

.. _EFM32WG Website:
   http://www.silabs.com/products/mcu/32-bit/efm32-wonder-gecko

.. _EFM32WG Datasheet:
   http://www.silabs.com/documents/public/data-sheets/EFM32WG990.pdf

.. _EFM32WG Reference Manual:
   http://www.silabs.com/documents/public/reference-manuals/EFM32WG-RM.pdf

.. _J-Link:
   https://www.segger.com/jlink-debug-probes.html

.. _J-Link-Downloads:
   https://www.segger.com/downloads/jlink
