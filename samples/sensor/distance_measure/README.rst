.. zephyr:code-sample:: hc-sr04
   :name: Generic distance measurement
   :relevant-api: sensor_interface

   To measure distance of an object using distance sensor

Overview
********

This sample application periodically measures the distance of an object and
display it, via the console

Building and Running
********************

This sample supports up to 5 distance sensors. Each sensor needs to be aliased
as ``distanceN`` where ``N`` goes from ``0`` to ``4``. For example:

.. code-block:: devicetree

  / {
          aliases {
                  distance0 = &hc_sr04;
          };
  };

Make sure the aliases are in devicetree, then build and run with:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/hc_sr04
   :board: <board to use>
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

   hc_sr04: 0.153000 (meters)
   hc_sr04: 0.154000 (meters)
   hc_sr04: 0.154000 (meters)
   hc_sr04: 0.153000 (meters)
