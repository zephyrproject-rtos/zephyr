.. _fade-led-sample:

Fade LED
########

Overview
********

This application "fades" a LED using the :ref:`PWM API <pwm_api>`.

The LED starts off increases its brightness until it is fully or nearly fully
on. The brightness then decreases until the LED is off, completing on fade
cycle. Each cycle takes 2.5 seconds, and the cycles repeat forever.

Requirements and Wiring
***********************

This sample has the same requirements and wiring considerations as the
:ref:`pwm-blinky-sample`.

Building and Running
********************

To build and flash this sample for the :ref:`nrf52840dk_nrf52840`:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/fade_led
   :board: nrf52840dk_nrf52840
   :goals: build flash
   :compact:

Change ``nrf52840dk_nrf52840`` appropriately for other supported boards.

After flashing, the sample starts fading the LED as described above. It also
prints information to the board's console.
