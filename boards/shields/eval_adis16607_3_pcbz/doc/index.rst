.. _eval_adis16607_3_pcbz:

EVAL-ADIS16607-3-PCBZ
######################

Overview
********

The EVAL-ADIS16607-3-PCBZ is an evaluation board for the Analog Devices
ADIS16607-3, a 6-degrees-of-freedom IMU combining a +/-40 g accelerometer
with a +/-2000 deg/s gyroscope.

Programming
***********

Set ``--shield eval_adis16607_3_pcbz`` when you invoke ``west build``. For example:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/6dof_motion_drdy/
   :board: apard32690/max32690/m4
   :shield: eval_adis16607_3_pcbz
   :goals: build

Requirements
************

This shield can only be used with a board which provides a configuration for
Arduino connectors and defines node aliases for SPI and GPIO interfaces (see
:ref:`shields` for more details).

References
**********

- `ADIS16607 product page`_
- `EVAL-ADIS16607 user guide`_

.. _ADIS16607 product page:
   https://www.analog.com/en/products/adis16607.html

.. _EVAL-ADIS16607 user guide:
   https://www.analog.com/media/en/technical-documentation/user-guides/eval-adis16607.pdf
