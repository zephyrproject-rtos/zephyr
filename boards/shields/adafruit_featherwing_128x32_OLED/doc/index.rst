.. _adafruit_featherwing_128x32_OLED:

Adafruit FeatherWing 128 x 32 OLED
##################################

Overview
********

The Adafruit FeatherWing with a resolution of 128x32 pixels, is based on
the SSD1306 controller. More information about the shield can be found
at the `Adafruit website`_.

Pins Assignment of the Adafruit 
========================================================

+-----------------------+---------------------------------------------+
| Shield Connector Pin  | Function                                    |
+=======================+=============================================+
+-----------------------+---------------------------------------------+
| SDA                   | SSD1306 I2C SDA                             |
+-----------------------+---------------------------------------------+
| SCL                   | SSD1306 I2C SCL                             |
+-----------------------+---------------------------------------------+
| GPIO5                 | Button C (INPUT_KEY_3)                      |
+-----------------------+---------------------------------------------+
| GPIO6                 | Button B (INPUT_KEY_2)                      |
+-----------------------+---------------------------------------------+
| GPIO9                 | Button A (INPUT_KEY_1)                      |
+-----------------------+---------------------------------------------+

.. note::

Requirements
************
This shield can only be used with a board which provides a configuration for Feather connectors and
defines a node aliases for I2C (see :ref:`shields` for more details).



Programming
***********

Set ``--shield adafruit_featherwing_128x32_OLED`` when you invoke ``west build``. For example:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/display
   :board: adafruit_feather_nrf52840
   :shield: adafruit_featherwing_128x32_OLED
   :goals: build

References
**********

.. target-notes::

.. _Adafruit OLED FeatherWing (128x32) website:
   https://learn.adafruit.com/adafruit-oled-featherwing
