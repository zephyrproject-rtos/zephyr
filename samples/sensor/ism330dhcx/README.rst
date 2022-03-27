.. _ism330dhcx:

ISM330DHCX: Accelerometer and Gyroscope Sensor
##############################################

Overview
********

This sample sets the date rate of ISM330DHCX accelerometer and gyroscope
to 12.5Hz and enables a trigger on data ready. It displays on the console
the axes values for accelerometer and gyroscope.

Requirements
************

This sample uses the ISM330DHCX sensor controlled using the I2C interface.
It has been tested on the :ref:`b_u585i_iot02a_board`.

References
**********

For more information about the ISM330DHCX sensor module, see `ISM330DHCX Sensor Module website`_.

Building and Running
********************

This project outputs sensor data to the console. It requires an ISM330DHCX
sensor, which is present on the :ref:`b_u585i_iot02a_board`.

Building on b_u585i_iot02a board
================================

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/ism330dhcx
   :host-os: unix
   :board: b_u585i_iot02a
   :goals: build
   :compact:

Sample Output
=============

.. code-block:: console

   Testing ISM330DHCX sensor in trigger mode.

   accel x:19.599609 m/s2 y:-19.599609 m/s2 z:19.599609 m/s2
   gyro x:-0.020005 dps y:0.021762 dps z:0.148592 dps
   trig_cnt:1

   accel x:-0.004187 m/s2 y:-0.156131 m/s2 z:9.889535 m/s2
   gyro x:-0.010231 dps y:0.008475 dps z:0.046196 dps
   trig_cnt:2

   <repeats endlessly>


.. _ISM330DHCX sensor module website:
   https://www.st.com/en/mems-and-sensors/ism330dhcx.html
