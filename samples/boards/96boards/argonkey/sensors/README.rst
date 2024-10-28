.. zephyr:code-sample:: argonkey_sensors
   :name: Sensors

   Read sensor data from the ArgonKey board's onboard sensors.

Overview
********
This sample provides an example of how to read sensor data
from the ArgonKey board. The result is displayed on the console.
It makes use of both the trigger and poll methods.

Requirements
************

This sample just requires the ArgonKey board. The board can be powered
in either one of the following two ways:

- mezzanine mode, plugging the ArgonKey to HiKey board through its 96Board
  low-speed connector
- standalone mode, supplying 5V directly on P1 connector

The user may select or unselect the sensors from
:zephyr_file:`samples/boards/96boards/argonkey/sensors/prj.conf`.

Please note that all sensor related code is conditionally compiled
using the ``#ifdef`` directive, so this sample is supposed to always
build correctly. Example:

.. code-block:: c

    #ifdef CONFIG_HTS221
      struct device *hum_dev = DEVICE_DT_GET_ONE(st_hts221);

      if (!device_is_ready(hum_dev)) {
        printk("%s: device not ready.\n", hum_dev->name);
        return;
      }
    #endif

References
**********

- :ref:`96b_argonkey`

Building and Running
********************

.. zephyr-app-commands::
    :zephyr-app: samples/boards/96boards/argonkey/sensors
    :host-os: unix
    :board: 96b_argonkey
    :goals: run
    :compact:

Sample Output
=============

A USB to TTL 1V8 serial cable may be attached to the low speed connector on
the back of the board on P3.5 (TX) and P3.7 (RX). User may use a simple
terminal emulator, such as minicom, to capture the output.

 .. code-block:: console

    proxy: 1  ;
    distance: 0 m -- 09 cm;
    temp: 30.35 C; press: 97.466259
    humidity: 43.500000
    accel (0.004000 -0.540000 9.757000) m/s2
    gyro (0.065000 -0.029000 -0.001000) dps
    magn (0.049500 0.208500 -0.544500) gauss
    - (6) (trig_cnt: 1878)

    <repeats endlessly every 2s>

In this example the output is generated polling the sensor every 2 seconds.

Sensor data is printed in the following order (no data is printed for
sensors that are not enabled):

#. *VL53L0x* proximity
#. *LPS22HB* baro/temp
#. *HTS221* humidity
#. *LSM6DSL* accel
#. *LSM6DSL* gyro
#. *LIS2MDL* magnetometer (attached to *LSM6DSL*)

The proximity sensor displays two lines:

- a flag (proxy) that goes on when the distance is below 10cm
- the absolute distance from the obstacle

The last line displays a counter of how many trigger interrupts
has been received.  It is possible to display the sensor data
read for each trigger by enabling the **ARGONKEY_TEST_LOG** macro.
