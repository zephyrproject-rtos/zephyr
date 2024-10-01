.. zephyr:code-sample:: is31fl3733
   :name: IS31FL3733 LED Matrix
   :relevant-api: led_interface

   Control a matrix of up to 192 LEDs connected to an IS31FL3733 driver chip.

Overview
********

This sample controls a matrix of up to 192 LEDs. The sample performs the
following test steps in an infinite loop:

- Set all LEDs to full brightness with :c:func:`led_write_channels` API
- Disable upper quadrant of LED array with :c:func:`led_write_channels` API
- Dim each LED in sequence using :c:func:`led_set_brightness` API
- Toggle each LED in sequency using :c:func:`led_on` and :c:func:`led_of` APIs
- Toggle between low or high current limit using :c:func:`is31fl3733_current_limit`
  API, and repeat the above tests

Sample Configuration
====================

The number of LEDs can be limited using the following sample specific Kconfigs:

- :kconfig:option:`CONFIG_LED_ROW_COUNT`
- :kconfig:option:`CONFIG_LED_COLUMN_COUNT`

Building and Running
********************

This sample can be run on any board with an IS31FL3733 LED driver connected via
I2C, and a node with the :dtcompatible:`issi,is31fl3733` compatible present in its devicetree.

This sample provides a DTS overlay for the :ref:`frdm_k22f` board
(:file:`boards/frdm_k22f.overlay`). It assumes that the IS31FL3733 LED
controller is connected to I2C0, at address 0x50. The SDB GPIO should be
connected to PTC2 (A3 on the arduino header)
