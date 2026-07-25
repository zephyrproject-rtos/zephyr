.. _gpio_fast_api:

Fast GPIO
#########

The fast GPIO API extension provides sub-microsecond GPIO operations for bit-bang
drivers that require tight timing constraints (e.g. WS2812 LED strips, 1-Wire,
DHT sensors). It bypasses the standard GPIO driver indirection by using
vendor-specific ``ALWAYS_INLINE`` functions that compile down to single register
writes.

Overview
********

The standard :c:func:`gpio_pin_set_raw` goes through two pointer indirections and
a non-inlinable function call (~10--20 cycles), while protocols like WS2812 allow
only ~20--40 cycles per phase. The fast GPIO API solves this by pre-computing
vendor-specific register addresses at init time into a backend-specific
``gpio_fast_spec_<compat>`` struct, then using ``ALWAYS_INLINE`` functions for all
hot-path operations.

Backend selection is automatic: the dispatch macros use ``DT_BINDING_COMPAT()``
to obtain the GPIO controller's DT compatible as a C token, then token-paste it
onto backend-namespaced symbols. Multiple backends can coexist in a single build.

This API is intended to be as opaque as possible to the user, allowing drivers to
write portable code for each platform.

Usage
*****

Consumers first resolve the backend with ``GPIO_FAST_COMPAT(node_id, prop)``,
then pass the result to all dispatch macros:

.. code-block:: c

   #include <zephyr/drivers/gpio.h>
   #include <zephyr/drivers/gpio/gpio_fast.h>

   #define MY_NODE    DT_ALIAS(my_pin)
   #define MY_BACKEND GPIO_FAST_COMPAT(MY_NODE, gpios)

   static const struct gpio_dt_spec my_pin = GPIO_DT_SPEC_GET(MY_NODE, gpios);
   static struct GPIO_FAST_DISPATCH_TYPE(MY_BACKEND) fast;

   /* At init, pre-compute vendor-specific register state: */
   GPIO_FAST_CONFIGURE_DT(MY_BACKEND, &fast, &my_pin, GPIO_OUTPUT);

   /* Or configure with an explicit port and pin mask: */
   GPIO_FAST_DISPATCH_CALL(MY_BACKEND, gpio_fast_configure,
                           &fast, my_pin.port, BIT(my_pin.pin), GPIO_OUTPUT);

   /* Set all masked pins HIGH: */
   GPIO_FAST_DISPATCH_CALL(MY_BACKEND, gpio_fast_set, &fast);

   /* Set all masked pins LOW: */
   GPIO_FAST_DISPATCH_CALL(MY_BACKEND, gpio_fast_clear, &fast);

   /* Toggle all masked pins: */
   GPIO_FAST_DISPATCH_CALL(MY_BACKEND, gpio_fast_toggle, &fast);

   /* Read raw physical state of masked pins: */
   gpio_port_pins_t val = GPIO_FAST_DISPATCH_CALL(MY_BACKEND, gpio_fast_get, &fast);

   /* Switch pin direction to input (single DIR register write): */
   GPIO_FAST_DISPATCH_CALL(MY_BACKEND, gpio_fast_set_input, &fast);

   /* Read the input value: */
   val = GPIO_FAST_DISPATCH_CALL(MY_BACKEND, gpio_fast_get, &fast);

   /* Switch pin direction back to output: */
   GPIO_FAST_DISPATCH_CALL(MY_BACKEND, gpio_fast_set_output, &fast);

.. note::

   These functions bypass the standard GPIO API and do not perform any error checking. Reading or
   writing to an unconfigured pin causes undefined behavior. The caller is responsible for
   ensuring the pin is properly configured.

Bit-Bang Timing
***************

For protocols with strict timing requirements (WS2812, soft-UART, etc.),
drivers use ``GPIO_FAST_DELAY_NOPS()`` to emit compile-time NOP delays between
``gpio_fast_set()`` and ``gpio_fast_clear()`` calls. The macro accepts any
C constant expression, so NOP counts can be computed using
``GPIO_FAST_NS_TO_CYCLES()`` with overhead subtracted.

Cycle-cost constants are accessed via ``GPIO_FAST_DISPATCH_VAL(compat, sym)``,
which resolves to the correct backend's constant (e.g.
``GPIO_FAST_SET_CYCLES_nordic_nrf_gpio``).

Drivers should call ``gpio_fast_pre_stream_<compat>()`` before and
``gpio_fast_post_stream_<compat>()`` after any timing-critical transmission
(via ``GPIO_FAST_DISPATCH_CALL``) to allow vendors to enable clocks or
perform other setup.

