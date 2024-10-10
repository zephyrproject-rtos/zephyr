.. _slwrb4321a:

WGM160P Wi-Fi Module (SLWRB4321A)
#################################

Overview
********

The WGM160P Starter Kit SLWSTK6121A comes with the BRD4321A radio board.
This radio boards contains a WGM160P module, which combines the WF200 Wi-Fi
transceiver with an EFM32GG11 microcontroller.

.. figure:: wgm160p-starter-kit.jpg
   :align: center
   :alt: SLWSTK6121A

   SLWSTK6121A (image courtesy of Silicon Labs)

Hardware
********

- Advanced Energy Monitoring provides real-time information about the energy
  consumption of an application or prototype design.
- Ultra low power 128x128 pixel color Memory-LCD
- 2 user buttons and 2 LEDs
- Si7021 Humidity and Temperature Sensor
- On-board Segger J-Link USB and Ethernet debugger
- 10/100Base-TX ethernet PHY and RJ-45 jack (on included expansion board)
- MicroSD card slot
- USB Micro-AB connector

For more information about the WGM160P and SLWSTK6121A board:

- `WGM160P Website`_
- `WGM160P Datasheet`_
- `SLWSTK6121A Website`_
- `SLWSTK6121A User Guide`_
- `EFM32GG11 Datasheet`_
- `EFM32GG11 Reference Manual`_
- `WF200 Datasheet`_

Supported Features
==================

The slwrb4321a board configuration supports the following hardware
features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| MPU       | on-chip    | memory protection unit              |
+-----------+------------+-------------------------------------+
| COUNTER   | on-chip    | rtcc                                |
+-----------+------------+-------------------------------------+
| ETHERNET  | on-chip    | ethernet                            |
+-----------+------------+-------------------------------------+
| FLASH     | on-chip    | flash memory                        |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | gpio                                |
+-----------+------------+-------------------------------------+
| I2C       | on-chip    | i2c port-polling                    |
+-----------+------------+-------------------------------------+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| SYSTICK   | on-chip    | systick                             |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port-polling;                |
|           |            | serial port-interrupt               |
+-----------+------------+-------------------------------------+

The default configuration can be found in
:zephyr_file:`boards/silabs/slwrb4321a/slwrb4321a_defconfig`

Other hardware features, including the WF200 WiFi transceiver, are
currently not supported by the port.

Connections and IOs
===================

The WGM160P's EFM32GG11 SoC has six GPIO controllers (PORTA to PORTF), all of which are
currently enabled for the SLWSTK6121A board.

In the following table, the column **Name** contains pin names. For example, PE1
means pin number 1 on PORTE, as used in the board's datasheets and manuals.

+-------+-------------+-------------------------------------+
| Name  | Function    | Usage                               |
+=======+=============+=====================================+
| PA4   | GPIO        | LED0                                |
+-------+-------------+-------------------------------------+
| PA5   | GPIO        | LED1                                |
+-------+-------------+-------------------------------------+
| PD6   | GPIO        | Push Button PB0                     |
+-------+-------------+-------------------------------------+
| PD8   | GPIO        | Push Button PB1                     |
+-------+-------------+-------------------------------------+
| PE7   | UART_TX     | UART TX Console VCOM_TX US0_TX #1   |
+-------+-------------+-------------------------------------+
| PE6   | UART_RX     | UART RX Console VCOM_RX US0_RX #1   |
+-------+-------------+-------------------------------------+
| PB11  | I2C_SDA     | SENSOR_I2C_SDA I2C1_SDA #1          |
+-------+-------------+-------------------------------------+
| PB12  | I2C_SCL     | SENSOR_I2C_SCL I2C1_SCL #1          |
+-------+-------------+-------------------------------------+


System Clock
============

The EFM32GG11 SoC is configured to use the 50 MHz external oscillator on the
board.

Serial Port
===========

The EFM32GG11 SoC has four USARTs, two UARTs and two Low Energy UARTs (LEUART).
USART0 is connected to the board controller and is used for the console.

Programming and Debugging
*************************

.. note::
   Before using the kit the first time, you should update the J-Link firmware
   from `J-Link-Downloads`_

Flashing
========

The SLWSTK6121A includes an `J-Link`_ serial and debug adaptor built into the
board. The adaptor provides:

- A USB connection to the host computer
- A physical UART connection which is relayed over interface USB serial port.

Flashing an application to SLWSTK6121A
--------------------------------------

Connect the SLWSTK6121A to your host computer using the USB port.

Here is an example to build and flash the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: slwrb4321a
   :goals: flash

Open a serial terminal (minicom, putty, etc.) with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Reset the board and you'll see the following message on the corresponding serial port
terminal session:

.. code-block:: console

   Hello World! slwrb4321a

.. _WGM160P Website:
   https://www.silabs.com/wireless/wi-fi/wfm160-series-1-modules

.. _WGM160P Datasheet:
   https://www.silabs.com/documents/public/data-sheets/wgm160p-datasheet.pdf

.. _SLWSTK6121A Website:
   https://www.silabs.com/development-tools/wireless/wi-fi/wgm160p-wifi-module-starter-kit

.. _SLWSTK6121A User Guide:
   https://www.silabs.com/documents/public/user-guides/ug351-brd4321a-user-guide.pdf

.. _EFM32GG11 Datasheet:
   https://www.silabs.com/documents/public/data-sheets/efm32gg11-datasheet.pdf

.. _EFM32GG11 Reference Manual:
   https://www.silabs.com/documents/public/reference-manuals/efm32gg11-rm.pdf

.. _WF200 Datasheet:
   https://www.silabs.com/documents/public/data-sheets/wf200-datasheet.pdf

.. _J-Link:
   https://www.segger.com/jlink-debug-probes.html

.. _J-Link-Downloads:
   https://www.segger.com/downloads/jlink
