.. zephyr:code-sample:: distance_polling
   :name: Generic distance measurement
   :relevant-api: sensor_interface

   Measure distance to an object using a distance sensor

Overview
********

This sample application periodically measures the distance of an object and
display it, via the console.

Building and Running
********************

This sample supports up to 5 distance sensors. Each sensor needs to be aliased
as ``distanceN`` where ``N`` goes from ``0`` to ``4``. For example:

.. code-block:: devicetree

  / {
          aliases {
                  distance0 = &vl53l1x;
          };
  };

Make sure the aliases are in devicetree, then build and run with:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/distance_polling
   :board: <board to use>
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

   vl53l1x: 0.153m
   vl53l1x: 0.154m
   vl53l1x: 0.154m
   vl53l1x: 0.153m
