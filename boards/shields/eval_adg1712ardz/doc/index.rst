.. _eval_adg1712ardz:

EVAL-ADG1712ARDZ
################

Overview
********

The `EVAL-ADG1712ARDZ`_ is an evaluation board for the `Analog Devices ADG1712`_
quad SPST analog switch with an Arduino-compatible header.

Each of the four SPST switches (SW1-SW4) is independently controlled by a
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
| D4           | ADG1712 IN1 (SW1 control)    |
+--------------+------------------------------+
| D5           | ADG1712 IN2 (SW2 control)    |
+--------------+------------------------------+
| D6           | ADG1712 IN3 (SW3 control)    |
+--------------+------------------------------+
| D7           | ADG1712 IN4 (SW4 control)    |
+--------------+------------------------------+

Programming
***********

Set ``--shield eval_adg1712ardz`` when you invoke ``west build``. For example
when running the :zephyr:code-sample:`adg1712` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/misc/adg1712
   :board: adi_sdp_k1
   :shield: eval_adg1712ardz
   :goals: build flash

.. _EVAL-ADG1712ARDZ:
   https://www.analog.com/en/resources/evaluation-hardware-and-software/evaluation-boards-kits/eval-adg1712.html

.. _Analog Devices ADG1712:
   https://www.analog.com/en/products/adg1712.html
