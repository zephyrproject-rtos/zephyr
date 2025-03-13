.. _rk055hdmipi4m:

NXP RK055HDMIPI4M MIPI Display
##############################

Overview
********

The Rocktech RK055HDMIPI4M MIPI Display is a 5.5 inch TFT 720x1280 pixels
panel with LED backlighting, full viewing angle, MIPI interface and
capacitive touch panel from Rocktech.

More information about the shield can be found
at the `RK055HDMIPI4M product page`_.

This display uses a 40 pin FPC interface, which is available on many
NXP EVKs.

Pins Assignment of the Rocktech RK055HDMIPI4M MIPI Display
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

Set ``--shield rk055hdmipi4m`` when you invoke ``west build``. For
example:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/display
   :board: mixmrt1170_evk_cm7
   :shield: rk055hdmipi4m
   :goals: build

.. include:: ../../../nxp/common/board-footer.rst
   :start-after: nxp-board-footer

References
**********

.. target-notes::

.. _RK055HDMIPI4M product page:
   https://www.nxp.com/design/development-boards/i-mx-evaluation-and-development-boards/5-5-lcd-panel:RK055HDMIPI4M
