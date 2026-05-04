.. zephyr:code-sample:: adg1712
   :name: ADG1712 Quad SPST Analog Switch

   Control the ADG1712/ADG2712 quad SPST analog switch.

Overview
********

This sample demonstrates the ADG1712/ADG2712 quad SPST analog switch driver
API. It toggles each switch individually, sets multiple switches via bitmask,
and resets all switches.

Requirements
************

A board with an ADG1712 or ADG2712 device configured in devicetree, or use
the :ref:`eval_adg1712ardz` shield with any board that has an Arduino R3
header.

Building and Running
********************

Using the EVAL-ADG1712ARDZ shield:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/misc/adg1712
   :board: adi_sdp_k1
   :shield: eval_adg1712ardz
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

   ADG1712 Quad SPST Analog Switch Sample
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

   ADG1712 sample completed
