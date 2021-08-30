.. _lps22hb:

LPS22HB: Temperature and Humidity Monitor
#########################################

Overview
********
This sample periodically reads pressure from the LPS22HB MEMS pressure
sensor and displays it on the console.


Requirements
************

This sample uses the LPS22HB sensor controlled using the I2C interface.

References
**********

- LPS22HB: http://www.st.com/en/mems-and-sensors/lps22hb.html

Building and Running
********************

This project outputs sensor data to the console. It requires an LPS22HB sensor,
which is present on the ``disco_l475_iot1`` board. To build for that platform:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/lps22hb
   :board: disco_l475_iot1
   :goals: build
   :compact:

On other platforms, ensure the :ref:`devicetree <dt-guide>` has an enabled
:dtcompatible:`st,lps22hb-press` node.

If you get an error on the line of code using :c:macro:`DEVICE_DT_GET_ONE`,
your devicetree is likely to be missing this node.

Sample Output
=============

.. code-block:: console

   Observation:1
   Pressure:98.7 kPa
   Temperature:22.5 C
   Observation:2
   Pressure:98.7 kPa
   Temperature:22.5 C
   Observation:3
   Pressure:98.7 kPa
   Temperature:22.5 C

   <repeats endlessly every 2 seconds>
