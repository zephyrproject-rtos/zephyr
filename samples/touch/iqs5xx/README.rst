.. _iqs5xx-sample:

iqs5xx
######

Overview
********

This sample reads data from the iqs5xx devices and prints the data reported by the
driver.

The source code shows how to:

#. Get the trackpad device from the :ref:`devicetree <dt-guide>` as a
    :c:struct:`device`
#. Print data from the device.

.. _iqs5xx-sample-requirements:

Requirements
************

Your board must:

#. Have an iqs5xx connected via the i2c bus.
#. Have the RDY pin of the iqs5xx connected to the mcu.
#. Have pullup resistors installed on the i2c bus.

Sample output
=============

You should get a similar output as below.

.. code-block:: console

[00:00:00.424,000] <err> i2c_esp32: I2C transfer error: -14
[00:00:00.424,000] <err> iqs5xx: I2C Transfer Failed, retrying
[00:00:00.425,000] <err> i2c_esp32: I2C transfer error: -116
[00:00:00.425,000] <err> iqs5xx: I2C Transfer Failed, retrying
[00:00:00.445,000] <inf> iqs5xx: I2C Error Corrected
[00:00:00.449,000] <inf> iqs5xx: IQS Driver Probed. Product Number: 0x28
[00:00:06.862,000] <inf> iqs5xx_sample: x: 810, y: 1145
[00:00:06.873,000] <inf> iqs5xx_sample: x: 810, y: 1145
[00:00:06.883,000] <inf> iqs5xx_sample: x: 828, y: 1188
[00:00:06.893,000] <inf> iqs5xx_sample: x: 855, y: 1277
[00:00:06.903,000] <inf> iqs5xx_sample: x: 893, y: 1408
[00:00:06.913,000] <inf> iqs5xx_sample: x: 893, y: 1408
