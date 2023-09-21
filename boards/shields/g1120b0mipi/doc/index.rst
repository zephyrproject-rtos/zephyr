.. _g1120b0mipi:

G1120B0MIPI MIPI Display
##########################

Overview
********

The G1120B0MIPI is a 1.2 inch circular AMOLED display, 390x390 pixels, with a
1-lane MIPI interface. This display connects to the i.MX RT595 Evaluation Kit.


More information about the shield can be found
at the `G1120B0MIPI product page`_.

This display uses a 40 pin FPC interface, which is available on many
NXP EVKs.

Pins Assignment of the G1120B0MIPI MIPI Display
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

Set ``-DSHIELD=g1120b0mipi`` when you invoke ``west build``. For
example:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/display
   :board: mimxrt595_evk_cm33
   :shield: g1120b0mipi
   :goals: build

References
**********

.. target-notes::

.. _G1120B0MIPI product page:
   https://www.nxp.com/design/development-boards/i-mx-evaluation-and-development-boards/1-2-wearable-display-g1120b0mipi:G1120B0MIPI
