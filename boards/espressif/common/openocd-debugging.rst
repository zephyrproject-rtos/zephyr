:orphan:

.. espressif-openocd-debugging

OpenOCD Debugging
=================

Espressif chips require a custom OpenOCD build with ESP32-specific patches.
Download the latest release from `OpenOCD for ESP32`_.

For detailed JTAG setup instructions, see `JTAG debugging for ESP32`_.

Zephyr Thread Awareness
-----------------------

OpenOCD supports Zephyr RTOS thread awareness, allowing GDB to:

- List all threads with ``info threads``
- Display thread names, priorities, and states
- Switch between thread contexts
- Show backtraces for any thread

**Requirements:**

- `OpenOCD ESP32 v0.12.0-esp32-20251215`_ or later
- Build with ``CONFIG_DEBUG_THREAD_INFO=y``

**Example:**

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: <board>
   :goals: debug
   :gen-args: -DCONFIG_DEBUG_THREAD_INFO=y -DOPENOCD=<path/to/bin/openocd> -DOPENOCD_DEFAULT_PATH=<path/to/openocd/share/openocd/scripts>

Using a Custom OpenOCD
----------------------

The Zephyr SDK includes a bundled OpenOCD, but it may not have ESP32 support.
To use the Espressif OpenOCD, specify the path when building:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: <board>
   :goals: debug
   :gen-args: -DOPENOCD=/path/to/openocd -DOPENOCD_DEFAULT_PATH=/path/to/openocd/scripts


.. _`OpenOCD for ESP32`: https://github.com/espressif/openocd-esp32/releases
.. _`OpenOCD ESP32 v0.12.0-esp32-20251215`: https://github.com/espressif/openocd-esp32/releases/tag/v0.12.0-esp32-20251215
.. _`JTAG debugging for ESP32`: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/jtag-debugging/index.html
