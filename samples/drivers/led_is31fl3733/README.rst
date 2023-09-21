.. _is31fl3733:

IS31FL3733 LED Matrix Driver Demo Application
#############################################

Overview
********

This sample controls a matrix of up to 192 LEDs. The sample performs the
following test steps in an infinite loop:

- Set all LEDs to full brightness with `led_write_channels` API
- Disable upper quadrant of LED array with `led_write_channels` API
- Dim each LED in sequence using `led_set_brightness` API
- Toggle each LED in sequency using `led_on` and `led_off` APIs
- Toggle between low or high current limit using `is31fl3733_current_limit`
  API, and repeat the above tests

Sample Configuration
====================

The number of LEDs can be limited using the following sample specific Kconfigs:
- `CONFIG_LED_ROW_COUNT`
- `CONFIG_LED_COLUMN_COUNT`

Building and Running
********************

This sample can be run on any board with an IS31FL3733 LED driver connected via
I2C, and a node with the `issi,is31fl3733` compatible present in its devicetree.

This sample provides a DTS overlay for the :ref:`frdm_k22f` board
(:file:`boards/frdm_k22f.overlay`). It assumes that the IS31FL3733 LED
controller is connected to I2C0, at address 0x50. The SDB GPIO should be
connected to PTC2 (A3 on the arduino header)
