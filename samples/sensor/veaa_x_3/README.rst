.. veaa_x_3:

VEAA-X-3 sample
##########################

Overview
********

A sensor sample that demonstrates how to use a VEAA-X-3 device.

Building and Running
********************

This sample sets the valve setpoint then reads the actual pressure.
This is done continuously. When the maximum supported pressure is reached the setpoint is reset to
the valve's minimum supported pressure value.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/veaa_x_3
   :board: <board to use>
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

    Testing test_veaa_x_3
    Valve range: 1 to 200 kPa
    Setpoint:    1 kPa, actual:   1 kPa
    Setpoint:    2 kPa, actual:   2 kPa
    Setpoint:    3 kPa, actual:   3 kPa
    ...
    Setpoint:  199 kPa, actual: 199 kPa
    Setpoint:  200 kPa, actual: 200 kPa
    Setpoint:    1 kPa, actual:   1 kPa
    <repeats endlessly>
