.. _renesas_aik_ov2640_cam:

Renesas AIK OV2640 Camera Shield
################################

Overview
********

The Renesas AIK OV2640 camera shield provides the OV2640 image sensor on a 20-pin DVP connector.
It is designed specifically for the Renesas AI Kit Development Platform.

Pins assignment
===============

+-----+-------------+
| Pin | Function    |
+=====+=============+
|  1  | GND         |
+-----+-------------+
|  2  | VCC         |
+-----+-------------+
|  3  | SCL         |
+-----+-------------+
|  4  | SDA         |
+-----+-------------+
|  5  | HSYNC       |
+-----+-------------+
|  6  | VSYNC       |
+-----+-------------+
|  7  | PCLK        |
+-----+-------------+
|  8  | XCLK        |
+-----+-------------+
|  9  | D7          |
+-----+-------------+
| 10  | D6          |
+-----+-------------+
| 11  | D5          |
+-----+-------------+
| 12  | D4          |
+-----+-------------+
| 13  | D3          |
+-----+-------------+
| 14  | D2          |
+-----+-------------+
| 15  | D1          |
+-----+-------------+
| 16  | D0          |
+-----+-------------+
| 17  | RET         |
+-----+-------------+
| 18  | PWDN        |
+-----+-------------+
| 19  | RET         |
+-----+-------------+
| 20  | PWDN        |
+-----+-------------+

Requirements
************

This shield requires the Renesas AI Kit Development Platform to operate
(e.g. :zephyr:board:`aik_ra8d1`).

Programming
***********

Set ``--shield renesas_aik_ov2640_cam`` when you invoke ``west build``. For example:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/video/capture
   :board: aik_ra8d1
   :shield: renesas_aik_ov2640_cam
   :goals: build flash

References
**********

.. target-notes::

.. _AIK-RA8D1:
   https://www.renesas.com/en/design-resources/boards-kits/aik-ra8d1
