.. zephyr:code-sample:: rgb-led
   :name: PWM RGB LED
   :relevant-api: pwm_interface

   Drive an RGB LED using the PWM API.

Overview
********

This is a sample app which drives an RGB LED using the :ref:`PWM API <pwm_api>`.

There are three single-color component LEDs in an RGB LED. Each component LED
is driven by a PWM port where the pulse width is changed from zero to the period
indicated in Devicetree. Such period should be fast enough to be above the
flicker fusion threshold (the minimum flicker rate where the LED is perceived as
being steady). The sample causes each LED component to step from dark to max
brightness. Three **for** loops (one for each component LED) generate a gradual
range of color changes from the RGB LED, and the sample repeats forever.

Requirements
************

The board must have red, green, and blue LEDs connected via PWM output channels.

The LED PWM channels must be configured using the following :ref:`devicetree
<dt-guide>` aliases, usually in the :ref:`BOARD.dts file
<devicetree-in-out-files>`:

- ``red-pwm-led``
- ``green-pwm-led``
- ``blue-pwm-led``

You will see at least one of these errors if you try to build this sample for
an unsupported board:

.. code-block:: none

   Unsupported board: red-pwm-led devicetree alias is not defined
   Unsupported board: green-pwm-led devicetree alias is not defined
   Unsupported board: blue-pwm-led devicetree alias is not defined

See :zephyr_file:`boards/nxp/hexiwear/hexiwear_mk64f12.dts` for an example
:file:`BOARD.dts` file which supports this sample.

Wiring
******

No additional wiring is necessary if the required devicetree aliases refer to
hardware that is already connected to LEDs on the board.

Otherwise, LEDs should be connected to the appropriate PWM channels.

Building and Running
********************

For example, to build and flash this board for :ref:`hexiwear`:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/rgb_led
   :board: hexiwear/mk64f12
   :goals: build flash
   :compact:

Change ``hexiwear/mk64f12`` appropriately for other supported boards.
