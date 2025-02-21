.. zephyr:code-sample:: sensor_batch_processing
   :name: Sensor batch processing
   :relevant-api: rtio

   Implement a sensor device using RTIO for asynchronous data processing.

Overview
********

This sample application demonstrates the use of the :ref:`rtio` framework for
doing asynchronous operation chains.
Application uses :ref:`rtio` with mempool API to fetch data from virtual sensor
and displays it on the console.

Requirements
************

* A board with flash support or native_sim target

Building and Running
********************

This sample can be found under :zephyr_file:`samples/subsys/rtio` in the Zephyr tree.

The sample can be built for most platforms, the following commands build the
application for the native_sim target.

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/rtio
   :board: native_sim
   :goals: build run
   :compact:

When running, the output on the console shows the operations of
submitting for, consuming, and processing the sensor data.

Sample Output
=============

.. code-block:: console

   *** Booting Zephyr OS build v4.0.0-1260-gbaa49f6f32d5 ***
   I: Submitting 4 read requests
   D: sensor@0: buf_len = 16, buf = 0x8056430
   D: sensor@0: buf_len = 16, buf = 0x8056440
   D: sensor@0: buf_len = 16, buf = 0x8056450
   D: sensor@0: buf_len = 16, buf = 0x8056460
   D: Consumed completion event 0
   D: Consumed completion event 1
   D: Consumed completion event 2
   D: Consumed completion event 3
   I: Start processing 4 samples
   D: Sample data:
   D: 00 01 02 03 04 05 06 07 |........
   D: 08 09 0a 0b 0c 0d 0e 0f |........
   D: Sample data:
   D: 10 11 12 13 14 15 16 17 |........
   D: 18 19 1a 1b 1c 1d 1e 1f |........
   D: Sample data:
   D: 20 21 22 23 24 25 26 27 | !"#$%&'
   D: 28 29 2a 2b 2c 2d 2e 2f |()*+,-./
   D: Sample data:
   D: 30 31 32 33 34 35 36 37 |01234567
   D: 38 39 3a 3b 3c 3d 3e 3f |89:;<=>?
   D: sensor@0: buf_len = 16, buf = 0x8056470
   D: sensor@0: buf_len = 16, buf = 0x8056480
   D: sensor@0: buf_len = 16, buf = 0x8056490
   I: Finished processing 4 samples
   I: Submitting 4 read requests
   D: sensor@0: buf_len = 16, buf = 0x8056430
   D: sensor@0: buf_len = 16, buf = 0x8056440
   D: sensor@0: buf_len = 16, buf = 0x8056450
   D: sensor@0: buf_len = 16, buf = 0x8056460
   D: Consumed completion event 0
   ...
