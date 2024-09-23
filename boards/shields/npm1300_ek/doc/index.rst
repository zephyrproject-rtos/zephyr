.. _npm1300_ek:

nPM1300 EK
##########

Overview
********

The nPM1300 EK lets you test different functions and features of the nPM1300
Power Management Integrated Circuit (PMIC).

Requirements
************

The nPM1300 EK board is not designed to fit straight into an Arduino connector.
However, the Zephyr shield is designed expecting it to be connected to the
Arduino shield connectors. For example, the I2C lines need to be connected to
the ``arduino_i2c`` bus. This allows to use the shield with any host board that
supports the Arduino connector.

Usage
*****

The shield can be used in any application by setting ``--shield npm1300_ek``
when invoking ``west build``. You can check :zephyr:code-sample:`npm1300_ek` for a
comprehensive sample.

References
**********

TBC
