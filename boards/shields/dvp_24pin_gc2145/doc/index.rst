.. _dvp_24pin_gc2145:

DVP 24-pin GC2145 Camera Modules
################################


Overview
********

This shield supports the GC2145 camera modules with an 24-pin FFC connector compatible with the
de-facto standard for camera modules described here: :dtcompatible:`dvp-24pin-connector`

.. figure:: gc2145.webp
   :align: center
   :alt: GC2145 Camera Sensor

+-------------------+--------------+
| FFC Connector Pin | Function     |
+===================+==============+
| 1                 | NC           |
+-------------------+--------------+
| 2                 | Analog GND   |
+-------------------+--------------+
| 3                 | SDA          |
+-------------------+--------------+
| 4                 | Analog VDD   |
+-------------------+--------------+
| 5                 | SCL          |
+-------------------+--------------+
| 6                 | Reset        |
+-------------------+--------------+
| 7                 | Vsync        |
+-------------------+--------------+
| 8                 | Powerdown    |
+-------------------+--------------+
| 9                 | Hsync        |
+-------------------+--------------+
| 10                | Digital VDD  |
+-------------------+--------------+
| 11                | I/O VDD      |
+-------------------+--------------+
| 12                | Data 8       |
+-------------------+--------------+
| 13                | Master Clock |
+-------------------+--------------+
| 14                | Data 7       |
+-------------------+--------------+
| 15                | Data GND     |
+-------------------+--------------+
| 16                | Data 6       |
+-------------------+--------------+
| 17                | Pixel Clock  |
+-------------------+--------------+
| 18                | Data 5       |
+-------------------+--------------+
| 19                | Data 1       |
+-------------------+--------------+
| 20                | Data 4       |
+-------------------+--------------+
| 21                | Data 2       |
+-------------------+--------------+
| 22                | Data 3       |
+-------------------+--------------+
| 23                | NC           |
+-------------------+--------------+
| 24                | NC           |
+-------------------+--------------+


Requirements
************

See :dtcompatible:`dvp-24pin-connector`


Programming
***********

Set ``--shield dvp_24pin_gc2145`` when you invoke ``west build``. For example:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/video/capture
   :board: aipi_eyes_s2
   :shield: dvp_24pin_gc2145
   :goals: build


References
**********

.. target-notes::

.. _Camera GC2145:
   https://blog.arducam.com/gc2145/

.. _GC2145 datasheet:
   https://www.uctronics.com/download/cam_module/GC2145DS.pdf
