.. _arducam_cu450_ov5640:

ArduCam CU450 OV5640 Camera Module
##################################

Overview
********

The ArduCam CU450 OV5640 shield provides the OV5640 image sensor on a 40-pin FFC connector,
supporting both the DVP (Digital Video Port) interface and the MIPI CSI-2 interface.
It is designed specifically for the Renesas `EK-RA8P1`_ evaluation kit.

Pins assignment
===============

+-----+----------------------+
| Pin | Function             |
+=====+======================+
| 1   | GND                  |
+-----+----------------------+
| 2   | D11                  |
+-----+----------------------+
| 3   | D10                  |
+-----+----------------------+
| 4   | GND                  |
+-----+----------------------+
| 5   | D9 / MIPI_CSI2_DL1_P |
+-----+----------------------+
| 6   | D8 / MIPI_CSI2_DL1_N |
+-----+----------------------+
| 7   | GND                  |
+-----+----------------------+
| 8   | D7 / MIPI_CSI2_CL1_P |
+-----+----------------------+
| 9   | D6 / MIPI_CSI2_CL1_N |
+-----+----------------------+
| 10  | GND                  |
+-----+----------------------+
| 11  | D5 / MIPI_CSI2_DL0_P |
+-----+----------------------+
| 12  | D4 / MIPI_CSI2_DL0_N |
+-----+----------------------+
| 13  | GND                  |
+-----+----------------------+
| 14  | D3                   |
+-----+----------------------+
| 15  | D2                   |
+-----+----------------------+
| 16  | GND                  |
+-----+----------------------+
| 17  | D1                   |
+-----+----------------------+
| 18  | D0                   |
+-----+----------------------+
| 19  | GND                  |
+-----+----------------------+
| 20  | SCL                  |
+-----+----------------------+
| 21  | SDA                  |
+-----+----------------------+
| 22  | GND                  |
+-----+----------------------+
| 23  | VSYNC                |
+-----+----------------------+
| 24  | HSYNC                |
+-----+----------------------+
| 25  | RESET                |
+-----+----------------------+
| 26  | XCLK                 |
+-----+----------------------+
| 27  | PCLK                 |
+-----+----------------------+
| 28  | INT                  |
+-----+----------------------+
| 29  | GND                  |
+-----+----------------------+
| 30  | GND                  |
+-----+----------------------+
| 31  | 3V3                  |
+-----+----------------------+
| 32  | GND                  |
+-----+----------------------+
| 33  | GND                  |
+-----+----------------------+
| 34  | 3V3                  |
+-----+----------------------+
| 35  | 3V3                  |
+-----+----------------------+
| 36  | 3V3                  |
+-----+----------------------+
| 37  | GND                  |
+-----+----------------------+
| 38  | GND                  |
+-----+----------------------+
| 39  | 3V3                  |
+-----+----------------------+
| 40  | GND                  |
+-----+----------------------+


Requirements
************

This shield requires a board with a 40-pin FFC connector. The Renesas EK-RA8P1
evaluation kit is fully supported.

.. note::
   While the CU450 OV5640 module and connector provide both DVP and
   MIPI CSI-2 interfaces, only the DVP interface is currently supported
   on Renesas RA SoCs. MIPI CSI-2 is not yet supported on this platform.

Programming
***********

Set ``--shield arducam_cu450_ov5640_dvp`` when you invoke ``west build``. For example:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/video/capture
   :board: ek_ra8p1/r7ka8p1kflcac/cm85
   :shield: arducam_cu450_ov5640_dvp
   :goals: build

References
**********

.. target-notes::

.. _EK-RA8P1:
   https://www.renesas.com/en/design-resources/boards-kits/ek-ra8p1/
