.. _zc143ac72mipi:

NXP ZC143AC72MIPI MIPI Display
##############################

Overview
********

The ZC143AC72MIPI is a 1.43 inch Circular AMOLED display, 466x466 pixels, with a
1-lane MIPI interface. This display connects to the i.MX RT700 Evaluation Kit.

More information about the shield can be found
at the `ZC143AC72MIPI product page`_.

This display uses a 40 pin FPC interface, which is available on many
NXP EVKs.

Pins Assignment of the ZC143AC72MIPI MIPI Display
==========================================================

+-----------------------+------------------------+
| FPC Connector Pin     | Function               |
+=======================+========================+
| 1                     | LED backlight cathode  |
+-----------------------+------------------------+
| 21                    | Controller reset       |
+-----------------------+------------------------+
| 22                    | Controller LPTE        |
+-----------------------+------------------------+
| 26                    | Touch ctrl I2C SDA     |
+-----------------------+------------------------+
| 27                    | Touch ctrl I2C SCL     |
+-----------------------+------------------------+
| 28                    | Touch ctrl reset       |
+-----------------------+------------------------+
| 29                    | Touch ctrl interrupt   |
+-----------------------+------------------------+
| 32                    | LCD power enable       |
+-----------------------+------------------------+
| 34                    | Backlight power enable |
+-----------------------+------------------------+

Requirements
************

This shield can only be used with a board which provides a configuration
for the 40 pin FPC interface

Programming
***********

Set ``--shield zc143ac72mipi`` when you invoke ``west build``. For
example:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/display
   :board: mimxrt700_evk/mimxrt798s/cm33_cpu0
   :shield: zc143ac72mipi
   :goals: build

.. include:: ../../../nxp/common/board-footer.rst.inc


References
**********

.. target-notes::

.. _ZC143AC72MIPI product page:
   https://www.nxp.com/design/design-center/development-boards-and-designs/1-43-wearable-display-zc143ac72mipi:ZC143AC72MIPI
