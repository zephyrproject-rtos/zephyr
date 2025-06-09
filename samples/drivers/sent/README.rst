.. zephyr:code-sample:: sent
   :name: SENT interface
   :relevant-api: sent_interface

   Use the Single Edge Nibble Transmission (SENT) driver.

Overview
********

The sample application shows how to use the Single Edge Nibble Transmission (SENT) driver:

* Receive data

Requirements
************

This sample requires a SENT sensor to be connected.

Building, Flashing and Running
******************************

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/sent
   :board: s32z2xxdc2/s32z270/rtu0
   :goals: build flash

Sample Output:

.. code-block:: console

   Received a frame on channel 1
