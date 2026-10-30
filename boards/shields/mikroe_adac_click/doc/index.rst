.. _mikroe_adac_click_shield:

MikroElektronika ADAC Click
===========================

Overview
********

The `ADAC Click`_ is an ADC/DAC module powered by the Analog Devices
`AD5593R`_.

Requirements
************

This shield can only be used with a board that provides a mikroBUS socket and
defines a ``mikrobus_i2c`` node label for the mikroBUS I2C interface. See
:ref:`shields` for more details.

Programming
***********

Set ``--shield mikroe_adac_click`` when you invoke ``west build``. For
example:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/adc/adc_dt
   :board: frdm_mcxn947/mcxn947/cpu0
   :shield: mikroe_adac_click
   :goals: build flash

See :dtcompatible:`adi,ad559x-adc` and :dtcompatible:`adi,ad559x-dac` for
documentation on the additional binding properties available for the AD5593R.

References
**********

- `ADAC Click`_
- `ADAC Click schematic`_

.. _ADAC Click:
   https://www.mikroe.com/adac-click-click

.. _ADAC Click schematic:
   https://download.mikroe.com/documents/add-on-boards/click/adac/adac-click-schematic.pdf

.. _AD5593R:
   https://www.analog.com/en/products/ad5593r.html
