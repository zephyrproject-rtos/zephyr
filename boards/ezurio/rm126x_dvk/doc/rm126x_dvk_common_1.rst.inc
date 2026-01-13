.. SPDX-License-Identifier: Apache-2.0
..
.. Copyright (c) 2026 Ezurio LLC

Overview
********

The |RM126x variant| Development Kit provides support for the Ezurio |RM126x variant| LoRa module.

The module includes an EFR32BG22 Arm Cortex-M33 CPU and a Semtech |SX126x variant| LoRa radio.

The |RM126x variant| module incorporates the QFN package EFR32BG22 (512kB Flash, 32kB RAM). The part features
up to 16 configurable GPIOs and Lora Radio TX Power up to |LoRa TX Power|.

The kit features a USB interface, an on-board SEGGER J-Link debugger, one user-LED and button, and
support for hardware add-on boards via a `mikroBUS`_ socket and a `Qwiic`_ connector.

.. note::
   You can find more information about the RM126x family of modules in the `RM126x product brief`_, the `RM126x datasheet`_
   and on the `RM126x website`_.

   You can find more information about the underlying EFR32BG22 SoC in the `EFR32BG22 datasheet`_
   and `EFR32BG22 reference manual`_.

   You can find more information about the Semtech |SX126x variant| LoRa radio in the `SX126x datasheet`_.

Hardware
********

The |RM126x variant| DVK has one crystal oscillator as follows.

* Low-frequency 32.768 kHz crystal oscillator (LFXO)

The crystal oscillator is fitted within the |RM126x variant| module.

The module supports an external LoRa antenna.

.. note::
   The MHF4 connector exposed via the |RM126x variant| shield can should be used for antenna connectivity.

Full details of the DVK can be found in the `RM126x DVK user guide`_ and |Schematics Ref|.

Supported Features
==================

.. zephyr:board-supported-hw::

DVK Connections and IOs
=======================

In the following table, the column **Name** contains Pin names. For example, PA2
means Pin number 2 on PORTA, as used in the board's datasheets and manuals.

The **Direction** column indicates the pin direction from the module perspective, with
I indicating Input, O Output and I/O both.

+-------+----------------------+--------------------------------------------------------+-----------+
| Name  | Function             | Usage                                                  | Direction |
+=======+======================+========================================================+===========+
| PA1   | SWD_SWCLK            | JLink SWCLK                                            | I         |
+-------+----------------------+--------------------------------------------------------+-----------+
| PA2   | SWD_SWDIO            | JLink SWDIO                                            | I/O       |
+-------+----------------------+--------------------------------------------------------+-----------+
| PA3   | SWD_SWO              | JLink SWO                                              | O         |
+-------+----------------------+--------------------------------------------------------+-----------+
| PB2   | EUART0_RTS           | UART Console VCOM_RTS                                  | O         |
+-------+----------------------+--------------------------------------------------------+-----------+
| PB3   | EUART0_RX            | UART Console VCOM_RX                                   | I         |
+-------+----------------------+--------------------------------------------------------+-----------+
| PB4   | EUART0_CTS           | UART Console VCOM_CTS                                  | I         |
+-------+----------------------+--------------------------------------------------------+-----------+
| PC0   | USART0_SCK           | mikroBUS SCK                                           | O         |
+-------+----------------------+--------------------------------------------------------+-----------+
| PC1   | GPIO                 | mikroBUS CS                                            | I/O       |
+-------+----------------------+--------------------------------------------------------+-----------+
| PC2   | GPIO                 | mikroBUS AN                                            | I         |
+-------+----------------------+--------------------------------------------------------+-----------+
| PC3   | GPIO                 | mikroBUS RST                                           | O         |
+-------+----------------------+--------------------------------------------------------+-----------+
| PC4   | GPIO                 | mikroBUS PWM                                           | O         |
+-------+----------------------+--------------------------------------------------------+-----------+
| PC5   | GPIO                 | mikroBUS INT / LED0                                    | I/O       |
+-------+----------------------+--------------------------------------------------------+-----------+
| PC6   | GPIO                 | Button 0                                               | I         |
+-------+----------------------+--------------------------------------------------------+-----------+
| PC7   | EUART0_TX            | UART Console VCOM_TX                                   | O         |
+-------+----------------------+--------------------------------------------------------+-----------+
| PD2   | USART0_RX / I2C0_SCL | mikroBUS MISO / mikroBUS RX / mikroBUS SCL / Qwiic SCL | I/O       |
+-------+----------------------+--------------------------------------------------------+-----------+
| PD3   | USART0_TX / I2C0_SDA | mikroBUS MOSI / mikroBUS TX / mikroBUS SDA / Qwiic SDA | I/O       |
+-------+----------------------+--------------------------------------------------------+-----------+

