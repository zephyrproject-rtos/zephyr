.. zephyr:code-sample:: saej2716
   :name: SAEJ2716 interface
   :relevant-api: saej2716_interface

   Use the SAEJ2716 Single Edge Nibble Transmission (SENT) driver.

Overview
********

The sample application shows how to use the SAEJ2716 Single Edge Nibble Transmission (SENT) driver:

* Receive data

Requirements
************

This sample requires a SENT sensor to be connected.

Building, Flashing and Running
******************************

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/saej2716
   :board: s32z2xxdc2/s32z270/rtu0
   :goals: build flash

Sample Output:

.. code-block:: console

   Received a frame on channel 1
