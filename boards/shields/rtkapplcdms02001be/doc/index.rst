.. _rtkapplcdms02001be:

RTKAPPLCDMS02001BE MIPI Display
###############################

Overview
********

The Focus LCDs RTKAPPLCDMS02001BE MIPI Display is a 5 inch TFT 1280x720 pixels
capacitive touch panel.

This display uses a 40 pin connector header.

Pins Assignment of the Renesas RTKAPPLCDMS02001BE MIPI Display
==============================================================

+----------------------+-------------------------+
| Connector Pin        | Function                |
+======================+=========================+
| 20                   | Touch ctrl I2C SDA      |
+----------------------+-------------------------+
| 21                   | Display backlight enable|
+----------------------+-------------------------+
| 22                   | Touch ctrl I2C SCL      |
+----------------------+-------------------------+
| 23                   | External interrupt      |
+----------------------+-------------------------+
| 24                   | Display reset           |
+----------------------+-------------------------+

Hardware Requirements:
**********************

Supported Renesas RZ boards: EK-RZ/A3M
- 1 x RZ Board
- 1 x USB-C cable

Programming
***********

Set ``--shield=rtkapplcdms02001be`` when you invoke ``west build``. For
example:

.. zephyr-app-commands::
   :zephyr-app: tests/drivers/display/display_read_write
   :board: rza3m_ek
   :shield: rtkapplcdms02001be
   :goals: build
