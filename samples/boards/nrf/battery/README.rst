.. _battery:

Battery voltage measurement
###########################

Description
***********

This sample application periodically measures the battery voltage
and prints it on console.

Depending on hardware app provides two modes of working:
- if proper hardware is in place usage of voltage divider is preffered
- if no divider is present, direct measurement on voltage on MCU
Mode can be selected by menuconfig, please refer to ``Kconfig`` file.

Wiring
*******

If measurement with voltage divider is selected then depending on hardware,
it might not need any extra wiring (eg nrf52_pca20020) or configuration
it might require to deliver proper voltage divider configuration through
device tree.

If measurement is done direct on MCU no other wiring is needed.


Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nrf/battery
   :board: nrf52840_pca10056 nrf52_pca20020
   :goals: build flash

Sample Output
=============

.. code-block:: console

   *** Booting Zephyr OS build zephyr-v2.1.0-1666-g641670324ae6  ***
   [0:00:00.019]: 2996 mV; 0 pptt
   [0:00:05.008]: 2997 mV; 0 pptt
   [0:00:09.997]: 2996 mV; 0 pptt
   [0:00:14.987]: 2997 mV; 0 pptt
   [0:00:19.977]: 2997 mV; 0 pptt
   [0:00:24.967]: 2996 mV; 0 pptt

<repeats endlessly>
