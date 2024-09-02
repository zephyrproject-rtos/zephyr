.. zephyr:code-sample:: proximity_polling
   :name: Proximity sensor
   :relevant-api: sensor_interface

   Get proximity data from up to 10 proximity sensors (polling mode).

Overview
********

This sample demonstrates how to use one or multiple proximity sensors.

Building and Running
********************

The sample supports up to 10 proximity sensors. The number of the sensors will
automatically detected from the devicetree, you only need to set aliases from
``prox-sensor-0`` to ``prox-sensor-9``.

For example:

.. code-block:: devicetree

   / {
              aliases {
                      prox-sensor0 = &tmd2620;
              };
   };

After creating the devicetree overlay you can build the sample with:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/proximity_polling
   :board: <your_board>
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

   *** Booting Zephyr OS build zephyr-v3.2.0-210-gd95295f08646  ***
   Proximity sensor sample application
   Found 1 proximity sensor(s): tmd2620@29
   Proximity on tmd2620@29: 202
   Proximity on tmd2620@29: 205
   Proximity on tmd2620@29: 204
   Proximity on tmd2620@29: 60
   Proximity on tmd2620@29: 1
