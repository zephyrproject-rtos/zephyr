.. _dfrobot_gravity_tm6605:

DFRobot Gravity TM6605 Haptic Motor Driver Module
#################################################

Overview
********

The `DFRobot Gravity TM6605 Haptic Motor Driver Module`_ (DRI0056) features a
`Titan Micro Electronics TM6605`_ haptic driver for linear resonant actuators
(LRAs), exposed over I2C on a Gravity 4-pin connector.

Requirements
************

This shield can be used with boards which provide an I2C connector, for example
a Gravity, STEMMA QT or Qwiic connector. The target board must define a
``zephyr_i2c`` node label. See :ref:`shields` for more details.

Pin Assignments
===============

+------------+-----------------+
| Shield Pin | Function        |
+============+=================+
| SCL        | TM6605 I2C SCL  |
+------------+-----------------+
| SDA        | TM6605 I2C SDA  |
+------------+-----------------+
| VCC        | Power supply    |
+------------+-----------------+
| GND        | Ground          |
+------------+-----------------+

Programming
***********

Set ``--shield dfrobot_gravity_tm6605`` when you invoke ``west build``. For
example when running the :zephyr:code-sample:`tm6605` haptics sample:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/haptics/tm6605
   :board: adafruit_qt_py_rp2040
   :shield: dfrobot_gravity_tm6605
   :goals: build

.. _DFRobot Gravity TM6605 Haptic Motor Driver Module:
   https://www.dfrobot.com/product-3003.html

.. _Titan Micro Electronics TM6605:
   http://www.titanmec.com/
