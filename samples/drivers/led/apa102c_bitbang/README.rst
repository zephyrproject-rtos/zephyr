.. zephyr:code-sample:: apa102c-bitbang
   :name: APA102C LED (bit-bang)
   :relevant-api: gpio_interface

   Control APA102C RGB LEDs using GPIO bit-banging.

Overview
********

This sample demonstrates how to control APA102C RGB LEDs using GPIO bit-banging
(manual software control of GPIO pins). The sample cycles through four colors:
red, green, blue, and white.

The APA102C is a smart RGB LED that uses a simple two-wire interface (data and
clock). This sample uses the :ref:`GPIO API <gpio_api>` to manually control
these signals.

.. warning::

   APA102C LEDs are extremely bright even at low settings. To protect your eyes,
   do not look directly at the LEDs. This sample uses very low brightness
   settings for safety.

Hardware Setup
**************

The APA102C requires 5V data and clock signals. You will need:

- One or more APA102C LEDs
- A logic level shifter to convert 3.3V signals to 5V

.. warning::

   Do not connect 3.3V GPIO outputs directly to 5V device inputs unless your
   board's GPIO pins are explicitly 5V tolerant. Consult your board's datasheet.


Wiring
======

By default, the sample uses the following GPIO pins:

- **Data pin**: GPIO 16
- **Clock pin**: GPIO 19

Connect these pins to the APA102C LED:

- Data pin → DI (Data Input) on LED
- Clock pin → CI (Clock Input) on LED
- Also connect power (5V) and ground

Configuration
*************

The sample can be configured by modifying ``src/main.c``:

- ``GPIO_DATA_PIN``: Change the data pin number
- ``GPIO_CLK_PIN``: Change the clock pin number
- ``NUM_LEDS``: Set the number of LEDs in your chain
- ``SLEEPTIME``: Adjust the color change interval

Requirements
************

Your board must:

#. Have GPIO support
#. Have the ``gpio-0`` devicetree alias defined
#. Have GPIO pins that can be configured as outputs

Building and Running
********************

Build and flash the sample as follows, changing ``your_board`` appropriately:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/led/apa102c_bitbang
   :board: your_board
   :goals: build flash
   :compact:

After flashing, the LED will cycle through the colors. The sample runs
indefinitely without printing to the console.

Sample Output
*************

This sample does not produce console output. You should observe the LED cycling
through red, green, blue, and white colors every 250 milliseconds.
