.. zephyr:code-sample:: magn_polling
   :name: Magnetometer Sensor
   :relevant-api: sensor_interface

   Get magnetometer data from a magnetometer sensor (polling mode).

Overview
********

Sample application that periodically reads magnetometer (X, Y, Z) data from
the first available device that implements SENSOR_CHAN_MAGN_* (predefined array
of device names).
