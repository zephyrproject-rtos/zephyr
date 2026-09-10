.. zephyr:code-sample:: twinkie_v2_pda
   :name: Power Delivery Analyzer
   :relevant-api: adc_interface

   Implement a basic Power Delivery Analyzer to determine if a USB device is currently charging.

Overview
********

This provides access to :zephyr:board:`google_twinkie_v2` so you can try out
the supported features.

Building and Running
********************

Build and flash Twinkie as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/google/twinkie_v2/pda
   :board: google_twinkie_v2
   :goals: build flash
   :compact:

After flashing, the LED will start red. Putting the Twinkie in between any
usbc connection will cause the LED to turn blue. The LED will turn green instead
if the device is currently charging.
