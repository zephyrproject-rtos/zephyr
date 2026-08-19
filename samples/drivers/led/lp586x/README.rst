.. zephyr:code-sample:: lp586x
   :name: LP586x LED matrix
   :relevant-api: led_interface

   Control LED dots on an LP586x LED matrix controller.

Overview
********

This sample controls LED dots connected to an LP586x LED matrix controller.

The LP586x family consists of I2C LED matrix controllers with 18 constant
current sinks and a configurable number of scan lines:

- LP5861: 1 scan line,  18 LED dots
- LP5862: 2 scan lines, 36 LED dots
- LP5864: 4 scan lines, 72 LED dots
- LP5866: 6 scan lines, 108 LED dots
- LP5868: 8 scan lines, 144 LED dots
- LP5860: 11 scan lines, 198 LED dots

First, information for each configured LED node is retrieved and printed.
Then, from an infinite loop, three test patterns are applied repeatedly:

1. **Channel test** (all LEDs simultaneously via :c:func:`led_write_channels`):
   all LED dots are faded from off to full brightness together.

2. **LED brightness test** (each configured LED node individually):
   each monochromatic LED is faded in using :c:func:`led_set_brightness`, then
   turned off with :c:func:`led_off`.

3. **LED color test** (each configured LED node individually):
   each color channel of each multi-color LED is faded in using
   :c:func:`led_set_color`.

I2C Addressing
**************

The LP586x uses a non-standard I2C addressing scheme: the upper 2 bits of the
10-bit register address are embedded in the 7-bit I2C device address.

The 7-bit address format is ``[CA4:CA0][RA9:RA8]``, where ``CA[4:0]`` is
configured by the hardware ADDR pins and ``RA[9:8]`` selects the register page.
The ``reg`` property in the DTS node must specify the **page-0 base address**
(with ``RA[9:8] = 00``). The driver computes the correct address for page-1
and page-2 accesses automatically.

LED Node Configuration
**********************

Each child node represents one LED or a group of consecutive LED dots:

- ``index``: the dot index of the first dot in this node
  (``line_number * 18 + sink_number``)
- ``color-mapping``: one entry per dot in the group; use
  ``LED_COLOR_ID_WHITE`` for monochrome dots

Building and Running
********************

This sample requires an LP586x LED matrix controller connected to the I2C bus.
Adapt the overlay to match your board's I2C bus alias and hardware address.

An example overlay for boards with an Arduino-compatible I2C connector is
provided in :file:`boards/arduino_i2c.overlay`. It configures an LP5864
at address ``0x40`` (CA[4:0] = 0b10000) with six LED nodes.

References
**********

- LP5860/LP5861/LP5862/LP5864/LP5866/LP5868:
  https://www.ti.com/product/LP5860
