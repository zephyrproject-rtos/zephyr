.. zephyr:code-sample:: s3km1110
   :name: Iclegend S3KM1110 mmWave sensor
   :relevant-api: sensor_interface s3km1110_interface

   Read standard and custom channels from an Iclegend S3KM1110 mmWave radar sensor.

Overview
********

This sample fetches data from a :dtcompatible:`iclegend,s3km1110` sensor and prints:

- Occupancy and "target status" by reading the :c:enumerator:`SENSOR_CHAN_PROX` standard channel, as
  well as the :c:enumerator:`SENSOR_CHAN_S3KM1110_TARGET_STATUS` custom channel which provides
  additional information regarding the detected presence (static, moving, or both).

- Detection distance using the :c:enumerator:`SENSOR_CHAN_DISTANCE` standard channel, moving-target
  and static-target distances using the :c:enumerator:`SENSOR_CHAN_S3KM1110_MOVING_DISTANCE` and
  :c:enumerator:`SENSOR_CHAN_S3KM1110_STATIC_DISTANCE` custom channels respectively.

- Energy of moving and static targets using the :c:enumerator:`SENSOR_CHAN_S3KM1110_MOVING_ENERGY`
  and :c:enumerator:`SENSOR_CHAN_S3KM1110_STATIC_ENERGY` custom channels respectively.

Building and Running
********************

Build for a XIAO board with the :ref:`seeed_xiao_hsp24`:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/s3km1110
   :board: xiao_esp32c6/esp32c6/hpcore
   :shield: seeed_xiao_hsp24
   :goals: build flash

Sample Output
=============

.. code-block:: console

   mmWave Sensor Report
   ====================

     Presence            occupied
     Target              Static target
     Distances
     *********

     Detection           0.390 m
     Moving target       0.630 m
     Static target       0.630 m

     Energies
     ********

     Moving              0%
     Static              80%
