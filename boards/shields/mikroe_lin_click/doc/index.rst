.. _mikroe_lin_click_shield:

MikroElektronika LIN Click
##########################

Overview
--------

The LIN Click shield features the `TLE7259-3GE`_, a LIN transceiver from Infineon Technologies, with
integrated wake-up and protection features.

The LIN click can be configured for commander node applications or for responder node applications
by setting the appropriate mode selection register on the board.

More information about the shield can be found at `Mikroe LIN click`_.

Requirements
************

The shield uses a mikroBUS interface. The target board must define the ``mikrobus_header`` and
``mikrobus_lin`` node labels for GPIOs access and LIN interface
(see :ref:`shields` for more details).

Programming
***********

Set ``--shield mikroe_lin_click`` when you invoke ``west build``, for example:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/lin/ncv7430
   :board: ek_ra8m1
   :shield: mikroe_lin_click
   :goals: build flash

References
**********

.. target-notes::

.. _Mikroe LIN click:
   https://www.mikroe.com/lin-click

.. _TLE7259-3GE:
   https://www.infineon.com/part/TLE7259-3GE
