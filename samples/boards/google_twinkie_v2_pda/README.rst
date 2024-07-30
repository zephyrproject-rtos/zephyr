.. _google_twinkie_v2_pda:

Twinkie Power Delivery
######################

Overview
********

This provides access to :ref:`Twinkie <google_twinkie_v2_board>` so you can try out
the supported features.

Building and Running
********************

Build and flash Twinkie as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/google_pda
   :board: google_twinkie_v2
   :goals: build flash
   :compact:

After flashing, the LED will start red. Putting the Twinkie in between any
usbc connection will cause the LED to turn blue. The LED will turn green instead
if the device is currently charging.
