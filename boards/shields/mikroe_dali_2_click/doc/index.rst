.. _mikroe_dali_2_click_shield:

MikroElektronika DALI 2 Click
#############################

Overview
********

The DALI 2 Click shield provides a physical interface to a DALI bus.
The DALI bus interface uses separate Rx and Tx signals to communicate with the DALI bus.
The board uses optocouplers to isolate the DALI bus from the host board.

More information about the shield can be found at
`Mikroe DALI 2 click`_.

.. figure:: images/dali_2_click.webp
   :align: center
   :height: 300px
   :alt: MikroElektronika DALI 2 Click

   MikroElektronika DALI 2 Click

Requirements
************

The shield uses a mikroBUS interface.
The target board must define ``mikrobus_header`` node labels
(see :ref:`shields` for more details).

Programming
***********

Set ``--shield mikroe_dali_2_click`` when you invoke ``west build``. For example:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/dali/
   :board: <board>
   :shield: mikroe_dali_2_click
   :goals: build flash

References
**********

.. target-notes::

.. _Mikroe DALI 2 click:
   https://www.mikroe.com/dali-2-click
