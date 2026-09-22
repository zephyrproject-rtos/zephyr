.. zephyr:code-sample:: is31fl3733
   :name: IS31FL3733 / IS31FL3743B LED Matrix
   :relevant-api: led_interface

   Control a matrix of LEDs connected to an IS31FL3733 or IS31FL3743B driver chip.

Overview
********

This sample controls a matrix of up to 198 LEDs. It supports two controllers:

- :dtcompatible:`issi,is31fl3733`, an I2C part with a 12 SW by 16 CS matrix
  (192 LEDs)
- :dtcompatible:`issi,is31fl3743b`, an SPI part with an 11 SW by 18 CS matrix
  (198 LEDs)

The sample performs the following test steps in an infinite loop:

- Set all LEDs to full brightness with :c:func:`led_write_channels` API
- Disable upper quadrant of LED array with :c:func:`led_write_channels` API
- Dim each LED in sequence using :c:func:`led_set_brightness` API
- Toggle each LED in sequency using :c:func:`led_on` and :c:func:`led_of` APIs
- On the IS31FL3733 only, toggle between low or high current limit using
  :c:func:`is31fl3733_current_limit` API, and repeat the above tests

The IS31FL3743B does not expose a runtime current limit API. It takes its
global current limit from the ``current-limit`` devicetree property, which is
applied during driver initialization.

If both controllers are enabled in the devicetree, the sample will run
for the IS31FL3743B.

Sample Configuration
====================

The number of LEDs can be limited using the following sample specific Kconfigs:

- :kconfig:option:`CONFIG_LED_ROW_COUNT`
- :kconfig:option:`CONFIG_LED_COLUMN_COUNT`

These default to the full matrix of whichever controller is enabled.

Building and Running
********************

This sample can be run on any board with an IS31FL3733 LED driver connected via
I2C, or an IS31FL3743B LED driver connected via SPI, with the relevant node
present and enabled in its devicetree.

IS31FL3733 via I2C
==================

This sample provides a DTS overlay for the :zephyr:board:`frdm_k22f` board
(:file:`boards/frdm_k22f.overlay`). It assumes that the IS31FL3733 LED
controller is connected to I2C0, at address 0x50. The SDB GPIO should be
connected to PTC2 (A3 on the arduino header)

IS31FL3743B via SPI
===================

No overlay is currently provided for the IS31FL3743B. A node can be added with
the :dtcompatible:`issi,is31fl3743b` compatible to the SPI bus it is wired to.
For example:

.. code-block:: devicetree

   &spi1 {
           status = "okay";
           cs-gpios = <&gpioa 4 GPIO_ACTIVE_LOW>;

           is31fl3743b@0 {
                   compatible = "issi,is31fl3743b";
                   reg = <0>;
                   spi-max-frequency = <1000000>;
           };
   };

The SDB pin is optional. If it is routed on your board, add ``sdb-gpios`` so
the driver can take the device out of hardware shutdown at boot.
