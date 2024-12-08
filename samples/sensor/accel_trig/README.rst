.. zephyr:code-sample:: accel_trig
   :name: Accelerometer trigger

   Test and debug accelerometer with interrupts.

Overview
********

This sample application demonstrates how to use 3-Axis accelerometers with triggers.

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

Sample Output
=============

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
