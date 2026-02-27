.. _lp_em_cc2340r5moda:

TI CC2340R5 Module LaunchPad
############################

Overview
********

The Texas Instruments CC2340R5 Module LaunchPad |trade| (LP_EM_CC2340R5MODA) is a
development kit for the SimpleLink |trade| CC2340R5 wireless module (MODA package).

Hardware
********

The CC2340R5 Module LaunchPad |trade| development kit features the CC2340R5MODA
wireless module. The board is equipped with two LEDs, two push buttons and
BoosterPack connectors for expansion. Note that due to the module package, many
BoosterPack pins are not connected.

The CC2340R5 wireless MCU has a 48 MHz Arm |reg| Cortex |reg|-M0+ SoC and an
integrated 2.4 GHz transceiver supporting multiple protocols including Bluetooth
|reg| Low Energy and IEEE |reg| 802.15.4.

Supported Features
==================

The CC2340R5 Module LaunchPad board configuration supports the following hardware
features:

+-----------+------------+----------------------+
| Interface | Controller | Driver/Component     |
+===========+============+======================+
| GPIO      | on-chip    | gpio                 |
+-----------+------------+----------------------+
| UART      | on-chip    | serial               |
+-----------+------------+----------------------+
| SPI       | on-chip    | spi                  |
+-----------+------------+----------------------+
| I2C       | on-chip    | i2c                  |
+-----------+------------+----------------------+
| PWM       | on-chip    | pwm                  |
+-----------+------------+----------------------+
| Watchdog  | on-chip    | watchdog             |
+-----------+------------+----------------------+
| Crypto    | on-chip    | crypto               |
+-----------+------------+----------------------+

Connections and IOs
===================

Pin assignments differ from the standard CC2340R5 LaunchPad due to the module
package. Many BoosterPack header pins are not connected.

+-------+-----------+---------------------+
| Pin   | Function  | Usage               |
+=======+===========+=====================+
| DIO3  | GPIO      |                     |
+-------+-----------+---------------------+
| DIO4  | GPIO      |                     |
+-------+-----------+---------------------+
| DIO6  | UART0_RX  | UART TX             |
+-------+-----------+---------------------+
| DIO8  | GPIO/I2C  | Green LED / I2C SDA |
+-------+-----------+---------------------+
| DIO11 | SPI_CSN   | SPI CS / Flash CS   |
+-------+-----------+---------------------+
| DIO12 | SPI_PICO  | SPI PICO            |
+-------+-----------+---------------------+
| DIO18 | GPIO/SPI  | Button 1 / SPI CLK  |
+-------+-----------+---------------------+
| DIO20 | UART0_TX  | UART RX             |
+-------+-----------+---------------------+
| DIO21 | GPIO/SPI  | Red LED / SPI POCI  |
+-------+-----------+---------------------+
| DIO24 | GPIO/I2C  | Button 2 / I2C SCL  |
+-------+-----------+---------------------+

Shared PIN configuration
========================

Due to the limited pin count of the module package, several peripheral functions
share pins with LEDs and buttons. By default, SPI and I2C are disabled in the
device tree to allow LEDs and buttons to function.

SPI/I2C Pin Conflicts
---------------------

The following pins have multiple functions that conflict:

+-------+-------------------------------------------+
| Pin   | Function         | Jumper Required        |
+=======+==================+========================+
| DIO8  | Green LED        | P2 pins 5-6            |
+-------+------------------+------------------------+
| DIO8  | I2C SDA          | P2 pins 3-5            |
+-------+------------------+------------------------+
| DIO18 | SPI SCLK         | P6 pins 3-5            |
+-------+------------------+------------------------+
| DIO18 | Button 1         | P6 pins 5-6            |
+-------+------------------+------------------------+
| DIO21 | Red LED          | P2 pins 2-1            |
+-------+------------------+------------------------+
| DIO21 | SPI POCI         | P2 pins 2-4            |
+-------+------------------+------------------------+
| DIO24 | Button 2         | P6 pins 2-1            |
+-------+------------------+------------------------+
| DIO24 | I2C SCL          | P6 pins 2-4            |
+-------+------------------+------------------------+

To enable SPI or I2C:

1. Install the appropriate jumpers on headers P2 and P6
2. Note that the corresponding LED or button will no longer function

Programming and Debugging
*************************

The LP_EM_CC2340R5MODA requires an external debug probe such as the LP-XDS110 or
LP-XDS110ET.

References
**********

CC2340R5 LaunchPad Quick Start Guide:
  https://www.ti.com/lit/pdf/swru588

.. _TI CC2340R5 LaunchPad Product Page:
   https://www.ti.com/tool/LP-EM-CC2340R5

.. _TI CC2340R5 Product Page:
   https://www.ti.com/product/CC2340R5
