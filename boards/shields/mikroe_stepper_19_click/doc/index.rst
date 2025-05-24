.. _mikroe_stepper_19_click_shield:

MikroElektronika Stepper 19 Click shield
########################################

Overview
--------

Stepper 19 Click shield has a TI DRV8424 stepper driver accessed via gpio and
a TI TCA9538 gpio expander accessed via i2c. Some DRV8424 pins are accessed
via the gpio expander.
The DRV8424 uses by default the work-queue timing source, but that can be changed.

More information about the shield can be found at
`Mikroe Stepper 19 click`_.

Requirements
************

The shield uses a mikroBUS interface. The target board must define
a ``mikrobus_i2c`` and ``mikrobus_header``  node labels
(see :ref:`shields` for more details).

Programming
***********

Set ``--shield mikroe_stepper_19_click`` when you invoke ``west build``.

References
**********

.. target-notes::

.. _Mikroe Stepper 19 click:
   https://www.mikroe.com/stepper-19-click
