.. _eval_adg1736sdz:

EVAL-ADG1736SDZ
###############

Overview
********

The `EVAL-ADG1736SDZ`_ is an evaluation board for the Analog Devices
`ADG1736`_ dual SPDT (Single Pole, Double Throw) analog switch. The
ADG1736 contains two independently selectable switches, each routing a
signal to one of two paths (A or B) via dedicated GPIO control pins.
An optional enable pin allows the device to be globally enabled or
disabled.

Requirements
************

This shield can only be used with a board that provides a configuration
for Arduino connectors (see :ref:`shields` for more details).

Pin Assignments
===============

+--------------+------------------------------+
| Shield Pin   | Function                     |
+==============+==============================+
| D4           | IN1 (SW1 path select)        |
+--------------+------------------------------+
| D5           | IN2 (SW2 path select)        |
+--------------+------------------------------+
| D6           | EN (device enable)           |
+--------------+------------------------------+

Programming
***********

Set ``--shield eval_adg1736sdz`` when you invoke ``west build``. For example
when running the :zephyr:code-sample:`adg1736` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/misc/adg1736
   :board: adi_sdp_k1
   :shield: eval_adg1736sdz
   :goals: build flash

.. _EVAL-ADG1736SDZ:
   https://www.analog.com/en/resources/evaluation-hardware-and-software/evaluation-boards-kits/eval-adg1736sdz.html

.. _ADG1736:
   https://www.analog.com/en/products/adg1736.html
