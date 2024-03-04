.. _wuxi_ov5640:

WUXI_OV5640 Camera Module
#########################

Overview
********

This camera module connector comes from Wuxi A-KERR Science & Technology containing
an OV5640 image sensor which provides the full functionality of a single chip 5
megapixel (2592x1944) camera using OmniBSI™ technology in a small footprint package.

More information about the OV5640 can be found at `OV5640 camera module`_.

This module uses a 42-pin connector with MIPI CSI-2 interface which is available on
NXP i.MX RT11XX series.

Pins assignment of the Wuxi OV5640 camera module connector
============================================================

+-----------------------+------------------------+
| Camera Connector Pin  | Function               |
+=======================+========================+
| 5                     | I2C_SDA                |
+-----------------------+------------------------+
| 7                     | I2C_SCL                |
+-----------------------+------------------------+
| 9                     | RSTB_CTL               |
+-----------------------+------------------------+
| 17                    | PWND_CTL               |
+-----------------------+------------------------+
| 16                    | MIPI_CSI_DP1           |
+-----------------------+------------------------+
| 18                    | MIPI_CSI_DN1           |
+-----------------------+------------------------+
| 22                    | MIPI_CSI_CLKP          |
+-----------------------+------------------------+
| 24                    | MIPI_CSI_CLKN          |
+-----------------------+------------------------+
| 28                    | MIPI_CSI_DP0           |
+-----------------------+------------------------+
| 30                    | MIPI_CSI_DN0           |
+-----------------------+------------------------+

Requirements
************

This shield can only be used with a board which provides a configuration
for a 42-pin connector with MIPI CSI interface, such as i.MX RT1170-EVK.

Programming
***********

Set ``-DSHIELD=wuxi_ov5640`` when you invoke ``west build``. For example:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/video/capture
   :board: mimxrt1170_evk/mimxrt1176/cm7
   :shield: wuxi_ov5640
   :goals: build

References
**********

.. target-notes::

.. _OV5640 camera module:
   https://cdn.sparkfun.com/datasheets/Sensors/LightImaging/OV5640_datasheet.pdf
