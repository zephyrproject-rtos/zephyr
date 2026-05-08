.. zephyr:code-sample:: adg1736
   :name: ADG1736 Dual SPDT Analog Switch

   Control an ADG736/ADG1736 dual SPDT analog switch.

Overview
********

This sample demonstrates the ADG1736 dual SPDT analog switch driver API.
It exercises individual switch control (toggling between path A and
path B), enable/disable functionality for variants that support the EN
pin, and reset of all switches to the default state.

Requirements
************

A board with an ADG736 or ADG1736 device configured in devicetree, or use
the :ref:`eval_adg1736sdz` shield with any board that has an Arduino R3
header.

Building and Running
********************

Using the EVAL-ADG1736SDZ shield:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/misc/adg1736
   :board: adi_sdp_k1
   :shield: eval_adg1736sdz
   :goals: build flash
   :compact:

Sample Output
*************

.. code-block:: console

   ADG1736 Dual SPDT Analog Switch Sample
   Device ready

   --- Individual switch control ---
   SW1: path A
   SW1: path B
   SW2: path A
   SW2: path B

   --- Enable/Disable control ---
   Device enabled
   Device disabled
   Device re-enabled

   --- Reset ---
   SW1: path B
   SW2: path B

   ADG1736 sample completed
