.. zephyr:code-sample:: stepper
   :name: Stepper
   :relevant-api: stepper_interface

   Rotate a stepper motor in 4 different modes.

Description
***********

This sample demonstrates how to use the stepper driver API to control a stepper motor. The sample
spins the stepper and outputs the events to the console.

The stepper spins in 4 different modes: ping_pong_relative, ping_pong_absolute, continuous_clockwise
and continuous_anticlockwise. The micro-step interval in nanoseconds can be configured using the
:kconfig:option:`CONFIG_STEP_INTERVAL_NS`. The sample also demonstrates how to use the stepper callback
to change the direction of the stepper after a certain number of steps.

Pressing any button should change the mode of the stepper.

The sample also has a monitor thread that prints the actual position of the stepper motor every
:kconfig:option:`CONFIG_MONITOR_THREAD_TIMEOUT_MS` milliseconds.

Building and Running
********************

This project spins the stepper and outputs the events to the console.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/stepper/generic
   :board: nucleo_g071rb
   :goals: build flash

Sample Output
=============

.. code-block:: console

   *** Booting Zephyr OS build v4.0.0-5289-g0c368e85b117 ***
   Actual position: 910
   Actual position: 1821
   mode: ping pong absolute
   Actual position: 2410
   mode: rotate cw
   Actual position: 2162
   Actual position: 3073
   mode: rotate ccw
   Actual position: 3793
   mode: ping pong relative
   Actual position: 4607
   Actual position: 5518
   Actual position: 6428

   <repeats endlessly>
