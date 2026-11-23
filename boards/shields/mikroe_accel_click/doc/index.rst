.. _mikroe_accel_click_shield:

MikroElektronika Accel Click
============================

Overview
********

The `Accel Click`_ is a 3-axis digital accelerometer module powered by the
Analog Devices `ADXL345`_.

Requirements
************

This shield can only be used with a board that provides a mikroBUS socket and
defines a ``mikrobus_i2c`` node label for the mikroBUS I2C interface. See
:ref:`shields` for more details.

Programming
***********

Set ``--shield mikroe_accel_click`` when you invoke ``west build``. For
example:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/accel_polling
   :board: frdm_mcxn947/mcxn947/cpu0
   :shield: mikroe_accel_click
   :goals: build flash

See :dtcompatible:`adi,adxl345` for documentation on the additional binding
properties available for the ADXL345 sensor.

References
**********

- `Accel Click`_
- `Accel Click schematic`_

.. _Accel Click:
   https://www.mikroe.com/accel-click

.. _Accel Click schematic:
   https://download.mikroe.com/documents/add-on-boards/click/accel/Accel_Click_v101_Schematic.pdf

.. _ADXL345:
   https://www.analog.com/en/products/adxl345.html
