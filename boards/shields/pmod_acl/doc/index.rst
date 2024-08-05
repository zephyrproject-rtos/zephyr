.. pmod_acl:

Digilent Pmod ACL
#################

Overview
********

The Digilent Pmod ACL is a 3-axis digital accelerometer module powered by the
Analog Devices ADXL345.

Programming
***********

Set ``--shield pmod_acl`` when you invoke ``west build``. For example:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/sensor_shell
   :board: apard32690/max32690/m4
   :shield: pmod_acl
   :goals: build

Requirements
************

This shield can only be used with a board which provides a configuration
for Pmod connectors and defines node aliases for SPI and GPIO interfaces
(see :ref:`shields` for more details).

References
**********

- `Pmod ACL product page`_
- `Pmod ACL reference manual`_
- `Pmod ACL schematic`_
- `ADXL345 product page`_
- `ADXL345 data sheet`_

.. _Pmod ACL product page:
   https://digilent.com/shop/pmod-acl-3-axis-accelerometer/

.. _Pmod ACL reference manual:
   https://digilent.com/reference/pmod/pmodacl/reference-manual

.. _Pmod ACL schematic:
   https://digilent.com/reference/_media/reference/pmod/pmodacl/pmodacl_sch.pdf

.. _ADXL345 product page:
   https://www.analog.com/en/products/adxl345.html

.. _ADXL345 data sheet:
   https://www.analog.com/media/en/technical-documentation/data-sheets/adxl345.pdf
