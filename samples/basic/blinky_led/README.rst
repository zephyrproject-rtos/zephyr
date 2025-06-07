.. zephyr:code-sample:: blinky_led
   :name: Blinky LED
   :relevant-api: led_interface

   Blink an LED forever using the LED API.

Overview
********

The Blinky LED sample blinks an LED forever using the :ref:`LED API <led_api>`.

The source code shows how to:

#. Get a pin specification from the :ref:`devicetree <dt-guide>` as a
   :c:struct:`led_dt_spec`
#. Toggle the pin forever

See :zephyr:code-sample:`pwm-blinky` for a similar sample that uses the PWM API instead.

.. _blinky_led-sample-requirements:

Requirements
************

Your board must:

#. Have the LED configured using the ``led0`` devicetree alias.

Building and Running
********************

Build and flash Blinky as follows, changing ``reel_board`` for your board:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky_led
   :board: reel_board
   :goals: build flash
   :compact:

After flashing, the LED starts to blink and messages with the current LED state
are printed on the console. If a runtime error occurs, the sample exits without
printing to the console.

Build errors
************

You will see a build error at the source code line defining the ``struct
led_dt_spec led`` variable if you try to build Blinky for an unsupported
board.

Adding board support
********************

To add support for your board, add something like this to your devicetree:

.. code-block:: DTS

   / {
      aliases {
         led0 = &led_blue;
      };

      leds {
         compatible = "gpio-leds";

         led_blue: led_0 {
            gpios = <&gpio0 2 GPIO_ACTIVE_HIGH>;
         };
      };
   };

The above sets your board's ``led0`` alias to use pin 13 on GPIO controller
``gpio0``. The pin flags :c:macro:`GPIO_ACTIVE_HIGH` mean the LED is on when
the pin is set to its high state, and off when the pin is in its low state.

Tips:

- See :dtcompatible:`gpio-leds` for more information on defining GPIO-based LEDs
  in devicetree.

- If you're not sure what to do, check the devicetrees for supported boards which
  use the same SoC as your target. See :ref:`get-devicetree-outputs` for details.

- See :zephyr_file:`include/zephyr/dt-bindings/gpio/gpio.h` for the flags you can use
  in devicetree.

- If the LED is built in to your board hardware, the alias should be defined in
  your :ref:`BOARD.dts file <devicetree-in-out-files>`. Otherwise, you can
  define one in a :ref:`devicetree overlay <set-devicetree-overlays>`.
