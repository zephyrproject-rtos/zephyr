.. _waveshare_dsi_lcd:

WAVESHARE DSI LCD Panel
########################

Overview
********

The WAVESHARE DSI LCD Panel Shield is a universal driver shield.
The shield can be used to drive various DSI LCD panel.

More information about the shield can be found
-at the `WAVESHARE DSI LCD Panel website`_.

Current supported displays
==========================

+--------------+------------------------------+
| Display      | Shield Designation           |
|              |                              |
+==============+==============================+
|  7inch DSI   | waveshare_7inch_dsi_lcd_c    |
|   LCD(C)     |                              |
+--------------+------------------------------+


Requirements
************

This shield can only be used with a board that provides a configuration
for MIPI DSI and defines node aliases for I2C interfaces
(see :ref:`shields` for more details).

Programming
***********

Correct shield designation (see the table above) for your display must
be entered when you invoke ``west build``.
For example:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/display/lvgl
   :board: frdm_imx93/mimx9352/a55
   :shield: waveshare_7inch_dsi_lcd_c
   :goals: build

References
**********

.. target-notes::

.. _WAVESHARE DSI LCD Panel website:
   https://www.waveshare.com/product/displays/lcd-oled/7inch-dsi-lcd-c.htm
