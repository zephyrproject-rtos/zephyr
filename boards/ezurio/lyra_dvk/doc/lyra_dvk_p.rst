.. zephyr:board:: lyra_dvk_p

.. |Lyra variant| replace:: P
.. |Datasheet Ref| replace:: `Lyra P datasheet`_
.. |SoC| replace:: BGM220PC22HNA
.. |SoC Datasheet Ref| replace:: `BGM220P datasheet`_
.. |Number of IO| replace:: 24
.. |BLE TX Power| replace:: 8dBm
.. |Board Quoted| replace:: ``lyra_dvk_p``
.. |User Guide Ref| replace:: `Lyra P DVK user guide`_
.. |Schematics Ref| replace:: `Lyra P DVK schematics`_

.. include:: lyra_dvk_common_1.rst.inc

Hardware
********

The Lyra P DVK has two crystal oscillators as follows.

* High-frequency 38.4 MHz crystal oscillator (HFXO)
* Low-frequency 32.768 kHz crystal oscillator (LFXO)

Both crystal oscillators are fitted within the Lyra P module.

The module supports an on-chip Bluetooth Low Energy antenna.

Full details of the DVK can be found in the |User Guide Ref| and |Schematics Ref|.

.. include:: lyra_dvk_common_2.rst.inc

+-------+-------------+-------------------------------------+-----------+
| Name  | Function    | Usage                               | Direction |
+=======+=============+=====================================+===========+
| PA0   | USART1_TX   | UART Console VCOM_TX                | O         |
+-------+-------------+-------------------------------------+-----------+
| PA1   | SWD_SWCLK   | JLink SWCLK                         | I         |
+-------+-------------+-------------------------------------+-----------+
| PA2   | SWD_SWDIO   | JLink SWDIO                         | I/O       |
+-------+-------------+-------------------------------------+-----------+
| PA3   | SWD_SWO     | JLink SWO                           | O         |
+-------+-------------+-------------------------------------+-----------+
| PA4   | USART1_RTS  | UART Console VCOM_RTS               | O         |
+-------+-------------+-------------------------------------+-----------+
| PA5   | USART1_CTS  | UART Console VCOM_CTS               | I         |
+-------+-------------+-------------------------------------+-----------+
| PA6   | GPIO        | Breakout Connector GPIO             | I/O       |
+-------+-------------+-------------------------------------+-----------+
| PA7   | USART1_RX   | UART Console VCOM_RX                | I         |
+-------+-------------+-------------------------------------+-----------+
| PA8   | GPIO        | LED 0                               | O         |
+-------+-------------+-------------------------------------+-----------+
| PB0   | GPIO        | mikroBUS AN                         | I         |
+-------+-------------+-------------------------------------+-----------+
| PB1   | EUART0_TX   | mikroBUS TX                         | O         |
+-------+-------------+-------------------------------------+-----------+
| PB2   | EUART0_RX   | mikroBUS RX                         | I         |
+-------+-------------+-------------------------------------+-----------+
| PB3   | GPIO        | mikroBUS INT                        | I         |
+-------+-------------+-------------------------------------+-----------+
| PB4   | GPIO        | mikroBUS PWM                        | O         |
+-------+-------------+-------------------------------------+-----------+
| PC0   | PTI_FRAME   | Packet Trace Interface FRAME        | O         |
+-------+-------------+-------------------------------------+-----------+
| PC1   | PTI_DATA    | Packet Trace Interface DATA         | O         |
+-------+-------------+-------------------------------------+-----------+
| PC2   | USART0_SCK  | mikroBUS SCK                        | O         |
+-------+-------------+-------------------------------------+-----------+
| PC3   | GPIO        | mikroBUS CS                         | O         |
+-------+-------------+-------------------------------------+-----------+
| PC4   | USART0_TX   | mikroBUS MOSI                       | O         |
+-------+-------------+-------------------------------------+-----------+
| PC5   | USART0_RX   | mikroBUS MISO                       | I         |
+-------+-------------+-------------------------------------+-----------+
| PC6   | GPIO        | mikroBUS RST                        | O         |
+-------+-------------+-------------------------------------+-----------+
| PC7   | GPIO        | Button 0                            | I         |
+-------+-------------+-------------------------------------+-----------+
| PD2   | I2C0_SCL    | mikroBUS SCL / Qwiic SCL            | I/O       |
+-------+-------------+-------------------------------------+-----------+
| PD3   | I2C0_SDA    | mikroBUS SDA / Qwiic SDA            | I/O       |
+-------+-------------+-------------------------------------+-----------+

.. include:: lyra_dvk_common_3.rst.inc

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/observer
   :board: lyra_dvk_p
   :goals: build

.. include:: lyra_dvk_common_4.rst.inc

.. _Lyra P datasheet:
   https://www.ezurio.com/documentation/datasheet-lyra-p

.. _BGM220P datasheet:
   https://www.silabs.com/documents/public/data-sheets/bgm220p-datasheet.pdf

.. _Lyra P DVK user guide:
   https://www.ezurio.com/documentation/user-guide-lyra-p-development-kit

.. _Lyra P DVK schematics:
   https://www.ezurio.com/documentation/schematic-pcb-assembly-lyra-p-dev-board
