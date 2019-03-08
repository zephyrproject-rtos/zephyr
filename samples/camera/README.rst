.. _ucam-camera-sample:

uCAM-III Camera Application
###########################

Overview
********

This camera application is a Zephyr port of the `4D Systems uCAM-III camera`_
module. The camera API is defined in :zephyr_file:`samples/camera/src/ucam3.h`.

.. _4D Systems uCAM-III camera:
   https://www.4dsystems.com.au/product/uCAM_III/

Requirements
************

The demo assumes that a uCAM-III is connected to the UART_2 TX/RX of a Nucleo
L496ZG. Also, the camera module should be powered by the 5V pin.

Building and Running
********************

This sample takes a series of snapshots and outputs their size to the
console. It can be built and flashed to a board as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/camera
   :board: nucleo_l496zg
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

    1/10: Took snapshot of 8826 bytes.
    2/10: Took snapshot of 8804 bytes.
    3/10: Took snapshot of 8900 bytes.
    4/10: Took snapshot of 8924 bytes.
    5/10: Took snapshot of 8846 bytes.
    6/10: Took snapshot of 8912 bytes.
    7/10: Took snapshot of 8862 bytes.
    8/10: Took snapshot of 8782 bytes.
    9/10: Took snapshot of 8884 bytes.
    10/10: Took snapshot of 8834 bytes.

