.. _rtk7eka6m3b00001bu:

RTK7EKA6M3B00001BU Display
##########################

Overview
********

The Graphics Expansion Board includes a 4.3-inch 480x272 pixel TFT color LCD with a
capacitive touch overlay.

This display uses a 40-pin connector header.

Pins Assignment of the Renesas RTK7EKA6M3B00001BU Display
=========================================================

+-----------------+--------------------------+
| Connector Pin   | Function                 |
+=================+==========================+
| 1               | Display backlight enable |
+-----------------+--------------------------+
| 2               | Touch ctrl I2C SDA       |
+-----------------+--------------------------+
| 3               | External interrupt       |
+-----------------+--------------------------+
| 4               | Touch ctrl I2C SCL       |
+-----------------+--------------------------+
| 6               | Display reset            |
+-----------------+--------------------------+

Hardware Requirements:
**********************

Supported Renesas RA boards: EK-RA8D1

- 1 x RA Board
- 1 x Micro USB cable

Programming
***********

Set ``--shield=rtk7eka6m3b00001bu`` when you invoke ``west build``. For
example:

.. zephyr-app-commands::
   :zephyr-app: tests/drivers/display/display_read_write
   :board: ek_ra8d1
   :shield: rtk7eka6m3b00001bu
   :goals: build