.. note::
   MikroBUS INT and LED0 are multiplexed to the same I/O. Usage is determined by solder bridge SB4. Refer to
   the `RM126x DVK user guide`_ and |Schematics Ref| for further details.

   The solder bridge defaults to the closed position to enable LED0 connectivity.

.. note::
   MikroBUS MISO, RX and SCL are multiplexed to the same I/O. Usage is determined by solder bridges SB5, SB7 and SB8.
   Refer to the `RM126x DVK user guide`_ and |Schematics Ref| for further details.

   The SB7 solder bridge defaults to closed to enable MISO connectivity.

.. note::
   MikroBUS MOSI, TX and SDA are multiplexed to the same I/O. Usage is determined by solder bridges SB9, SB11 and SB12.
   Refer to the `RM126x DVK user guide`_ and |Schematics Ref| for further details.

   The SB11 solder bridge defaults to closed to enable MOSI connectivity.

.. note::
   Qwiic SCL is multipexed to the same I/O as MikroBUS MISO, RX and SCL. MikroBUS I2C based boards can be used
   in conjunction with Qwiic based boards by closing solder bridges SB5 and SB6.

   Qwiic SDA is multipexed to the same I/O as MikroBUS MOSI, TX and SDA. MikroBUS I2C based boards can be used
   in conjunction with Qwiic based boards by closing solder bridges SB9 and SB10.

   Refer to the `RM126x DVK user guide`_ and |Schematics Ref| for further details.

EFR32BG22 To |SX126x variant| Radio Connections
===============================================

The following are internal to the |RM126x variant| module and describe connectivity between the
EFR32BG22 SoC and the |SX126x variant| LoRa radio.

In the following table, the column **Name** contains Pin names. For example, PA2
means Pin number 2 on PORTA, as used in the board's datasheets and manuals.

The **Direction** column indicates the pin direction from the SoC perspective, with
I indicating Input and O Output.

+-------+---------------------+------------------------------------------------+-----------+
| Name  | Function            | Usage                                          | Direction |
+=======+=====================+================================================+===========+
| PA0   | USART1_SCK          | |SX126x variant| SCK                           | O         |
+-------+---------------------+------------------------------------------------+-----------+
| PA4   | USART1_TX           | |SX126x variant| MOSI                          | O         |
+-------+---------------------+------------------------------------------------+-----------+
| PA5   | GPIO                | |SX126x variant| DIO1                          | I         |
+-------+---------------------+------------------------------------------------+-----------+
| PA6   | GPIO                | |SX126x variant| RESET                         | O         |
+-------+---------------------+------------------------------------------------+-----------+
| PA7   | GPIO                | |SX126x variant| BUSY                          | I         |
+-------+---------------------+------------------------------------------------+-----------+
| PA8   | USART1_RX           | |SX126x variant| MISO                          | I         |
+-------+---------------------+------------------------------------------------+-----------+
| PB0   | GPIO                | |SX126x variant| CS                            | O         |
+-------+---------------------+------------------------------------------------+-----------+
| PB1   | GPIO                | |SX126x variant| ANT SW                        | O         |
+-------+---------------------+------------------------------------------------+-----------+

|SX126x variant| Radio Connections
==================================

In the following table, the column **Name** contains Pin names as defined in the `SX126x datasheet`_.

The **Direction** column indicates the pin direction from the radio perspective, with
O indicating Output.

+-------+---------------------+------------------------------------------------+-----------+
| Name  | Function            | Usage                                          | Direction |
+=======+=====================+================================================+===========+
| DIO2  | GPIO                | RF Direction                                   | O         |
+-------+---------------------+------------------------------------------------+-----------+
| DIO3  | GPIO                | TCXO Enable                                    | O         |
+-------+---------------------+------------------------------------------------+-----------+

System Clock
============

The |RM126x variant| is configured to use the internal HFRCO oscillator as the System Clock at 38MHz.
It can operate at clock speeds of up to 80 MHz.

Serial Port
===========

The |RM126x variant| has two USARTs and one EUART.

* USART0 is mapped to the mikroBUS SPI port.
* USART1 is dedicated to the |SX126x variant| radio.
* EUART0 is connected to the board controller and is used for the console.

Programming and Debugging
*************************

Applications for the |Board Quoted| board can be built, flashed, and debugged in the usual way.
See :ref:`build_an_application` and :ref:`application_run` for more details on building and running.

.. note::
   Before using the kit, you should update the J-Link firmware
   in `Simplicity Studio`_.

Testing the LED and button
==========================

The :zephyr:code-sample:`blinky` sample can be used to test the DVK LED.

The :zephyr:code-sample:`button` sample can be used to test the DVK button.

Testing LoRa
============

The :zephyr:code-sample:`lorawan-class-a` sample can be programmed to an |RM126x variant| DVK to demonstrate
joining and uplinking to a LoRaWAN network server.

This is built as follows.
