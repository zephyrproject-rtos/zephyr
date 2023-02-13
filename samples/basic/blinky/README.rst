.. _blinky-sample:

Blinky
######

Overview
********

The Blinky sample blinks an LED forever using the :ref:`GPIO API <gpio_api>`.

The source code shows how to:

#. Get a pin specification from the :ref:`devicetree <dt-guide>` as a
   :c:struct:`gpio_dt_spec`
#. Configure the GPIO pin as an output
#. Toggle the pin forever

See :ref:`pwm-blinky-sample` for a similar sample that uses the PWM API instead.

.. _blinky-sample-requirements:

Requirements
************

Your board must:

#. Have an LED connected via a GPIO pin (these are called "User LEDs" on many of
   Zephyr's :ref:`boards`).
#. Have the LED configured using the ``led0`` devicetree alias.

Building and Running
********************

Build and flash Blinky as follows, changing ``reel_board`` for your board:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: reel_board
   :goals: build flash
   :compact:

After flashing, the LED starts to blink. If a runtime error occurs, the sample
exits without printing to the console.

Build errors
************

You will see a build error at the source code line defining the ``struct
gpio_dt_spec led`` variable if you try to build Blinky for an unsupported
board.

On GCC-based toolchains, the error looks like this:

.. code-block:: none

   error: '__device_dts_ord_DT_N_ALIAS_led_P_gpios_IDX_0_PH_ORD' undeclared here (not in a function)

Adding board support
********************

To add support for your board, add something like this to your devicetree:

.. code-block:: DTS

   / {
   	aliases {
   		led0 = &myled0;
   	};

   	leds {
   		compatible = "gpio-leds";
   		myled0: led_0 {
   			gpios = <&gpio0 13 GPIO_ACTIVE_LOW>;
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
