.. _open_smart_3_8_tft_touch_arduino:

Open-Smart Display 3.8" TFT Touch Shield with Arduino adapter
#############################################################

Overview
********

The Open-Smart 3.8" TFT Touch Shield provides a 320x480 pixels color
display with a resistive touch, plus a flash card connector.

.. note::
   Neither touch nor flash card is currently supported

Pin Assignments
===============

+-----------------------+---------------------------------------------+
| Shield Connector Pin  | Function                                    |
+=======================+=============================================+
| D8                    | Data 0                                      |
+-----------------------+---------------------------------------------+
| D9                    | Data 1                                      |
+-----------------------+---------------------------------------------+
| D10                   | Data 2                                      |
+-----------------------+---------------------------------------------+
| D11                   | Data 3                                      |
+-----------------------+---------------------------------------------+
| D4                    | Data 4                                      |
+-----------------------+---------------------------------------------+
| D13                   | Data 5                                      |
+-----------------------+---------------------------------------------+
| D6                    | Data 6    (and X+ on touch)                 |
+-----------------------+---------------------------------------------+
| D7                    | Data 7    (and Y- on touch)                 |
+-----------------------+---------------------------------------------+
| A0                    | RD                                          |
+-----------------------+---------------------------------------------+
| A1                    | WR   (and Y+ on touch)                      |
+-----------------------+---------------------------------------------+
| A2                    | Data/Command (DC/X)  (and X- on touch)      |
+-----------------------+---------------------------------------------+
| A3                    | CS                                          |
+-----------------------+---------------------------------------------+
| D3                    | Backlight PWM (req. minor mod of PCB)       |
+-----------------------+---------------------------------------------+

Requirements
************

This shield can only be used with a board which provides a configuration
for Arduino connectors and defines node aliases for GPIO interfaces
(see :ref:`shields` for more details).

Programming
***********

Set ``-DSHIELD=open_smart_3_8_tft_touch_arduino`` when you invoke
``west build``. For example:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/display/lvgl
   :board: frdm_k64f
   :shield: open_smart_3_8_tft_touch_arduino
   :goals: build

References
**********

.. target-notes::

.. _Open-Smart 3.8" TFT Touch Shield website:
   https://www.youtube.com/watch?v=RnuWa8ISe-4&ab_channel=FrankieChu
