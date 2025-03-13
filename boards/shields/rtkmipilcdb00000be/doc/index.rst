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

+-----------------------+------------------------+
| Connector Pin        | Function                |
+=======================+========================+
| 14                   | Touch ctrl I2C SDA      |
+-----------------------+------------------------+
| 15                   | Display backlight enable|
+-----------------------+------------------------+
| 16                   | Touch ctrl I2C SCL      |
+-----------------------+------------------------+
| 17                   | External interrupt      |
+-----------------------+------------------------+
| 18                   | Display reset           |
+-----------------------+------------------------+

Hardware Requirements:
**********************

Supported Renesas RA boards: EK-RA8D1
- 1 x RA Board
- 1 x Micro USB cable

Hardware Configuration:
***********************

The MIPI Graphics Expansion Port (J58) connects the EK-RA8D1 board to the MIPI Graphics Expansion Board
supplied as part of the kit.

Set the configuration switches (SW1) as below to avoid potential failures.
		+-------------+-------------+--------------+------------+------------+------------+-------------+-----------+
		| SW1-1 PMOD1 | SW1-2 TRACE | SW1-3 CAMERA | SW1-4 ETHA | SW1-5 ETHB | SW1-6 GLCD | SW1-7 SDRAM | SW1-8 I3C |
		+-------------+-------------+--------------+------------+------------+------------+-------------+-----------+
		|     OFF     |     OFF     |      OFF     |     OFF    |     OFF    |     ON     |     ON      |    OFF    |
		+-------------+-------------+--------------+------------+------------+------------+-------------+-----------+

Programming
***********

Set ``--shield=rtkmipilcdb00000be`` when you invoke ``west build``. For
example:

.. zephyr-app-commands::
   :zephyr-app: tests/drivers/display/display_read_write
   :board: ek_ra8d1
   :shield: rtkmipilcdb00000be
   :goals: build
