.. zephyr:board:: slstk3701a

Overview
********

The EFM32 Giant Gecko Starter Kit SLSTK3701A contains an MCU from the
EFM32GG Series 1 family built on an ARM® Cortex®-M4F processor with excellent
low power capabilities.

Hardware
********

- Advanced Energy Monitoring provides real-time information about the energy
  consumption of an application or prototype design.
- Ultra low power 128x128 pixel color Memory-LCD
- 2 user buttons, 2 LEDs and a touch slider
- Relative humidity, magnetic Hall Effect and inductive-capacitive metal sensor
- USB interface for Host/Device/OTG
- 32 Mb Quad-SPI Flash memory
- SD card slot
- RJ-45 Ethernet jack
- 2 digital microphones
- On-board Segger J-Link USB debugger

For more information about the EFM32GG11 SoC and SLSTK3701A board:

- `EFM32GG Series 1 Website`_
- `EFM32GG11 Datasheet`_
- `EFM32GG11 Reference Manual`_
- `SLSTK3701A Website`_
- `SLSTK3701A User Guide`_
- `SLSTK3701A Schematics`_

Supported Features
==================

The slstk3701a board configuration supports the following hardware
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
:zephyr_file:`boards/silabs/starter_kits/slstk3701a/slstk3701a_defconfig`

Other hardware features are currently not supported by the port.

Connections and IOs
===================

The EFM32GG11 SoC has nine GPIO controllers (PORTA to PORTI), all of which are
currently enabled for the SLSTK3701A board.

In the following table, the column **Name** contains pin names. For example, PE1
means pin number 1 on PORTE, as used in the board's datasheets and manuals.

+-------+-------------+-------------------------------------+
| Name  | Function    | Usage                               |
+=======+=============+=====================================+
| PH10  | GPIO        | LED0 red                            |
+-------+-------------+-------------------------------------+
| PH11  | GPIO        | LED0 green                          |
+-------+-------------+-------------------------------------+
| PH12  | GPIO        | LED0 blue                           |
+-------+-------------+-------------------------------------+
| PH13  | GPIO        | LED1 red                            |
+-------+-------------+-------------------------------------+
| PH14  | GPIO        | LED1 green                          |
+-------+-------------+-------------------------------------+
| PH15  | GPIO        | LED1 blue                           |
+-------+-------------+-------------------------------------+
| PC8   | GPIO        | Push Button PB0                     |
+-------+-------------+-------------------------------------+
| PC9   | GPIO        | Push Button PB1                     |
+-------+-------------+-------------------------------------+
| PE1   | GPIO        | Board Controller Enable             |
|       |             | EFM_BC_EN                           |
+-------+-------------+-------------------------------------+
| PH4   | UART_TX     | UART TX Console VCOM_TX US0_TX #4   |
+-------+-------------+-------------------------------------+
| PH5   | UART_RX     | UART RX Console VCOM_RX US0_RX #4   |
+-------+-------------+-------------------------------------+
| PI4   | I2C_SDA     | SENSOR_I2C_SDA I2C2_SDA #7          |
+-------+-------------+-------------------------------------+
| PI5   | I2C_SCL     | SENSOR_I2C_SCL I2C2_SCL #7          |
+-------+-------------+-------------------------------------+


System Clock
============

The EFM32GG11 SoC is configured to use the 50 MHz external oscillator on the
board.

Serial Port
===========

The EFM32GG11 SoC has six USARTs, two UARTs and two Low Energy UARTs (LEUART).
USART4 is connected to the board controller and is used for the console.

Programming and Debugging
*************************

.. note::
   Before using the kit the first time, you should update the J-Link firmware
   in Simplicity Studio.

Flashing
========

The SLSTK3701A includes an `J-Link`_ serial and debug adaptor built into the
board. The adaptor provides:

- A USB connection to the host computer, which exposes a mass storage device and a
  USB serial port.
- A serial flash device, which implements the USB flash disk file storage.
- A physical UART connection which is relayed over interface USB serial port.

Flashing an application to SLSTK3701A
-------------------------------------

The sample application :zephyr:code-sample:`hello_world` is used for this example.
Build the Zephyr kernel and application:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: slstk3701a
   :goals: build

Connect the SLSTK3701A to your host computer using the USB port and you
should see a USB connection which exposes a mass storage device(STK3701A) and
a USB Serial Port. Copy the generated zephyr.bin to the STK3701A drive.

Open a serial terminal (minicom, putty, etc.) with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Reset the board and you'll see the following message on the corresponding serial port
terminal session:

.. code-block:: console

   Hello World! slstk3701a


.. _SLSTK3701A Website:
   https://www.silabs.com/products/development-tools/mcu/32-bit/efm32-giant-gecko-gg11-starter-kit

.. _SLSTK3701A User Guide:
   https://www.silabs.com/documents/public/user-guides/ug287-stk3701.pdf

.. _SLSTK3701A Schematics:
   https://www.silabs.com/documents/public/schematic-files/BRD2204A-B00-schematic.pdf

.. _EFM32GG Series 1 Website:
   https://www.silabs.com/products/mcu/32-bit/efm32-giant-gecko-s1

.. _EFM32GG11 Datasheet:
   https://www.silabs.com/documents/public/data-sheets/efm32gg11-datasheet.pdf

.. _EFM32GG11 Reference Manual:
   https://www.silabs.com/documents/public/reference-manuals/efm32gg11-rm.pdf

.. _J-Link:
   https://www.segger.com/jlink-debug-probes.html
