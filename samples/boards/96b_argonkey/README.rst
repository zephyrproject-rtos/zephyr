.. _ArgonKey:

ArgonKey sensor board
#####################

Overview
********
This sample provides an example of how to read sensor data
from the ArgonKey board. The result is displayed on the console.
It makes use of both the trigger and poll methods.

Requirements
************

This sample just requires the ArgonKey board.
The user may select or unselect the sensors from
:file:`samples/boards/96b_argonkey/prj.conf`.

Please note that all sensor related code is conditionally compiled
using the `#ifdef` directive, so this sample is supposed to always
build correctly. Example:

 .. code-block:: c

    #ifdef CONFIG_HTS221
      struct device *hum_dev = device_get_binding("HTS221");

      if (!hum_dev) {
        printk("Could not get pointer to %s sensor\n", "HTS221");
        return;
      }
    #endif

References
**********

- :ref:`96b_argonkey`

Building and Running
********************

 .. zephyr-app-commands::
    :zephyr-app: samples/boards/96b_argonkey
    :host-os: unix
    :board: 96b_argonkey
    :goals: run
    :compact:

Sample Output
=============

 .. code-block:: console

    temp: 24.78 C; press: 101.448535
    humidity: 43.000000
    accel (4.121000 -6.859000 -5.384000) m/s2
    gyro (-0.008000 0.270000 0.161000) dps
    magn (0.021000 -0.552000 0.271500) gauss
    - (6) (trig_cnt: 254)

    <repeats endlessly every 2s>

In this example the output is generated polling the sensor every 2 seconds.
Sensor data is printed in the following order (no data is printed for
sensors that are not enabled):

#. *LPS22HB* baro/temp
#. *HTS221* humidity
#. *LSM6DSL* accel
#. *LSM6DSL* gyro
#. *LIS2MDL* magnetometer (attached to *LSM6DSL*)

The last line displays a counter of how many trigger interrupts
has been received.  It is possible to display the sensor data
read for each trigger by enabling the **ARGONKEY_TEST_LOG** macro.
