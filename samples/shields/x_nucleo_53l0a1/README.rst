.. _x-nucleo-53l0a1-sample:

X-NUCLEO-53L0A1 ranging and gesture detection sensor expansion board
#####################################################################

Overview
********
This sample demonstrate the usage of the 4 digits x 7 segments display and the
three VL53L0X ranging sensors on the :ref:`x_nucleo_53l0a1_shield`.

When flashed on a board, 2 modes are available. To switch from one mode to the
other, press the button ``sw0``.

Distance (onboard sensor)
-------------------------

This is the default mode when starting up. In this mode, if the distance to
the center sensor (soldered on the shield) is lower than 1.25m, then the
4x7 segment display shows the distance in cm.

Proximity (onboard + satellites)
--------------------------------

In this mode, the 4x7 segment display shows 3 stacked horizontal bars if there
is something in proximity for each sensor. The left sensor is shown on the
leftmost digit, then the center sensor, then the right sensor.
The proximity threshold is configured in
:kconfig:option:`CONFIG_VL53L0X_PROXIMITY_THRESHOLD`

To switch from one mode to another, press the button ``sw0``

Requirements
************

This sample communicates over I2C with the X-NUCLEO-53L0A1 shield
stacked on a board with an Arduino connector. The board's I2C must be
configured for the I2C Arduino connector (both for pin muxing
and devicetree). The board must also have a button with the alias ``sw0``
in its device tree.

References
**********

* `X-NUCLEO-53L0A1 website`_

Building and Running
********************

This sample runs with X-NUCLEO-53L0A1 stacked on any board with a matching
Arduino connector. For this example, we use a :ref:`nucleo_f429zi_board` board.

.. zephyr-app-commands::
   :zephyr-app: samples/shields/x_nucleo_53l0a1
   :board: nucleo_f429zi
   :goals: build
   :compact:

.. _X-NUCLEO-53L0A1 website:
   https://www.st.com/en/evaluation-tools/x-nucleo-53l0a1.html
