.. _mikroe_accel22_click_shield:

MikroElektronika Accel 22 Click
===============================

Overview
********

The `Accel 22 Click`_ is a 3-axis digital accelerometer module powered by the
Analog Devices `ADXL367`_.

Requirements
************

This shield can only be used with a board that provides a mikroBUS socket and
defines a ``mikrobus_spi`` node label for the mikroBUS SPI interface. See
:ref:`shields` for more details.

Programming
***********

Set ``--shield mikroe_accel22_click`` when you invoke ``west build``. For
example:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/accel_polling
   :board: lpcxpresso55s69/lpc55s69/cpu0
   :shield: mikroe_accel22_click
   :goals: build flash

See :dtcompatible:`adi,adxl367` for documentation on the additional binding
properties available for the ADXL367 sensor.

References
**********

- `Accel 22 Click`_
- `Accel 22 Click schematic`_

.. _Accel 22 Click:
   https://www.mikroe.com/accel-22-click

.. _Accel 22 Click schematic:
   https://download.mikroe.com/documents/add-on-boards/click/accel_22_click/Accel_22_Click_v100_Schematic.PDF

.. _ADXL367:
   https://www.analog.com/en/products/adxl367.html
