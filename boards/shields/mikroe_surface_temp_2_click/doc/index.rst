.. _mikroe_surface_temp_2_click_shield:

MikroElektronika Surface Temp 2 Click
=====================================

Overview
********

The `Surface Temp 2 Click`_ is a 16-bit digital temperature sensor module
powered by the Analog Devices `ADT7422`_.

Requirements
************

This shield can only be used with a board that provides a mikroBUS socket and
defines a ``mikrobus_i2c`` node label for the mikroBUS I2C interface. See
:ref:`shields` for more details.

Programming
***********

Set ``--shield mikroe_surface_temp_2_click`` when you invoke ``west build``.
For example:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/thermometer
   :board: frdm_mcxn947/mcxn947/cpu0
   :shield: mikroe_surface_temp_2_click
   :goals: build flash

See :dtcompatible:`adi,adt7422` for documentation on the additional binding
properties available for the ADT7422 sensor.

References
**********

- `Surface Temp 2 Click`_
- `Surface Temp 2 Click schematic`_

.. _Surface Temp 2 Click:
   https://www.mikroe.com/surface-temp-2-click

.. _Surface Temp 2 Click schematic:
   https://download.mikroe.com/documents/add-on-boards/click/surface_temp_2_click/surface-temp-2-click-schematic-v100.pdf

.. _ADT7422:
   https://www.analog.com/en/products/adt7422.html
