.. zephyr:code-sample:: psi5
   :name: PSI5 interface
   :relevant-api: psi5_interface

   Use the PSI5 (Peripheral Sensor Interface) driver.

Overview
********

The sample application shows how to use the :ref:`PSI5 API <psi5_api>`:

* Receive data
* Transmit data

Requirements
************

This sample requires a PSI5 sensor to be connected and exposed as ``psi5-0`` Devicetree alias.

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
