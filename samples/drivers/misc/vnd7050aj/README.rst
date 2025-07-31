.. _vnd7050aj_sample:

VND7050AJ High-Side Driver Sample
#################################

Overview
********

This sample demonstrates how to use the VND7050AJ high-side driver with Zephyr.
It shows how to control the output channels and read diagnostic information like
load current, supply voltage, and chip temperature.

Requirements
************

- A board with a VND7050AJ high-side driver connected.
- The necessary GPIOs and ADC channel must be configured in the device tree.

Building and Running
********************

This sample can be built and flashed to a board using the following commands:

.. code-block:: console

   west build -b <your_board> samples/drivers/misc/vnd7050aj
   west flash

Sample Output
*************

.. code-block:: console

   *** VND7050AJ Sample ***
   Channel 0 ON
   Load current on channel 0: 123 mA
   Channel 0 OFF
   Channel 1 ON
   Load current on channel 1: 456 mA
   Channel 1 OFF
   Supply voltage: 12000 mV
   Chip temperature: 25 C
