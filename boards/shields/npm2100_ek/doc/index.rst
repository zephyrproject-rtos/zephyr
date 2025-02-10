.. _npm2100_ek:

nPM2100 EK
##########

Overview
********

The nPM2100 EK lets you test different functions and features of the nPM2100
Power Management Integrated Circuit (PMIC).

Requirements
************

The nPM2100 EK board is not designed to fit straight into an Arduino connector.
However, the Zephyr shield is designed expecting it to be connected to the
Arduino shield connectors. That is, the I2C lines need to be connected to
the ``arduino_i2c`` bus. This allows to use the shield with any host board that
supports the Arduino connector.

Usage
*****

The shield can be used in any application by setting ``--shield npm2100_ek``
when invoking ``west build``. You can check :zephyr:code-sample:`npm2100_ek` for a
comprehensive sample.
