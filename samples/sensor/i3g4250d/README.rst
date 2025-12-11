.. zephyr:code-sample:: i3g4250d
   :name: I3G4250D 3-axis gyroscope sensor
   :relevant-api: sensor_interface

   Get gyroscope data from an I3G4250D sensor.

Description
***********

This sample application configures the gyroscope with a fixed
sampling rate and polls the measurements with the same rate and
displays the measurements along with a timestamp since startup.

Requirements
************

This sample application uses the I3G4250D sensor connected via
SPI interface. This is the case for example for stm32f3_disco@E
board devicetree.

Building and Running
********************

To build the application a board with I3G4250D on SPI interface
has to be chosen, or a custom devicetree overlay has to be provided.
Here STM32F3 discovery board is used.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/i3g4250d
   :board: stm32f3_disco@E
   :goals: build flash

Sample Output
*************

.. code-block:: console

   *** Booting Zephyr OS build zephyr-v2.6.0-1897-ga09838064f26  ***
   Set sensor sampling frequency to 200.000000 Hz.
   Polling at 200.000000 Hz
   12 ms: x 2454.031430 , y 2336.015410 , z -1800.030108
   22 ms: x -248.003106 , y -268.979704 , z 6.018390
   32 ms: x -214.989906 , y -237.023468 , z 6.013926
   41 ms: x -193.978308 , y -205.032232 , z 2.000356
   51 ms: x -158.986716 , y -171.014568 , z 3.969998
   60 ms: x -138.979582 , y -153.003326 , z 3.978748
   70 ms: x -120.981554 , y -129.982800 , z -13.971422
   80 ms: x -106.984060 , y -112.967272 , z -80.006572
