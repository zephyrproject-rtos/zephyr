.. _blinky-sample:

Blinky
######

Overview
********

Blinky is a simple application which blinks an LED forever using the :ref:`GPIO
API <gpio_api>`. The source code shows how to configure GPIO pins as outputs,
then turn them on and off.

See :ref:`pwm-blinky-sample` for a sample which uses the PWM API to blink an
LED.

.. _blinky-sample-requirements:

Requirements
************

You will see this error if you try to build Blinky for an unsupported board:

.. code-block:: none

   Unsupported board: led0 devicetree alias is not defined

The board must have an LED connected via a GPIO pin. These are called "User
LEDs" on many of Zephyr's :ref:`boards`. The LED must be configured using the
``led0`` :ref:`devicetree <dt-guide>` alias. This is usually done in the
:ref:`BOARD.dts file <devicetree-in-out-files>` or a :ref:`devicetree overlay
<set-devicetree-overlays>`.

Building and Running
********************

Build and flash Blinky as follows, changing ``reel_board`` for your board:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: reel_board
   :goals: build flash
   :compact:

After flashing, the LED starts to blink. Blinky does not print to the console.
