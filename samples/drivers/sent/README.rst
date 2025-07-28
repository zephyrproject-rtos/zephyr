.. zephyr:code-sample:: sent
   :name: SENT interface
   :relevant-api: sent_interface

   Use the SENT (Single Edge Nibble Transmission) driver.

Overview
********

The sample application shows how to use the :ref:`SENT API <sent_api>`:

* Receive data

Requirements
************

This sample requires a SENT sensor to be connected and exposed as ``sent0`` Devicetree alias.

Building, Flashing and Running
******************************

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/sent
   :board: s32z2xxdc2/s32z270/rtu0
   :goals: build flash

Sample Output:

.. code-block:: console

   Received a frame on channel 1
