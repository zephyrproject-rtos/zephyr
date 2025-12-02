.. _apollo5_eb_display_8080_card:

Ambiq Apollo510 Display Add-on Board
####################################

Overview
********

Display shield add-on board for AP510 4" round 480*800 Pixels touchscreen
with MIPI-DBI Type-B interface.

.. note::
   The shield apollo5_eb_display_8080_card is utilizing a NT35510 panel
   controller and shall specifically use ``apollo5_eb_display_8080_card``
   as SHIELD

Requirements
************

Your board needs to have a ``mipi_dbi`` device tree label to work with
this shield.

Usage
*****

The shield can be used in any application by setting ``SHIELD`` to
``apollo5_eb_display_8080_card`` and adding the necessary device tree
properties.

Set ``--shield apollo5_eb_display_8080_card`` when you invoke ``west build``.
For example:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/display
   :board: apollo510_eb
   :shield: apollo5_eb_display_8080_card
   :goals: build

The user can notice a pure color bar at the top of the display. Its color
changes over time.

References
**********

The Apollo510 Engineering Board & its components are not sold, you can contact
us to inquire about replacement boards.

- `Product page <https://support.ambiq.com/hc/en-us>`_
