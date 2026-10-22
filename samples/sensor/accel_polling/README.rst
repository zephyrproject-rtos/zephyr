.. zephyr:code-sample:: accel_polling
   :name: Generic 3-Axis accelerometer polling
   :relevant-api: sensor_interface

   Get 3-Axis accelerometer data from a sensor (polling mode).

Overview
********

This sample application demonstrates how to use 3-Axis accelerometers using the
:ref:`RTIO framework <rtio>` based :ref:`Read and Decode method <sensor-read-and-decode>`
in polling mode (sensor_read).

Building and Running
********************

This sample supports up to 10 3-Axis accelerometers. Each accelerometer needs
to be aliased as ``accelN`` where ``N`` goes from ``0`` to ``9``.
For example, in case of x_nucleo_iks4a1 shield:

.. code-block:: devicetree

  / {
	aliases {
			accel0 = &lsm6dso16is_6a_x_nucleo_iks4a1;
			accel1 = &lsm6dsv16x_6b_x_nucleo_iks4a1;
			accel2 = &lis2duxs12_1e_x_nucleo_iks4a1;
		};
	};

Then build for this shield and run with:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/accel_polling
   :board: <board to use>
   :shield: x_nucleo_iks4a1
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

       lsm6dso16is@6a [m/s^2]:    (    0.923629,    -0.084945,     9.891330)
        lsm6dsv16x@6b [m/s^2]:    (   -0.059820,     0.862612,     9.827322)
        lis2duxs12@19 [m/s^2]:    (    0.851844,    -0.028713,     9.896714)
       lsm6dso16is@6a [m/s^2]:    (    0.924825,    -0.072981,     9.894919)
        lsm6dsv16x@6b [m/s^2]:    (   -0.061615,     0.864407,     9.825527)
        lis2duxs12@19 [m/s^2]:    (    0.823131,    -0.057427,     9.915857)
       lsm6dso16is@6a [m/s^2]:    (    0.928415,    -0.081355,     9.898508)
        lsm6dsv16x@6b [m/s^2]:    (   -0.061615,     0.864407,     9.829117)
        lis2duxs12@19 [m/s^2]:    (    0.832702,    -0.047856,     9.935000)
       lsm6dso16is@6a [m/s^2]:    (    0.922433,    -0.078963,     9.917053)
        lsm6dsv16x@6b [m/s^2]:    (   -0.063409,     0.864407,     9.825527)
        lis2duxs12@19 [m/s^2]:    (    0.832702,    -0.057427,     9.906286)