Timing-critical functions must be placed in RAM on platforms where flash execution
corrupts NOP timing. Each backend defines
``GPIO_FAST_SEND_STREAM_ATTR_<compat>`` (accessed via
``GPIO_FAST_DISPATCH_VAL(compat, GPIO_FAST_SEND_STREAM_ATTR)``) as ``__ramfunc``
when RAM execution is required, or as empty when it is not. Currently required on
SAM0 (no instruction cache, flash wait states add jitter) and RP2040 (XIP cache too
small for large NOP blocks, causing cache thrashing).

.. code-block:: c

   #define MY_NODE    DT_ALIAS(my_pin)
   #define MY_BACKEND GPIO_FAST_COMPAT(MY_NODE, gpios)

   #define SET_CYCLES   GPIO_FAST_DISPATCH_VAL(MY_BACKEND, GPIO_FAST_SET_CYCLES)
   #define CLEAR_CYCLES GPIO_FAST_DISPATCH_VAL(MY_BACKEND, GPIO_FAST_CLEAR_CYCLES)

   /* Fail the build if set/clear ops are too slow for WS2812 timing. */
   BUILD_ASSERT(SET_CYCLES < GPIO_FAST_NS_TO_CYCLES(400),
                "gpio_fast_set is too slow for WS2812 T0H (400 ns)");
   BUILD_ASSERT(CLEAR_CYCLES < GPIO_FAST_NS_TO_CYCLES(450),
                "gpio_fast_clear is too slow for WS2812 T1L (450 ns)");

   /* NOP counts from Kconfig, computed from CPU clock frequency. */
   static GPIO_FAST_DISPATCH_VAL(MY_BACKEND, GPIO_FAST_SEND_STREAM_ATTR) int
   ws2812_send_buf(const struct GPIO_FAST_DISPATCH_TYPE(MY_BACKEND) *spec,
                   const uint8_t *buf, size_t len)
   {
       unsigned int key;

       /* Set up any platform-specific state before the stream (e.g. enable GPIO clock). */
       GPIO_FAST_DISPATCH_CALL(MY_BACKEND, gpio_fast_pre_stream, spec);

       /* Lock out interrupts for the duration of the stream to ensure timing is not disrupted. */
       key = irq_lock();

       /* Bit-bang each byte in the buffer. */
       while (len--) {
           uint8_t byte = *buf++;
           for (int i = 7; i >= 0; i--) {
               if (byte & BIT(i)) {
                   /* Set pin HIGH, delay for T1H, set LOW, delay for T1L. */
                   GPIO_FAST_DISPATCH_CALL(MY_BACKEND, gpio_fast_set, spec);
                   GPIO_FAST_DELAY_NOPS(T1H_NOPS);
                   GPIO_FAST_DISPATCH_CALL(MY_BACKEND, gpio_fast_clear, spec);
                   GPIO_FAST_DELAY_NOPS(T1L_NOPS);
               } else {
                   /* Set pin HIGH, delay for T0H, set LOW, delay for T0L. */
                   GPIO_FAST_DISPATCH_CALL(MY_BACKEND, gpio_fast_set, spec);
                   GPIO_FAST_DELAY_NOPS(T0H_NOPS);
                   GPIO_FAST_DISPATCH_CALL(MY_BACKEND, gpio_fast_clear, spec);
                   GPIO_FAST_DELAY_NOPS(T0L_NOPS);
               }
           }
       }

       /* Re-enable interrupts after the stream. */
       irq_unlock(key);

       /* Perform any platform-specific cleanup after the stream. */
       GPIO_FAST_DISPATCH_CALL(MY_BACKEND, gpio_fast_post_stream, spec);
       return 0;
   }

Vendor Support
**************

Vendor-specific implementations are automatically selected at compile time
based on the DT compatible of the GPIO controller referenced by the consumer's
``gpios`` property. Each vendor provides a uniquely-named backend header
(e.g. ``zephyr_gpio_fast_nordic_nrf_gpio.h``) in ``drivers/gpio/fast/<vendor>/``.
The build system adds active vendor directories to the include path.

Multiple backends can coexist in a single build. The dispatch macros use
``DT_BINDING_COMPAT()`` to token-paste the controller's compatible onto the
backend symbol, so no per-vendor enumeration is needed in the dispatch logic.

Consumers can override the backend by setting the ``fast-gpio-backend``
string property on their DT node (e.g. ``fast-gpio-backend = "generic"``).
When set, the dispatch macros use this value instead of the GPIO controller's
DT compatible. This is how the generic fallback is selected: include
``gpio-fast.yaml`` in the consumer's DT binding, then set the property in
the board overlay or DTS.

