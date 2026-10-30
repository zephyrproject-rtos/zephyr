.. zephyr:code-sample:: fast_gpio
   :name: Fast GPIO with source set claiming

   Fuse the GPIO driver with the application hot path in one translation unit.

Overview
********

Loops over and toggles the led0 pin 16 times in a tight loop with a small delay between
each 16 toggle burst.

The toggle burst is done so that looping instructions are ensured to not effect timings
of toggles.

The sample showcases compilation techniques that ultimately result in inlined register writes
avoiding the costs of the function pointer table or internal driver functions.


Requirements
************

The board must define ``led0``. Building with amalgamation (claiming sources) requires
a GPIO driver that declares its sources into the ``gpio`` source set.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/gpio/fast_gpio
   :board: kit_pse84_eval/pse846gps2dbzc4a/m33
   :goals: build flash
   :compact:

To compare against the unfused baseline, disable claiming:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/gpio/fast_gpio
   :board: kit_pse84_eval/pse846gps2dbzc4a/m33
   :gen-args: -DCONFIG_FAST_GPIO_CLAIM=n
   :goals: build
   :compact:

Link time optimization composes with claiming and can be added with the
``lto.conf`` extra configuration file:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/gpio/fast_gpio
   :board: kit_pse84_eval/pse846gps2dbzc4a/m33
   :gen-args: -DEXTRA_CONF_FILE=lto.conf
   :goals: build
   :compact:

LTO alone devirtualizes the api calls but cannot inline them, because its
inliner runs before the vtable constant is folded; claiming fuses the
translation unit before LTO runs, so the two are independent and additive.

Sample Output
*************

.. code-block:: console

   fast_gpio: toggling gpio@52810780 pin 5

The pin output is the measurement: bursts of sixteen edges, roughly 10 ns
apart on kit_pse84_eval, separated by 20 us pauses.
