.. _npm2100_ek:

nPM2100 EK
##########

Overview
********

The nPM2100 EK lets you test different functions and features of the nPM2100
Power Management Integrated Circuit (PMIC).

Requirements
************

The nPM2100 EK board is not a direct fit into an Arduino connector. However,
the Zephyr shield must be connected to the Arduino shield connectors. That is,
you need to connect the I2C lines to the ``arduino_i2c`` bus. This allows to
use the shield with any host board that supports the Arduino connector.

Usage
*****

To use the shield in any application, build it with the following command:

.. zephyr-app-commands::
   :board: your_board
   :shield: npm2100_ek
   :goals: build

For a comprehensive sample, refer to :zephyr:code-sample:`npm2100_ek`.
