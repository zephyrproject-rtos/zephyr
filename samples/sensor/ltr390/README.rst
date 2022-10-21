.. _ltr390:

LTR390: Liteon LTR390 ambient light and ultraviolet light sensor
################################################################

Overview
********

This sample application periodically (5 Hz) measures the ambient light and uv index. The results are written to the console.

If triggered measurements are enabled, the sample initializes and maintains a |plusminus| 3 lux window around the current ambient light.
When the temperature moves outside the window an alert is given, and the window is moved to the center of the new light level.

Wiring
******

The LTR390 is available in a discrete component form but it is much easier to use it mounted on a breakout board.
For this sample, I used the Adafruit `LTR390 Sensor`_ breakout board.

.. _`LTR390 Sensor`: https://www.adafruit.com/product/4831

The sample can be configured to support LTR390 sensors using I2C.
Configuration is done via :ref:`devicetree <dt-guide>`. The devicetree
must have an enabled node with ``compatible = "liteon,ltr390";``.
See :dtcompatible:`liteon,ltr390` for the devicetree bindings and ses below for
examples and common configurations.


Building and Running
********************

After providing a devicetree overlay that specifies the sensor location,
build this sample app using:

.. zephyr-app-commands::
    :zephyr-app: samples/sensor/ltr390
    :board: nucleo_f411re
    :goals: build flash
    :compact:

Sample Output
=============

.. code-block:: console

    *** Booting Zephyr OS build v3.2.0-rc1-24-gb10c52635237  ***
    Found device "LTR390_I2C"
    Alert on ambient light [97.000000, 103.000000] lux
    Trigger fired 1, val: 12.799999
    Alert on ambient light [9.799999, 15.799999] lux
    Light:  12.799999 lux   UVI:    0.000000
    Light:  12.799999 lux   UVI:    0.000000
    Light:  12.799999 lux   UVI:    0.000000
    Light:  12.599999 lux   UVI:    0.000000

<repeats endlessly>
