.. zephyr:code-sample:: analog_switch
   :name: Analog Switch
   :relevant-api: analog_switch_interface

   Control analog switch channels using the analog switch driver API.

Overview
********

This sample demonstrates how to use the :ref:`analog switch driver API
<analog_switch_api>`.

The sample toggles each channel individually, sets a selective pattern
via the bitmask API, and resets all channels.

Requirements
************

The board's devicetree must define an ``analog-switch0`` alias pointing
to an analog switch device node, or use a shield that provides one
(e.g. :ref:`eval_adgm3121`).

Building and Running
********************

Using the EVAL-ADGM3121 shield with any board that has an Arduino R3 header:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/analog_switch
   :board: nucleo_f413zh
   :shield: eval_adgm3121
   :goals: build flash
   :compact:

Sample output
=============

.. code-block:: console

   Analog Switch Sample
   Device ready: adgm3121@0
   Number of channels: 4

   --- Individual channel control ---
   Channel 0: ON
   Channel 1: ON
   Channel 2: ON
   Channel 3: ON

   --- Bitmask control ---
   Channel states after setting ch0+ch2: 0x05
     Channel 0: ON
     Channel 1: OFF
     Channel 2: ON
     Channel 3: OFF

   --- Reset ---
   Channel states after reset: 0x00

   Analog switch sample completed
