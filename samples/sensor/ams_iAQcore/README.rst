.. _ams_iaqcore:

ams iAQcore Indoor air quality sensor
#####################################

Overview
********

This sample application demonstrates how to use the ams iAQcore sensor to
measure CO2 equivalent and VOC. The CO2 value is a predicted value derived from
VOC. The values are fetched and printed every second.

Building and Running
********************

This sample can be built as-is for any board with an Arduino-style I2C header
configured in its :ref:`devicetree <dt-guide>`, as configured in the
:zephyr_file:`samples/sensor/ams_iAQcore/app.overlay` devicetree overlay file.

(To build the sample on other platforms, use another devicetree overlay file.
See :ref:`set-devicetree-overlays`.)

Flash the binary to your board with a sensor connected to the I2C bus
configured via devicetree.

For example, to build and flash for the ``nucleo_f446re`` board:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/ams_iAQcore
   :board: nucleo_f446re
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

    device is 0x20001a08, name is IAQ_CORE
    Co2: 882.000000ppm; VOC: 244.000000ppb
    Co2: 863.000000ppm; VOC: 239.000000ppb
    Co2: 836.000000ppm; VOC: 232.000000ppb
