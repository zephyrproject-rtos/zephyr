.. zephyr:code-sample:: lp5815
   :name: LP5815 RGB LED
   :relevant-api: led_interface

   Control an RGB LED connected to an LP5815 driver chip.

Overview
********

This sample controls an RGB LED connected to a TI LP5815 driver chip. It shows
red, green, blue and white with :c:func:`led_set_color`, blinks the LED with
:c:func:`led_blink`, which the chip does by itself, and then ends the blinking
and switches the LED off.

Requirements
************

No board in the Zephyr tree has an LP5815, so the sample needs a devicetree
overlay that adds the chip to an I2C bus of the board, with an RGB LED on its
three outputs. For example, in :file:`boards/<board>.overlay`:

.. code-block:: devicetree

   #include <zephyr/dt-bindings/led/led.h>

   &i2c0 {
           status = "okay";

           lp5815@2d {
                   compatible = "ti,lp5815";
                   reg = <0x2d>;
                   dot-current = <255 255 255>;

                   led_0 {
                           label = "RGB LED";
                           index = <0>;
                           color-mapping = <LED_COLOR_ID_RED
                                            LED_COLOR_ID_GREEN
                                            LED_COLOR_ID_BLUE>;
                   };
           };
   };

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/led/lp5815
   :board: <board>
   :goals: build flash
   :compact:
