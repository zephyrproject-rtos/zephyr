.. zephyr:board:: esp32s2_devkitc

Overview
********

ESP32-S2-DevKitC is an entry-level development board. This board integrates complete Wi-Fi functions.
Most of the I/O pins are broken out to the pin headers on both sides for easy interfacing.
Developers can either connect peripherals with jumper wires or mount ESP32-S2-DevKitC on a breadboard.
For more information, check `ESP32-S2-DevKitC`_.

Hardware
********

.. include:: ../../../espressif/common/soc-esp32s2-features.rst
   :start-after: espressif-soc-esp32s2-features

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

The following table shows the pin mapping between ESP32-S2 board and JTAG interface.

+---------------+-----------+
| ESP32 pin     | JTAG pin  |
+===============+===========+
| MTDO / GPIO40 | TDO       |
+---------------+-----------+
| MTDI / GPIO41 | TDI       |
+---------------+-----------+
| MTCK / GPIO39 | TCK       |
+---------------+-----------+
| MTMS / GPIO42 | TMS       |
+---------------+-----------+

References
**********

.. target-notes::

.. _`ESP32-S2-DevKitC`: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s2/hw-reference/esp32s2/user-guide-saola-1-v1.2.html
