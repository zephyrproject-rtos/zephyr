.. zephyr:code-sample:: paj7620_gesture
   :name: PAJ7620 Gesture Sensor
   :relevant-api: sensor_interface

   Get hand gesture data from PAJ7620 sensor.

Overview
********

This sample application gets the output of a gesture sensor (PAJ7620) using either polling or
triggers, and outputs the corresponding gesture to the console, each time one is detected.

Requirements
************

To use this sample, the following hardware is required:

* A board with I2C support (and GPIO to detect external interrupts in trigger mode)
* PAJ7620 sensor

Building and Running
********************

This sample outputs data to the console. It requires a PAJ7620 sensor.

Polling Mode
============

Build and Running for nucleo_f334r8
-----------------------------------

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/paj7620_gesture
   :board: nucleo_f334r8
   :goals: build
   :compact:

Build and Running for nrf52840dk/nrf52840
-----------------------------------------

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/paj7620_gesture
   :board: nrf52840dk/nrf52840
   :goals: build
   :compact:

Trigger Mode
============

In trigger mode, the sample application uses a GPIO to detect external interrupts, therefore GPIO
support must be enabled. Just like every sensor supporting trigger mode, it is possible to choose
between using a global thread (``CONFIG_PAJ7620_TRIGGER_GLOBAL_THREAD``) or a dedicated thread
(``CONFIG_PAJ7620_TRIGGER_OWN_THREAD``) for the interrupt handling.

Build and Running for nucleo_f334r8
-----------------------------------
.. zephyr-app-commands::
   :zephyr-app: samples/sensor/paj7620_gesture
   :board: nucleo_f334r8
   :goals: build
   :gen-args: -DEXTRA_CONF_FILE=trigger.conf
   :compact:

Build and Running for nrf52840dk/nrf52840
-----------------------------------------
.. zephyr-app-commands::
   :zephyr-app: samples/sensor/paj7620_gesture
   :board: nrf52840dk/nrf52840
   :goals: build
   :gen-args: -DEXTRA_CONF_FILE=trigger.conf
   :compact:

Power Management Mode
=====================

In power management mode, the sample application includes Power modes functions (Suspend, Resume)
control upon button press,therefore GPIO support must be enabled. When in suspend the device stops
detecting gestures and enter a low power state as peer PAJ7620 sensor technical specification section 4.3.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/paj7620_gesture
   :board: nrf52840dk/nrf52840
   :goals: build
   :gen-args: -DEXTRA_CONF_FILE=powermodes.conf
   :compact:

Sample Output
=============

.. code-block:: console

   Gesture LEFT
   Gesture RIGHT
   Gesture UP
   Gesture DOWN
   Gesture FORWARD
   Gesture BACKWARD
   Gesture CLOCKWISE
   Gesture COUNTER CLOCKWISE
