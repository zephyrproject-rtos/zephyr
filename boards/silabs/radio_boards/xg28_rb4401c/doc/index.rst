.. zephyr:board:: xg28_rb4401c

Overview
********

The EFR32ZG28 Radio Board is the radio board delivered with
`xG28-PK6025A Website`_. It contains a Wireless System-On-Chip from the
EFR32ZG28 family built on an ARM Cortex®-M33 processor with excellent low
power capabilities.

The BRD4401C a.k.a. xG28-RB4401C radio board plugs into the Wireless Pro Kit
Mainboard BRD4002A and is supported as one of :ref:`silabs_radio_boards`.

Hardware
********

- EFR32ZG28B322F1024IM68 SoC
- CPU core: ARM Cortex®-M33 with FPU
- Flash memory: 1024 kB
- RAM: 256 kB
- Transmit power: up to +20 dBm
- Operation frequency: 868-915 MHz
- Crystals for LFXO (32.768 kHz) and HFXO (39 MHz).
- Silicon Labs Si7021 relative humidity and temperature sensor
- Low-power 128x128 pixel Memory LCD
- Macronix ultra low power 8-Mbit SPI flash (MX25R8035F)

For more information about the EFR32ZG28 SoC and BRD4401C board, refer to these
documents:

- `EFR32ZG28 Website`_
- `EFR32ZG28 Datasheet`_
- `EFR32xG28 Reference Manual`_
- `XG28-PK6025A Website`_
- `BRD4401C User Guide`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

In the following table, the column **Name** contains Pin names. For example, PA2
means Pin number 2 on PORTA, as used in the board's datasheets and manuals.

+-------+--------------+-------------------------------------+
| Name  | Function     | Usage                               |
+=======+==============+=====================================+
| PA8   | UART_TX      | UART Console TX                     |
+-------+--------------+-------------------------------------+
| PA9   | UART_RX      | UART Console RX                     |
+-------+--------------+-------------------------------------+
| PB1   | GPIO         | Push Button 0                       |
+-------+--------------+-------------------------------------+
| PB2   | GPIO         | LED0                                |
+-------+--------------+-------------------------------------+
| PB3   | GPIO         | Push Button 1                       |
+-------+--------------+-------------------------------------+
| PC1   | US1_TX       | Display/Flash SPI MOSI              |
+-------+--------------+-------------------------------------+
| PC2   | US1_RX       | Serial Flash MISO                   |
+-------+--------------+-------------------------------------+
| PC3   | US1_CLK      | Serial Flash/Display SPI Clock      |
+-------+--------------+-------------------------------------+
| PC4   | US1_CS       | Serial Flash Chip Select            |
+-------+--------------+-------------------------------------+
| PC5   | I2C0_SCL     | Si7021 I2C Clock                    |
+-------+--------------+-------------------------------------+
| PC6   | GPIO         | External COM Inversion Signal       |
+-------+--------------+-------------------------------------+
| PC7   | I2C0_SDA     | Si7021 I2C Data                     |
+-------+--------------+-------------------------------------+
| PC8   | US1_CS       | Display Serial Chip Select          |
+-------+--------------+-------------------------------------+
| PC9   | GPIO         | Display Control Access              |
+-------+--------------+-------------------------------------+
| PC11  | GPIO         | Si7021 Enable                       |
+-------+--------------+-------------------------------------+
| PD3   | GPIO         | LED1                                |
+-------+--------------+-------------------------------------+

The default configuration can be found in
:zephyr_file:`boards/silabs/radio_boards/xg28_rb4401c/xg28_rb4401c_defconfig`

System Clock
============

The EFR32ZG28 SoC is configured to use the 39 MHz external oscillator on the
board.

Serial Port
===========

The EFR32ZG28 SoC has one USART and three EUSARTs.
USART0 is connected to the board controller and is used for the console.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Flashing
========

Connect the BRD4002A board with a mounted BRD4401C radio module to your host
computer using the USB port.

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: xg28_rb4401c
   :goals: flash

Open a serial terminal (minicom, putty, etc.) with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Reset the board and you should see the following message in the terminal:

.. code-block:: console

   Hello World! xg28_rb4401c/efr32zg28b322f1024im68


.. _xG28-PK6025A Website:
   https://www.silabs.com/development-tools/wireless/efr32xg28-pro-kit-20-dbm

.. _BRD4401C User Guide:
   https://www.silabs.com/documents/public/user-guides/ug535-xg28-20dbm-user-guide.pdf

.. _EFR32ZG28 Website:
   https://www.silabs.com/wireless/z-wave/efr32zg28-z-wave-800-socs

.. _EFR32ZG28 Datasheet:
   https://www.silabs.com/documents/public/data-sheets/efr32zg28-datasheet.pdf

.. _EFR32xG28 Reference Manual:
   https://www.silabs.com/documents/public/reference-manuals/efr32xg28-rm.pdf
