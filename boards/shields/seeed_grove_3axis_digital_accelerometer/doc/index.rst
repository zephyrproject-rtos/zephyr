.. _seeed_grove_3axis_digital_accelerometer:

SeeedStudio Grove 3-Axis Digital Accelerometer
##############################################

Overview
********

The SeeedStudio Grove 3-Axis Digital Accelerometer is a compact sensor module
hat provides digital output of 3-axis acceleration.
There are several variations by the measurement range and chip.

.. image:: img/grove_3axis_digital_accelerometer_lis3dhtr.webp
     :align: center
     :alt: Grove - 3-Axis Digital Accelerometer LIS3DHTR

     Grove - 3-Axis Digital Accelerometer LIS3DHTR (Credit: SeeedStudio)

These allows for easy integration with Grove connector system.
More information about the Grove connector system can be found at the
`Grove Ecosystem Introduction`_.

Hardware
********

Currently the following models are supported:

- seeed_grove_lis3dhtr: see `LIS3DHTR: Grove - 3-Axis Digital Accelerometer`_

Programming
***********

Set ``--shield seeed_grove_[sensor_model]`` when you invoke ``west build``.
For example:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/sensor_shell
   :board: wio_terminal/samd51p19a
   :shield: seeed_grove_adxl345
   :goals: build

This shield can take a option to select grove bus.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/sensor_shell
   :board: wio_terminal/samd51p19a
   :shield: seeed_grove_adxl345@grove_i2c1
   :goals: build

References
**********

.. target-notes::

.. _Grove Ecosystem Introduction:
   https://wiki.seeedstudio.com/Grove_System/

.. _LIS3DHTR - Grove - 3-Axis Digital Accelerometer:
   https://wiki.seeedstudio.com/Grove-3-Axis-Digital-Accelerometer-LIS3DHTR/
