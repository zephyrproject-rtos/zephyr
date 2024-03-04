.. _huatian_mt9m114:

HUATIAN_MT9M114 Camera Module
#############################

Overview
********

This camera module connector is from HuaTian Technology (Xi' an) Co. containing
a MT9M114 camera, which is a 1.26 Mp CMOS digital image sensor with an active
pixel array of 1296x976. More information about the MT9M114 camera can be found
at `MT9M114 camera module`_.

This module uses a 24-pin FPC connector with camera parallel interface which is
available on NXP i.MX RT10XX series.

Pins assignment of the Huatian MT9M114 camera module
============================================================

+-----------------------+------------------------+
| FPC Connector Pin     | Function               |
+=======================+========================+
| 3                     | Data 4                 |
+-----------------------+------------------------+
| 4                     | Data 3                 |
+-----------------------+------------------------+
| 5                     | Data 5                 |
+-----------------------+------------------------+
| 6                     | Data 2                 |
+-----------------------+------------------------+
| 7                     | Data 6                 |
+-----------------------+------------------------+
| 8                     | Pixel Clock            |
+-----------------------+------------------------+
| 9                     | Data 7                 |
+-----------------------+------------------------+
| 11                    | Data 8                 |
+-----------------------+------------------------+
| 12                    | Master Clock           |
+-----------------------+------------------------+
| 13                    | Data 9                 |
+-----------------------+------------------------+
| 16                    | Hsync                  |
+-----------------------+------------------------+
| 17                    | Powerdown              |
+-----------------------+------------------------+
| 18                    | Vsync                  |
+-----------------------+------------------------+
| 20                    | I2C Data               |
+-----------------------+------------------------+
| 22                    | I2C Clock              |
+-----------------------+------------------------+

Requirements
************

This shield can only be used with a board which provides a configuration for
a 24-pin FPC connector with parallel interface, such as the i.MX RT1064-EVK.

Programming
***********

Set ``-DSHIELD=huatian_mt9m114`` when you invoke ``west build``. For example:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/video/capture
   :board: mimxrt1064_evk
   :shield: huatian_mt9m114
   :goals: build

References
**********

.. target-notes::

.. _MT9M114 camera module:
   https://www.onsemi.com/pdf/datasheet/mt9m114-d.pdf
