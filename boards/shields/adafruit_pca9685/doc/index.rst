.. _adafruit_pca9685:

Adafruit 16-channel PWM/Servo Shield
####################################

Overview
********

The Adafruit 16-channel PWM/Servo shield is an Arduino
UNO R3 compatible shield based on the NXP PCA9685 IC.

More information about the shield can be found
at the `Adafruit 16-channel PWM/Servo Shield webpage`_.

Pins Assignments
================

+-----------------------+---------------------+
| Shield Connector Pin  | Function            |
+=======================+=====================+
| A5                    | I2C - SCL1          |
+-----------------------+---------------------+
| A4                    | I2C - SDA1          |
+-----------------------+---------------------+


Programming
***********

Set ``-DSHIELD=adafruit_pca9685`` when you invoke ``west build``.
For example:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/led_pwm
   :board: nrf52840dk_nrf52840
   :shield: adafruit_pca9685
   :goals: build

References
**********

.. target-notes::

.. _Adafruit 16-channel PWM/Servo Shield webpage:
   https://learn.adafruit.com/adafruit-16-channel-pwm-slash-servo-shield?view=all
