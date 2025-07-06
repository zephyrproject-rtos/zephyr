.. zephyr:code-sample:: accel_trig
   :name: Accelerometer trigger

   Test and debug accelerometer with interrupts.

Overview
********

This sample application demonstrates how to use 3-Axis accelerometers with triggers.
By default it uses a data ready trigger to read the accelerometer data and print it to the console.

If the accelerometer is enabled with a tap trigger, the sample uses the tap trigger event to
read the accelerometer data and print it to the console.

Building and Running
********************

.. code-block:: devicetree

  / {
    aliases {
      accel0 = &fxos8700;
    };
  };

Make sure the aliases are in devicetree, then build and run with:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/accel_trig
   :board: <board to use>
   :goals: build flash
   :compact:

With this example, you can also detect a double tap with an accelerometer by activating the
:kconfig:option:`CONFIG_SAMPLE_TAP_DETECTION`.
In this example we use a x_nucleo_iks01a3 shield with a LIS2DW12 accelerometer.
You can build it with the following command:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/accel_trig
   :board: nrf52dk/nrf52832
   :shield: x_nucleo_iks01a3
   :goals: build
   :west-args: --extra-dtc-overlay x_nucleo_iks01a3.overlay
   :compact:


Sample Output (SENSOR_TRIG_DATA_READY)
=======================================

.. code-block:: console

     fxos8700@1d [m/s^2]:    (   -0.153229,    -0.057461,     9.931148)
     fxos8700@1d [m/s^2]:    (   -0.153229,    -0.057461,     9.931148)
     fxos8700@1d [m/s^2]:    (   -0.143653,    -0.057461,     9.921571)
     fxos8700@1d [m/s^2]:    (   -0.153229,    -0.067038,     9.931148)
     fxos8700@1d [m/s^2]:    (   -0.143653,    -0.067038,     9.921571)
     fxos8700@1d [m/s^2]:    (   -0.134076,    -0.047885,     9.931148)
     fxos8700@1d [m/s^2]:    (   -0.105345,    -0.038308,     9.940725)
     fxos8700@1d [m/s^2]:    (   -0.105345,    -0.019154,     9.931148)
     fxos8700@1d [m/s^2]:    (   -0.105345,    -0.028731,     9.921571)
     fxos8700@1d [m/s^2]:    (   -0.095769,    -0.028731,     9.931148)
     fxos8700@1d [m/s^2]:    (   -0.095769,    -0.009577,     9.940725)


Sample Output (SENSOR_TRIG_DOUBLE_TAP)
======================================

.. code-block:: console

  TAP detected
     lis2dw12@19 [m/s^2]:    (   -1.899901,   -12.550355,    -2.742174)
  TAP detected
     lis2dw12@19 [m/s^2]:    (   12.349357,   -18.125630,     6.015556)
  TAP detected
     lis2dw12@19 [m/s^2]:    (  -11.385050,    -7.274181,    -9.229117)
  TAP detected
     lis2dw12@19 [m/s^2]:    (    9.214760,    -9.286545,     2.311466)
  TAP detected
     lis2dw12@19 [m/s^2]:    (   10.090533,   -17.391034,    12.320643)
  TAP detected
     lis2dw12@19 [m/s^2]:    (   -0.478564,     2.390429,    15.876378)
  TAP detected
     lis2dw12@19 [m/s^2]:    (   -5.668596,   -13.138989,     0.741775)
  TAP detected
     lis2dw12@19 [m/s^2]:    (   -2.385644,   -10.559526,     9.899107)
  TAP detected
     lis2dw12@19 [m/s^2]:    (    7.537391,    -8.551948,    16.740187)
