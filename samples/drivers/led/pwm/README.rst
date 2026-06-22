.. zephyr:code-sample:: led-pwm
   :name: LED PWM
   :relevant-api: led_interface

   Control PWM LEDs using the LED API.

Overview
********

This sample allows to test the led-pwm driver. The first "pwm-leds" compatible
device instance found in DT is used. For each LEDs attached to this device
(child nodes) the same test pattern (described below) is executed. The LED API
functions are used to control the LEDs.

Test pattern
============

For each PWM LEDs (one after the other):

- Turning on
- Turning off
- Increasing brightness gradually
- Decreasing brightness gradually
- Blinking on: 0.1 sec, off: 0.1 sec
- Blinking on: 1 sec, off: 1 sec
- Turning off

Fallback Blink Delay Calculation
=================================

When the configured blink delays exceed the platform's timer range limitations,
the sample automatically calculates fallback delays based on each LED's PWM
period defined in the devicetree. The calculation uses configurable divisors:

- Short delay = LED PWM period / :kconfig:option:`CONFIG_BLINK_DELAY_SHORT_LED_PERIOD_DIV` / 2
- Long delay = LED PWM period / :kconfig:option:`CONFIG_BLINK_DELAY_LONG_LED_PERIOD_DIV` / 2

This ensures the sample works correctly on platforms with timer limitations by
providing hardware-appropriate delays that are compatible with each LED's PWM
configuration.

Building and Running
********************

This sample can be built and executed on all the boards with PWM LEDs connected.
The LEDs must be correctly described in the DTS: the compatible property of the
device node must match "pwm-leds". And for each LED, a child node must be
defined and the PWM configuration must be provided through a "pwms" phandle's
node.
