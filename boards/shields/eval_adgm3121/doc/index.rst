.. _eval_adgm3121:

EVAL-ADGM3121
#############

Overview
********

The `EVAL-ADGM3121`_ is an evaluation board for the `Analog Devices ADGM3121`_
DPDT MEMS RF switch (DC-24GHz) with an Arduino-compatible header.

Each of the four MEMS switches (SW1-SW4) is independently controlled by a
dedicated digital GPIO pin (IN1-IN4).

Requirements
************

This shield requires a board with an Arduino R3 header connector.
See :ref:`shields` for more details.

Pin Assignments
===============

+--------------+------------------------------+
| Shield Pin   | Function                     |
+==============+==============================+
| D4           | ADGM3121 IN1 (SW1 control)   |
+--------------+------------------------------+
| D5           | ADGM3121 IN2 (SW2 control)   |
+--------------+------------------------------+
| D6           | ADGM3121 IN3 (SW3 control)   |
+--------------+------------------------------+
| D7           | ADGM3121 IN4 (SW4 control)   |
+--------------+------------------------------+

Programming
***********

Set ``--shield eval_adgm3121`` when you invoke ``west build``. For example
when running the :zephyr:code-sample:`adgm3121` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/misc/adgm3121
   :board: adi_sdp_k1
   :shield: eval_adgm3121
   :goals: build flash

.. _EVAL-ADGM3121:
   https://www.analog.com/en/resources/evaluation-hardware-and-software/evaluation-boards-kits/eval-adgm3121.html

.. _Analog Devices ADGM3121:
   https://www.analog.com/en/products/adgm3121.html
