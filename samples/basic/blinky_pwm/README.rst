.. _blink-led-sample:
.. _pwm-blinky-sample:

PWM Blinky
##########

Overview
********

This application blinks a LED using the :ref:`PWM API <pwm_api>`. See
:ref:`blinky-sample` for a GPIO-based sample.

The LED starts blinking at a 1 Hz frequency. The frequency doubles every 4
seconds until it reaches 64 Hz. The frequency will then be halved every 4
seconds until it returns to 1 Hz, completing a single blinking cycle. This
faster-then-slower blinking cycle then repeats forever.

Some PWM hardware cannot set the PWM period to 1 second to achieve the blinking
frequency of 1 Hz. This sample calibrates itself to what the hardware supports
at startup. The maximum PWM period is decreased appropriately until a value
supported by the hardware is found.

Requirements
************

This application requires an LED connected to a PWM output channel.
If this is the case already on your board, the PWM controlling this LED must
be configured using the ``pwm_led0`` :ref:`devicetree <dt-guide>` alias,
usually in the :ref:`BOARD.dts file <devicetree-in-out-files>`.

You will see this error if you try to build this sample for an unsupported
board:

.. code-block:: none

   Unsupported board: pwm_led0 devicetree alias is not defined

In this case, you can still make this application work on your board by
wiring an LED manually to a PWM enabled pin and using a devicetree overlay to
reflect this configuration.
An example of .overlay file is available in the boards/ folder and works with
nucleo_f401re. See :ref:`set-devicetree-overlays` for more information.

Building and Running
********************

To build and flash this sample for the :ref:`nrf52840dk_nrf52840`:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blink_led
   :board: nrf52840dk_nrf52840
   :goals: build flash
   :compact:

Change ``nrf52840dk_nrf52840`` appropriately for other supported boards.

After flashing, the sample starts blinking the LED as described above. It also
prints information to the board's console.
