.. zephyr:code-sample:: mchp_evsys_led_control
   :name: Mchp evsys led control
   :relevant-api: gpio_interface

   Set and unset an LED using the button by utilising the capabilities of the EVSYS module.

Overview
********

The sample sets and unsets an LED using the button by utilising the capabilities of the EVSYS module.
The button is configured to generate an event on a rising edge, which is then used to trigger a GPIO
pin to set or unset the LED.

The source code shows how to:

#. Get a pin specification from the :ref:`devicetree <dt-guide>` as a
   :c:struct:`gpio_dt_spec`
#. Configure the GPIO pin as an output

.. _mchp_evsys_led_control-sample-requirements:

Requirements
************

Your board must:

#. Have an LED connected via a GPIO pin (these are called "User LEDs" on many of
   Zephyr's :ref:`boards`).
#. Have the LED configured using the ``led0`` devicetree alias.
#. Have a button connected via a GPIO pin (these are called "User Buttons" on many of
   Zephyr's :ref:`boards`).
#. Have the button configured using the ``sw0`` devicetree alias.
#. Have the EVSYS module available and enabled in the devicetree.

Building and Running
********************

Build and flash the sample as follows, changing ``sam_e54_xpro`` for your board:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/misc/mchp_evsys_led_control/
   :board: sam_e54_xpro
   :goals: build flash
   :compact:

After flashing, the LED will toggle when the button is pressed. If a runtime error occurs, the sample exits without
printing to the console.

Build errors
************

You will see a build error at the source code line defining the ``struct
gpio_dt_spec led`` variable if you try to build sample for an unsupported
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
            gpios = <&portc 19 GPIO_ACTIVE_LOW | MCHP_GPIO_TGL_ON_EVENT>;
         };
      };
   };

The above sets your board's ``led0`` alias to use pin 13 on GPIO controller
``portc``. The pin flags :c:macro:`GPIO_ACTIVE_LOW` mean the LED is on when
the pin is set to its low state, and off when the pin is in its high state.

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
