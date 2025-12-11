.. zephyr:code-sample:: stepper
   :name: Stepper
   :relevant-api: stepper_interface

   Rotate a stepper motor in 4 different modes.

Description
***********

This sample demonstrates how to use the stepper driver API to control a stepper motor. The sample
spins, enables, disables, stops the stepper and outputs the events to the console.

The stepper spins in 4 different modes: ping_pong_relative, ping_pong_absolute, continuous_clockwise
and continuous_anticlockwise. The micro-step interval in nanoseconds can be configured using the
:kconfig:option:`CONFIG_STEP_INTERVAL_NS`. The sample also demonstrates how to use the stepper callback
to change the direction of the stepper after a certain number of steps.

Pressing any button should change the mode of the stepper. The mode is printed to the console.
Following modes are supported: enable, ping_pong_relative, ping_pong_absolute, rotate_cw, rotate_ccw,
stop and disable.

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

   *** Booting Zephyr OS build v4.1.0-568-gad33d28d0348 ***
   [00:00:00.000,000] <inf> stepper: Starting generic stepper sample
   [00:00:00.000,000] <dbg> stepper: main: stepper is 0x80086b8, name is gpio_stepper
   [00:00:00.000,000] <dbg> gpio_stepper_motor_controller: gpio_stepper_set_microstep_interval: Setting Motor step interval to 1000000
   [00:00:00.000,000] <dbg> stepper: monitor_thread: Actual position: 0
   [00:00:00.491,000] <inf> stepper: mode: enable
   [00:00:00.876,000] <inf> stepper: mode: ping pong relative
   [00:00:01.000,000] <dbg> stepper: monitor_thread: Actual position: -114
   [00:00:01.237,000] <inf> stepper: mode: ping pong absolute
   [00:00:01.564,000] <inf> stepper: mode: rotate cw
   [00:00:01.871,000] <inf> stepper: mode: rotate ccw
   [00:00:02.000,000] <dbg> stepper: monitor_thread: Actual position: 129
   [00:00:02.164,000] <inf> stepper: mode: stop
   [00:00:02.444,000] <inf> stepper: mode: disable
   [00:00:02.755,000] <inf> stepper: mode: enable

   <repeats endlessly>
