.. _npm1304_ek:

nPM1304 EK
##########

Overview
********

The nPM1304 EK lets you test different functions and features of the nPM1304
Power Management Integrated Circuit (PMIC).

Requirements
************

The nPM1304 EK board is not a direct fit into an Arduino connector. However,
the Zephyr shield is expected to be connected to the Arduino shield connectors.
That is, you need to connect the I2C lines to the ``arduino_i2c`` bus. This allows to
use the shield with any host board that supports the Arduino connector.

Usage
*****

To use the shield in any application, build it with the following command:

.. zephyr-app-commands::
   :board: your_board
   :shield: npm1304_ek
   :goals: build

For a comprehensive sample, refer to :zephyr:code-sample:`npm13xx_ek`.
