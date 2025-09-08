.. zephyr:board:: yd_esp32

Overview
********

The YD-ESP32 development board is one of VCC-GND® Studio's official boards.
This board is based on the ESP32-WROOM-32E module, with the ESP32 as the core.

Hardware
********

.. include:: ../../../espressif/common/soc-esp32-features.rst
   :start-after: espressif-soc-esp32-features

Supported Features
==================

.. zephyr:board-supported-hw::

System Requirements
*******************

.. include:: ../../../espressif/common/system-requirements.rst
   :start-after: espressif-system-requirements

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

.. include:: ../../../espressif/common/building-flashing.rst
   :start-after: espressif-building-flashing

.. include:: ../../../espressif/common/board-variants.rst
   :start-after: espressif-board-variants

Debugging
=========

.. include:: ../../../espressif/common/openocd-debugging.rst
   :start-after: espressif-openocd-debugging

On the YD-ESP32 board, the JTAG pins are not run to a
standard connector (e.g. ARM 20-pin) and need to be manually connected
to the external programmer (e.g. a Flyswatter2):

+------------+-----------+
| ESP32 pin  | JTAG pin  |
+============+===========+
| 3V3        | VTRef     |
+------------+-----------+
| EN         | nTRST     |
+------------+-----------+
| IO14       | TMS       |
+------------+-----------+
| IO12       | TDI       |
+------------+-----------+
| GND        | GND       |
+------------+-----------+
| IO13       | TCK       |
+------------+-----------+
| IO15       | TDO       |
+------------+-----------+

Note on Debugging with GDB Stub
===============================

GDB stub is enabled on ESP32.

* When adding breakpoints, please use hardware breakpoints with command
  ``hbreak``. Command ``break`` uses software breakpoints which requires
  modifying memory content to insert break/trap instructions.
  This does not work as the code is on flash which cannot be randomly
  accessed for modification.


Sample applications
*******************

RGB LED
=======

The board contains an addressable RGB LED (`XL-5050RGBC-WS2812B`_), driven by GPIO16.
Here is an example of how to test it using the :zephyr:code-sample:`led-strip` application.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/led/led_strip
   :board: yd_esp32/esp32/procpu
   :goals: flash

.. _`XL-5050RGBC-WS2812B`: http://www.xinglight.cn/index.php?c=show&id=947

References
**********

.. target-notes::

.. _`ESP32-DevKitC-WROVER`: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/hw-reference/esp32/get-started-devkitc.html#
