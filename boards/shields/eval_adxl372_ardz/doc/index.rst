.. eval_adxl372_ardz:

EVAL-ADXL372-ARDZ
#################

Overview
********

The EVAL-ADXL372-ARDZ is a 3-axis digital accelerometer Arduino shield powered
by the Analog Devices ADXL372.

Programming
***********

Set ``--shield eval_adxl372_ardz`` when you invoke ``west build``. For example:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/sensor_shell
   :board: apard32690/max32690/m4
   :shield: eval_adxl372_ardz
   :goals: build

Requirements
************

This shield can only be used with a board which provides a configuration for
Arduino connectors and defines node aliases for SPI and GPIO interfaces (see
:ref:`shields` for more details).

References
**********

- `EVAL-ADXL372-ARDZ product page`_
- `EVAL-ADXL372-ARDZ user guide`_
- `EVAL-ADXL372-ARDZ schematic`_
- `ADXL372 product page`_
- `ADXL372 data sheet`_

.. _EVAL-ADXL372-ARDZ product page:
   https://www.analog.com/en/resources/evaluation-hardware-and-software/evaluation-boards-kits/eval-adxl372-ardz.html

.. _EVAL-ADXL372-ARDZ user guide:
   https://wiki.analog.com/resources/eval/user-guides/eval-adicup3029/hardware/adxl372

.. _EVAL-ADXL372-ARDZ schematic:
   https://www.analog.com/media/en/evaluation-documentation/evaluation-design-files/eval-adxl372-ardz-designsupport.zip

.. _ADXL372 product page:
   https://www.analog.com/en/products/adxl372.html

.. _ADXL372 data sheet:
   https://www.analog.com/media/en/technical-documentation/data-sheets/adxl372.pdf
