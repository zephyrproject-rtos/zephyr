.. zephyr:board:: m5stack_nanoc6

Overview
********

M5Stack NanoC6 is an ESP32-based development board from M5Stack.

Hardware
********

M5Stack NanoC6 features consist of:

- ESP32-C6 chip (incl. WiFi, Bluetooth, Thread/Zigbee)
- SRAM 512kB + 16kB
- Flash 4MB
- WS2812
- IR
- Button
- Grove connector

.. include:: ../../../espressif/common/soc-esp32c6-features.rst
   :start-after: espressif-soc-esp32c6-features

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

References
**********

.. target-notes::

.. _`M5Stack NanoC6 Documentation`: https://docs.m5stack.com/en/core/M5NanoC6
.. _`M5Stack NanoC6 Schematic`: https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/505/Sch_M5NanoC6_v0.0.1.pdf
