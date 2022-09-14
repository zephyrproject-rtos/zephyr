.. zephyr:code-sample:: fade-led
   :name: Fade LED
   :relevant-api: pwm_interface

   Fade an LED using the PWM API.

Overview
********

This application "fades" a LED using the :ref:`PWM API <pwm_api>`.

The LED starts off increases its brightness until it is fully or nearly fully
on. The brightness then decreases until the LED is off, completing on fade
cycle. Each cycle takes 2.5 seconds, and the cycles repeat forever. The PWM
period is taken from Devicetree. It should be fast enough to be above the
flicker fusion threshold.

Requirements and Wiring
***********************

This sample has the same requirements and wiring considerations as the
:zephyr:code-sample:`pwm-blinky` sample.

Building and Running
********************

To build and flash this sample for the :ref:`nrf52840dk_nrf52840`:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/fade_led
   :board: nrf52840dk/nrf52840
   :goals: build flash
   :compact:

Change ``nrf52840dk/nrf52840`` appropriately for other supported boards.

After flashing, the sample starts fading the LED as described above. It also
prints information to the board's console.
