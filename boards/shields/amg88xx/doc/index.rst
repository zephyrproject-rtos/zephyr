.. _amg88xx_shields:

Panasonic Grid-EYE (AMG88xx) Shields
####################################

Overview
********

The Panasonic AMG88XX is an 8x8 (64) pixel infrared array sensor available
only in a SMD design. Here are two sources for easily working with this sensor:

- `AMG88xx Evaluation Kit`_
- `Adafruit AMG8833 8x8 Thermal Camera Sensor`_

Current supported shields
=========================

+--------------------------------+--------------------------------+
| Name                           | Shield Designation             |
|                                |                                |
+================================+================================+
| AMG88xx Evaluation Kit         | amg88xx_eval_kit               |
+--------------------------------+--------------------------------+

Requirements AMG88xx Evaluation Kit
***********************************

On the AMG88xx Evaluation Kit, all jumpers except the J11 and J14 must be removed.
This shield can only be used with a board that provides a configuration
for Arduino connectors and defines node aliases for I2C and GPIO interfaces
(see :ref:`shields` for more details).

Pins Assignment of the AMG88xx Evaluation Kit Shield
====================================================

+-----------------------+------------+----------------------------+
| Shield Connector Pin  | Function   |                            |
+=======================+============+============================+
| D6                    | INT        |  AMG88xx INT Pin           |
+-----------------------+------------+----------------------------+
| D14                   | I2C SDA    |  AMG88xx SDA Pin           |
+-----------------------+------------+----------------------------+
| D15                   | I2C SCL    |  AMG88xx SCL Pin           |
+-----------------------+------------+----------------------------+

The same shield designation can also be used with breakout boards like
`Adafruit AMG8833 8x8 Thermal Camera Sensor`_. The wires should be connected
according to the table above.

Programming
***********

Correct shield designation (see the table above) for your shield must
be entered when you invoke ``west build``.
For example:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/amg88xx
   :board: nrf52840dk_nrf52840
   :shield: amg88xx_eval_kit
   :goals: build

References
**********

.. target-notes::

- https://eu.industrial.panasonic.com/products/sensors-optical-devices/sensors-automotive-and-industrial-applications/infrared-array

.. _`AMG88xx Evaluation Kit`: https://eu.industrial.panasonic.com/grideye-evalkit
.. _`Adafruit AMG8833 8x8 Thermal Camera Sensor`: https://learn.adafruit.com/adafruit-amg8833-8x8-thermal-camera-sensor/overview
