.. zephyr:board:: lyra_24_dvk_s10

.. |Lyra 24 variant| replace:: S10
.. |Datasheet Ref| replace:: `Lyra 24 S datasheet`_
.. |SoC| replace:: BGM240SB22VNA
.. |SoC Datasheet Ref| replace:: `BGM240S datasheet`_
.. |Antenna Type| replace:: on-chip or external
.. |Number of IO| replace:: 31
.. |BLE TX Power| replace:: 10dBm
.. |Board Quoted| replace:: ``lyra_24_dvk_s10``
.. |User Guide Ref| replace:: `Lyra 24 S DVK user guide`_
.. |Schematics Ref| replace:: `Lyra 24 S10 DVK schematics`_

.. include:: lyra_24_dvk_common_1.rst.inc

+-------+-------------+-------------------------------------+-----------+
| Name  | Function    | Usage                               | Direction |
+=======+=============+=====================================+===========+
| PA0   | USART0_TX   | UART Console VCOM_TX                | O         |
+-------+-------------+-------------------------------------+-----------+
| PA1   | SWD_SWCLK   | JLink SWCLK                         | I         |
+-------+-------------+-------------------------------------+-----------+
| PA2   | SWD_SWDIO   | JLink SWDIO                         | I/O       |
+-------+-------------+-------------------------------------+-----------+
| PA3   | SWD_SWO     | JLink SWO                           | O         |
+-------+-------------+-------------------------------------+-----------+
| PA4   | USART0_RTS  | UART Console VCOM_RTS               | O         |
+-------+-------------+-------------------------------------+-----------+
| PA5   | USART0_CTS  | UART Console VCOM_CTS               | I         |
+-------+-------------+-------------------------------------+-----------+
| PA6   | GPIO        | Breakout Connector GPIO             | I/O       |
+-------+-------------+-------------------------------------+-----------+
| PA7   | USART0_RX   | UART Console VCOM_RX                | I         |
+-------+-------------+-------------------------------------+-----------+
| PA8   | GPIO        | LED 0                               | O         |
+-------+-------------+-------------------------------------+-----------+
| PB0   | GPIO        | mikroBUS AN                         | I         |
+-------+-------------+-------------------------------------+-----------+
| PB1   | EUSART0_TX  | mikroBUS TX                         | O         |
+-------+-------------+-------------------------------------+-----------+
| PB2   | EUSART0_RX  | mikroBUS RX                         | I         |
+-------+-------------+-------------------------------------+-----------+
| PB3   | GPIO        | mikroBUS INT                        | I         |
+-------+-------------+-------------------------------------+-----------+
| PB4   | GPIO        | mikroBUS PWM                        | O         |
+-------+-------------+-------------------------------------+-----------+
| PB5   | GPIO        | Test Point GPIO                     | I/O       |
+-------+-------------+-------------------------------------+-----------+
| PC0   | PTI_FRAME   | Packet Trace Interface FRAME        | O         |
+-------+-------------+-------------------------------------+-----------+
| PC1   | PTI_DATA    | Packet Trace Interface DATA         | O         |
+-------+-------------+-------------------------------------+-----------+
| PC2   | EUSART1_SCK | mikroBUS SCK                        | O         |
+-------+-------------+-------------------------------------+-----------+
| PC3   | GPIO        | mikroBUS CS                         | O         |
+-------+-------------+-------------------------------------+-----------+
| PC4   | EUSART1_TX  | mikroBUS MOSI                       | O         |
+-------+-------------+-------------------------------------+-----------+
| PC5   | EUSART1_RX  | mikroBUS MISO                       | I         |
+-------+-------------+-------------------------------------+-----------+
| PC6   | GPIO        | mikroBUS RST                        | O         |
+-------+-------------+-------------------------------------+-----------+
| PC7   | GPIO        | Button 0                            | I         |
+-------+-------------+-------------------------------------+-----------+
| PC8   | GPIO        | Test Point GPIO                     | I/O       |
+-------+-------------+-------------------------------------+-----------+
| PC9   | GPIO        | Test Point GPIO                     | I/O       |
+-------+-------------+-------------------------------------+-----------+
| PD0   | LFXO        | LFXO                                | N/A       |
+-------+-------------+-------------------------------------+-----------+
| PD1   | LFXO        | LFXO                                | N/A       |
+-------+-------------+-------------------------------------+-----------+
| PD2   | I2C0_SCL    | mikroBUS SCL / Qwiic SCL            | I/O       |
+-------+-------------+-------------------------------------+-----------+
| PD3   | I2C0_SDA    | mikroBUS SDA / Qwiic SDA            | I/O       |
+-------+-------------+-------------------------------------+-----------+
| PD4   | GPIO        | Test Point GPIO                     | I/O       |
+-------+-------------+-------------------------------------+-----------+
| PD5   | GPIO        | Test Point GPIO                     | I/O       |
+-------+-------------+-------------------------------------+-----------+

.. include:: lyra_24_dvk_common_2.rst.inc

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/observer
   :board: lyra_24_dvk_s10
   :goals: build

.. include:: lyra_24_dvk_common_3.rst.inc

.. _Lyra 24 S datasheet:
   https://www.ezurio.com/documentation/datasheet-lyra-24s

.. _BGM240S datasheet:
   https://www.silabs.com/documents/public/data-sheets/bgm240s-datasheet.pdf

.. _Lyra 24 S DVK user guide:
   https://www.ezurio.com/documentation/user-guide-lyra-24s-development-kit

.. _Lyra 24 S10 DVK schematics:
   https://www.ezurio.com/documentation/schematic-pcb-assembly-dvk-lyra-24s-devboard-integrated-antenna-10dbm-453-00170-k1
