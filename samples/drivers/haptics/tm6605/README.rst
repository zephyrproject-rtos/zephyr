.. zephyr:code-sample:: tm6605
   :name: TM6605 Haptic Driver
   :relevant-api: haptics_interface

   Drive an LRA using the Titan Micro Electronics TM6605 haptic driver chip.

Overview
********

This sample cycles through a handful of the TM6605's pre-programmed haptic
effects, selecting each one in turn and triggering playback via the generic
:ref:`haptics_api` API and the device-specific
``tm6605_select_effect()`` helper.

Building and Running
********************

Build the application for the :zephyr:board:`nucleo_f401re` board, and
connect a TM6605 (e.g. the DFRobot Gravity TM6605 module, DRI0056) on the
``arduino_i2c`` bus at the 7-bit address ``0x2D``.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/haptics/tm6605
   :board: nucleo_f401re
   :goals: build
   :compact:

When using the DFRobot Gravity TM6605 module on a board with an I2C connector,
the :ref:`dfrobot_gravity_tm6605` shield can be used instead of a board overlay:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/haptics/tm6605
   :board: adafruit_qt_py_rp2040
   :shield: dfrobot_gravity_tm6605
   :goals: build
   :compact:

References
**********

- TM6605 module: https://www.dfrobot.com/product-3003.html
- TM6605 datasheet (Chinese):
  https://dfimg.dfrobot.com/wiki/23281/DRI0056_gravity-tm6605-haptic-motor-driver-module_datasheet_V1.0.pdf
- DFRobot reference library:
  https://github.com/DFRobot/DFRobot_TM6605
