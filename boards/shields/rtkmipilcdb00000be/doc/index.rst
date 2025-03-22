.. _rtkmipilcdb00000be:

RTKMIPILCDB00000BE MIPI Display
###############################

Overview
********

The Focus LCDs RTKMIPILCDB00000BE MIPI Display is a 4.5 inch TFT 480x854 pixels
capacitive touch panel, and a backlight unit.

This display uses a 26 pin connector header.

Pins Assignment of the Renesas RTKMIPILCDB00000BE MIPI Display
==============================================================

+----------------------+-------------------------+
| Connector Pin        | Function                |
+======================+=========================+
| 14                   | Touch ctrl I2C SDA      |
+----------------------+-------------------------+
| 15                   | Display backlight enable|
+----------------------+-------------------------+
| 16                   | Touch ctrl I2C SCL      |
+----------------------+-------------------------+
| 17                   | External interrupt      |
+----------------------+-------------------------+
| 18                   | Display reset           |
+----------------------+-------------------------+

Hardware Requirements:
**********************

Supported Renesas RA boards: EK-RA8D1
- 1 x RA Board
- 1 x Micro USB cable

Programming
***********

Set ``--shield=rtkmipilcdb00000be`` when you invoke ``west build``. For
example:

.. zephyr-app-commands::
   :zephyr-app: tests/drivers/display/display_read_write
   :board: ek_ra8d1
   :shield: rtkmipilcdb00000be
   :goals: build
