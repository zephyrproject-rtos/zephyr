.. _led_api:

Light-Emitting Diode (LED)
##########################

Overview
********

The LED API provides access to Light Emitting Diodes, both in individual and
strip form. It abstracts the differences between simple GPIO-driven LEDs,
PWM-dimmable LEDs, dedicated LED controller ICs, and addressable LED strips
behind a small set of common operations.

Two related subsystems are exposed:

* **LED** — general-purpose API for controlling discrete or controller-backed
  LEDs (on/off, brightness, color, blink)
* **LED Strip** — API tailored to addressable LED strings such as WS2812 and
  APA102 where each pixel carries its own color value

LED Operations
**************

A driver implements a subset of the operations below. Calls to optional
operations return ``-ENOSYS`` if the underlying driver does not provide them.

Mandatory operations (at least one of):

* :c:func:`led_on` / :c:func:`led_off` — turn an LED fully on or off
* :c:func:`led_set_brightness` — set brightness in the range 0 to
  :c:macro:`LED_BRIGHTNESS_MAX` (100); when implemented, this is also used
  automatically by :c:func:`led_on` and :c:func:`led_off`

Optional operations:

* :c:func:`led_blink` — start the LED blinking with given on/off durations
* :c:func:`led_set_color` — set the per-channel color values of a
  multi-color LED
* :c:func:`led_get_info` — retrieve the :c:struct:`led_info` describing a
  particular LED (label, index, color mapping)
* :c:func:`led_write_channels` — write a range of raw channel values, used
  by drivers that expose LEDs as an array of channels

LED Indexing
************

Each LED controlled by a driver is addressed by a zero-based ``led`` index.
Multi-color LEDs occupy a single index; their color channels are submitted
together to :c:func:`led_set_color`.

The color channel order for a multi-color LED is described by the
``color_mapping`` field of :c:struct:`led_info`. Each entry is one of the
``LED_COLOR_ID_*`` values defined in :file:`include/zephyr/dt-bindings/led/led.h`:

* ``LED_COLOR_ID_WHITE``
* ``LED_COLOR_ID_RED``
* ``LED_COLOR_ID_GREEN``
* ``LED_COLOR_ID_BLUE``
* ``LED_COLOR_ID_AMBER``
* ``LED_COLOR_ID_VIOLET``
* ``LED_COLOR_ID_YELLOW``
* ``LED_COLOR_ID_IR``

Device Tree Configuration
*************************

Simple LEDs are typically described as child nodes of an LED controller and
referenced via a devicetree alias:

.. code-block:: dts

   / {
       aliases {
           led0 = &status_led;
       };

       leds {
           compatible = "gpio-leds";
           status_led: led_0 {
               gpios = <&gpio0 13 GPIO_ACTIVE_HIGH>;
               label = "Status LED";
           };
       };
   };

For multi-color LEDs, color information is encoded with the ``color-mapping``
property using ``LED_COLOR_ID_*`` values from
:file:`include/zephyr/dt-bindings/led/led.h`.

Usage Examples
**************

Basic on/off control using a devicetree alias:

.. code-block:: c

   #include <zephyr/drivers/led.h>
   #include <zephyr/devicetree.h>

   #define LED_NODE DT_ALIAS(led0)
   static const struct device *led_dev = DEVICE_DT_GET(DT_PARENT(LED_NODE));
   static const uint32_t led_idx = DT_NODE_CHILD_IDX(LED_NODE);

   int turn_on_status_led(void)
   {
       if (!device_is_ready(led_dev)) {
           return -ENODEV;
       }
       return led_on(led_dev, led_idx);
   }

Setting brightness (0 to 100):

.. code-block:: c

   /* Dim the LED to 25% brightness */
   led_set_brightness(led_dev, led_idx, 25);

Blinking with a 500 ms on / 500 ms off pattern:

.. code-block:: c

   int err = led_blink(led_dev, led_idx, 500, 500);

   if (err == -ENOSYS) {
       /* Driver does not implement blink natively */
   }

Setting the color of a multi-color (RGB) LED:

.. code-block:: c

   /* Order of the values must match the color_mapping reported by
    * led_get_info(); for a standard RGB mapping the order is R, G, B.
    */
   const uint8_t purple[] = { 0x80, 0x00, 0x80 };

   led_set_color(led_dev, led_idx, ARRAY_SIZE(purple), purple);

Configuration Options
*********************

Related configuration options:

* :kconfig:option:`CONFIG_LED`
* :kconfig:option:`CONFIG_LED_STRIP`
* :kconfig:option:`CONFIG_LED_SHELL`
* :kconfig:option:`CONFIG_LED_INIT_PRIORITY`

API Reference
*************

LED
===

.. doxygengroup:: led_interface

LED Strip
=========

.. doxygengroup:: led_strip_interface
