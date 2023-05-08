.. _lorawan_class_a_sample:

LoRaWAN Class A Sample
######################

Overview
********

A simple application to demonstrate the LoRaWAN subsystem of Zephyr.

Building and Running
********************

This sample can be found under
:zephyr_file:`samples/subsys/lorawan/class_a` in the Zephyr tree.

Before building the sample, make sure to select the correct region in the
``prj.conf`` file.

The following commands build and flash the sample.

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/lorawan/class_a
   :board: nucleo_wl55jc
   :goals: build flash
   :compact:

Extended Configuration
**********************

This sample can be configured to run the application-layer clock
synchronization service in the background.

The following commands build and flash the sample with clock synchronization
enabled.

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/lorawan/class_a
   :board: nucleo_wl55jc
   :goals: build flash
   :gen-args: -DOVERLAY_CONFIG=overlay-clock-sync.conf
   :compact:
