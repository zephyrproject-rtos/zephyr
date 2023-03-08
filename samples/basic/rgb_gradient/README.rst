.. zephyr:code-sample:: rgb-gradient

RGB PWM Gradient
################

Overview
********

This is a sample app which drives an RGB LED in a color gradient pattern using PWM.

There are three single-color component LEDs in an RGB LED. In this sample, these
three component LEDs are driven with incrementally changing pulse widths between
several pre-defined color states.

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

Wiring
******

No additional wiring is necessary if the required devicetree aliases refer to
hardware that is already connected to LEDs on the board.

Otherwise, LEDs should be connected to the appropriate PWM channels.

Building and Running
********************

For example, to build and flash this board for :ref:`frdm_k82f`:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/rgb_led
   :board: frdm_k82f
   :goals: build flash
   :compact:

Change ``frdm_k82f`` appropriately for other supported boards.
