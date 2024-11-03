.. zephyr:code-sample:: display
   :name: Display
   :relevant-api: display_interface

   Draw basic rectangles on a display device.

Overview
********

This sample will draw some basic rectangles onto the display.
The rectangle colors and positions are chosen so that you can check the
orientation of the LCD and correct RGB bit order. The rectangles are drawn
in clockwise order, from top left corner: red, green, blue, grey. The shade of
grey changes from black through to white. If the grey looks too green or red
at any point or the order of the corners is not as described above then the LCD
may be endian swapped.

Building and Running
********************

As this is a generic sample it should work with any display supported by Zephyr.

Below is an example on how to build for a :ref:`nrf52840dk_nrf52840` board with a
:ref:`adafruit_2_8_tft_touch_v2`.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/display
   :board: nrf52840dk/nrf52840
   :goals: build
   :shield: adafruit_2_8_tft_touch_v2
   :compact:

For testing purpose without the need of any hardware, the :ref:`native_sim <native_sim>`
board is also supported and can be built as follows;

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/display
   :board: native_sim
   :goals: build
   :compact:

List of Arduino-based display shields
*************************************

- :ref:`adafruit_2_8_tft_touch_v2`
- :ref:`ssd1306_128_shield`
- :ref:`st7789v_generic`
- :ref:`waveshare_epaper`

Changes for ls0xx Display
*************************

To run the sample on an Adafruit Sharp memory display 2.7 inch (ls0xx) on a STM32H7B0 board, ensure the following:

1. Modify the device tree snippet to match the display configuration.
2. Ensure the `buf_desc.width` is set to 400 in the sample code.
3. Check the return value of `display_get_capabilities()` in the sample code.

Example device tree snippet:

.. code-block:: dts

   &spi2 {
   	pinctrl-0 = <&spi2_sck_pb10 &spi2_miso_pc2_c &spi2_mosi_pc1>;
   	cs-gpios = <&gpioc 0 GPIO_ACTIVE_HIGH>;
   	pinctrl-names = "default";
   	status = "okay";

   	ls0xx_ls027b7dh01: ls0xx@0 {
   		compatible = "sharp,ls0xx";
   		spi-max-frequency = <2000000>;
   		reg = <0>;
   		width = <400>;
   		height = <240>;
   		extcomin-gpios = <&gpioc 3 GPIO_ACTIVE_HIGH>;
   		extcomin-frequency = <60>; /* required if extcomin-gpios is defined */
   		disp-en-gpios = <&gpioc 13 GPIO_ACTIVE_HIGH>;
   	};

   };
