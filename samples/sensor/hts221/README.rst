.. _hts221:

HTS221: Temperature and Humidity Monitor
########################################

Overview
********
This sample periodically reads temperature and humidity from the HTS221
Temperature & Humidity Sensor and displays it on the console


Requirements
************

This sample uses the HTS221 sensor controlled using the I2C interface.

References
**********

 - HTS211: http://www.st.com/en/mems-and-sensors/hts221.html

Building and Running
********************

This project outputs sensor data to the console. It requires an HTS221 sensor,
which is present on the ``disco_l475_iot1`` board. To build for that platform:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/hts221
   :board: disco_l475_iot1
   :goals: build
   :compact:

On other platforms, ensure the :ref:`devicetree <dt-guide>` has an enabled
:dtcompatible:`st,hts221` node.

If you get an error on the line of code using :c:macro:`DEVICE_DT_GET_ONE`,
your devicetree is likely to be missing this node.

Sample Output
=============

 .. code-block:: console

    Temperature:25.3 C
    Relative Humidity:40%
    Temperature:25.3 C
    Relative Humidity:40%
    Temperature:25.3 C
    Relative Humidity:40%
    Temperature:25.3 C
    Relative Humidity:40%

    <repeats endlessly every 2 seconds>
