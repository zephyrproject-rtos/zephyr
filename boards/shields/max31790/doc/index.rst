.. _max31790_shield:

MAX31790 PWM and fan driver shield
##################################

Overview
********

This is a generic shield for PWM and fan driver
MAX31790. This chip has an I2C interface and can be
operated with only SCL, SDA, VCC and GND.

Requirements
************

This shield can only be used with a board which provides a configuration
for Arduino connectors and defines a node alias for the I2C interface
(see :ref:`shields` for more details).

Programming
***********

Set ``-DSHIELD=max31790`` when you invoke ``west build``. For example:

.. zephyr-app-commands::
   :zephyr-app: custom-app
   :board: nucleo_f429zi
   :shield: max31790
   :goals: build
