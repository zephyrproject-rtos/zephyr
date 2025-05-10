.. zephyr:code-sample:: psi5
   :name: PSI5 interface
   :relevant-api: psi5_interface

   Use the Peripheral Sensor Interface (PSI5) driver.

Overview
********

The sample application shows how to use the Peripheral Sensor Interface (PSI5) driver:

* Receive data
* Transmit data

Requirements
************

This sample requires a PSI5 sensor to be connected.

Building, Flashing and Running
******************************

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/psi5
   :board: s32z2xxdc2/s32z270/rtu0
   :goals: build flash

Sample Output:

.. code-block:: console

   Transmitted data on channel 1

   Received a frame on channel 1
