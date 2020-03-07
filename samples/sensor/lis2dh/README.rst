.. _lis2dh:

LIS2DH: Motion Sensor Monitor
#############################

Overview
********

This sample application periodically reads accelerometer data from the
LIS2DH sensor (or the compatible LS2DH12, LIS3DH, and LSM303DLHC
sensors), and displays the sensor data on the console.

Requirements
************

This sample uses the LIS2DH, ST MEMS system-in-package featuring a 3D
digital output motion sensor.

References
**********

For more information about the LIS2DH motion sensor see
http://www.st.com/en/mems-and-sensors/lis2dh.html.

Building and Running
********************

The LIS2DH2 or compatible sensors are available on a variety of boards
and shields supported by Zephyr, including:

* :ref:`actinius_icarus`
* :ref:`nrf52_pca20020`
* :ref:`stm32f3_disco_board`
* :ref:`x-nucleo-iks01a2`

See the board documentation for detailed instructions on how to flash
and get access to the console where acceleration data is displayed.

Building on actinius_icarus
===========================

:ref:`actinius_icarus` includes an ST LIS2DH12 accelerometer which
supports the LIS2DH interface.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/lis2dh
   :board: actinius_icarus
   :goals: build flash
   :compact:

Building on nucleo_l476rg with IKS01A2 shield
=============================================

The :ref:`x-nucleo-iks01a2` includes an LSM303AGR accelerometer which
supports the LIS2DH interface.  This shield may also be used on other
boards with Arduino headers.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/lis2dh
   :board: nucleo_l476rg
   :goals: build flash
   :shield: x_nucleo_iks01a2
   :compact:

Sample Output
=============

.. code-block:: console

    Polling at 0.5 Hz
    #1 @ 12 ms: x -5.387328 , y 5.578368 , z -5.463744
    #2 @ 2017 ms: x -5.310912 , y 5.654784 , z -5.501952
    #3 @ 4022 ms: x -5.349120 , y 5.692992 , z -5.463744

   <repeats endlessly every 2 seconds>
