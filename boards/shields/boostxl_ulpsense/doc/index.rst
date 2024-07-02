.. _boostxl-ulpsense:

BOOSTXL-ULPSENSE: Ultra-low Power Sensor BoosterPack
####################################################

Overview
********

The Ultra-low Power Sensor BoosterPack (BOOSTXL-ULPSENSE) adds analog and
digital sensors to a TI LaunchPad |trade| development kit. The plug-in module
features inductive flow meter measurement circuits, two capacitive touch
buttons, a light sensor, a reed switch, and an ultra-low power accelerometer.

More information about the board can be found at the
`BOOSTXL_ULPSENSE website`_.

Requirements
************

This shield can be used with any TI LaunchPad |trade| development kit with
BoosterPack connectors.

Programming
***********

Set ``--shield boostxl_ulpsense`` when you invoke ``west build``. For example:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/accel_polling/
   :board: cc1352r1_launchxl
   :shield: boostxl_ulpsense
   :goals: build

References
**********

.. target-notes::

.. _BOOSTXL_ULPSENSE website:
   http://www.ti.com/tool/BOOSTXL-ULPSENSE
