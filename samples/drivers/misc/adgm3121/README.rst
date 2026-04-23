.. zephyr:code-sample:: adgm3121
   :name: ADGM3121 MEMS Switch

   Control an ADGM3121 DPDT MEMS RF switch.

Overview
********

This sample demonstrates the ADGM3121 DPDT MEMS RF switch driver API.
It toggles each switch individually, sets multiple switches via bitmask,
and resets all switches. The driver supports both SPI and parallel GPIO
control modes.

Requirements
************

A board with an ADGM3121 device configured in devicetree, or use
the :ref:`eval_adgm3121` shield with any board that has an Arduino R3
header.

Building and Running
********************

Using the EVAL-ADGM3121 shield:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/misc/adgm3121
   :board: adi_sdp_k1
   :shield: eval_adgm3121
   :goals: build flash

Sample Output
*************

.. code-block:: console

   ADGM3121 MEMS Switch Sample
   Device ready

   --- Individual switch control ---
   SW1: ON
   SW2: ON
   SW3: ON
   SW4: ON

   --- Bitmask control ---
   Switch mask after setting SW1+SW3: 0x05
     SW1: ON
     SW2: OFF
     SW3: ON
     SW4: OFF

   --- Reset ---
   Switch mask after reset: 0x00

   ADGM3121 sample completed
