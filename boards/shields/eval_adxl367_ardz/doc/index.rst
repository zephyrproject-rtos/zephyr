.. _eval_adxl367_ardz:

EVAL-ADXL367-ARDZ
#################

Overview
********

The EVAL-ADXL367-ARDZ is a 3-axis digital accelerometer Arduino shield powered
by the Analog Devices ADXL367.

Programming
***********

Set ``--shield eval_adxl367_ardz`` when you invoke ``west build``. For example:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/sensor_shell
   :board: apard32690/max32690/m4
   :shield: eval_adxl367_ardz
   :goals: build

Requirements
************

This shield can only be used with a board which provides a configuration for
Arduino connectors and defines node aliases for SPI and GPIO interfaces (see
:ref:`shields` for more details).

References
**********

- `ADXL367 product page`_
- `ADXL367 data sheet`_

.. _ADXL367 product page:
   https://www.analog.com/en/products/adxl367.html

.. _ADXL367 data sheet:
   https://www.analog.com/media/en/technical-documentation/data-sheets/adxl367.pdf