.. list-table::
   :header-rows: 1

   * - Platform
     - Kconfig
     - Backend directory
     - DT compatible token
   * - Atmel SAM0
     - ``CONFIG_GPIO_SAM0``
     - ``drivers/gpio/fast/atmel/``
     - ``atmel_sam0_gpio``
   * - Espressif ESP32
     - ``CONFIG_GPIO_ESP32``
     - ``drivers/gpio/fast/espressif/``
     - ``espressif_esp32_gpio``
   * - Nordic nRF
     - ``CONFIG_GPIO_NRFX``
     - ``drivers/gpio/fast/nordic/``
     - ``nordic_nrf_gpio``
   * - NXP i.MX RT (Teensy 4.x)
     - ``CONFIG_GPIO_MCUX_IGPIO``
     - ``drivers/gpio/fast/nxp/``
     - ``nxp_imx_gpio``
   * - Raspberry Pi Pico (RP2040/RP2350)
     - ``CONFIG_GPIO_RPI_PICO``
     - ``drivers/gpio/fast/raspberrypi/``
     - ``raspberrypi_pico_gpio``
   * - STM32
     - ``CONFIG_GPIO_STM32``
     - ``drivers/gpio/fast/st/``
     - ``st_stm32_gpio``
   * - Generic fallback
     - Always included
     - ``drivers/gpio/fast/generic/``
     - Selected per-node via ``fast-gpio-backend = "generic"`` in DT.
       Wraps standard GPIO API. **Not cycle-accurate.**
       Can be used to write tests.

.. note::

   Drivers should use ``BUILD_ASSERT`` with ``GPIO_FAST_NS_TO_CYCLES`` and
   ``GPIO_FAST_DISPATCH_VAL(compat, GPIO_FAST_*_CYCLES)`` to verify timing
   requirements are met on each platform at compile time.

Adding a New Vendor
*******************

In-tree
=======

1. Create ``drivers/gpio/fast/<vendor>/zephyr_gpio_fast_<compat>.h``
   implementing ``struct gpio_fast_spec_<compat>`` and all six inline
   operations (``gpio_fast_set_<compat>``, ``gpio_fast_clear_<compat>``,
   ``gpio_fast_toggle_<compat>``, ``gpio_fast_get_<compat>``,
   ``gpio_fast_set_input_<compat>``, ``gpio_fast_set_output_<compat>``).
   Define all ``GPIO_FAST_*_CYCLES_<compat>`` constants and
   ``GPIO_FAST_SEND_STREAM_ATTR_<compat>`` (set to ``__ramfunc`` if the
   platform requires RAM execution, or empty if not).
   ``<compat>`` is the GPIO controller's DT compatible with commas and
   hyphens replaced by underscores (e.g. ``nordic_nrf_gpio``).
2. Implement ``gpio_fast_configure_<compat>()``,
   ``gpio_fast_pre_stream_<compat>()``, and
   ``gpio_fast_post_stream_<compat>()`` in the vendor driver ``.c``
   file behind ``#ifdef CONFIG_GPIO_FAST``. The pre/post stream hooks
   handle vendor-specific setup and teardown (e.g. enabling a
   high-frequency clock).
3. Add an ``if(CONFIG_GPIO_*)`` block to
   ``drivers/gpio/CMakeLists.txt`` that calls
   ``zephyr_include_directories(fast/<vendor>)``.
4. Add a ``#ifdef CONFIG_GPIO_*`` / ``#include`` block
   to ``gpio_fast.h``.
5. Plug in ten Neopixels and use the :zephyr:code-sample:`led-strip` sample
   to verify correct operation. If the LEDs light up solid white, then the
   timing is likely wrong and you should adjust ``GPIO_FAST_*_CYCLES_<compat>``.

Out-of-tree
===========

Out-of-tree vendors follow the same header and naming conventions as in-tree.
The backend header lives in the vendor's Zephyr module instead of under
``drivers/gpio/fast/``. The module must:

1. Provide a ``zephyr_gpio_fast_impl_oot.h`` that ``#include``\s the OOT
   backend header. Place it in a directory added to the include path
   using ``zephyr_include_directories()`` in the module's ``CMakeLists.txt``.
2. Enable ``CONFIG_GPIO_FAST_OUT_OF_TREE_BACKEND=y`` in the module's
   Kconfig (or the application's ``prj.conf``).

The dispatch macros automatically resolve to the backend via token-paste
on the DT compatible. No other changes to core Zephyr files are required.

.. note::

   **Important:** If the first LED of the strip briefly shows green when the
   MCU first boots, then you likely need to define
   ``GPIO_FAST_SEND_STREAM_ATTR_<compat>`` as ``__ramfunc`` or the equivalent for
   your platform in your backend header to ensure timing-critical code runs from
   RAM instead of flash. The :zephyr:code-sample:`led-strip` sample always starts
   with the first LED red. It is highly recommended to test with a strip of at
   least ten LEDs because it is very hard to catch this with only one LED.

No changes are needed to consumer drivers to use out-of-tree backends.

Configuration Options
*********************

* :kconfig:option:`CONFIG_GPIO_FAST`
* :kconfig:option:`CONFIG_GPIO_FAST_OUT_OF_TREE_BACKEND`

API Reference
*************

.. doxygengroup:: gpio_fast_interface
