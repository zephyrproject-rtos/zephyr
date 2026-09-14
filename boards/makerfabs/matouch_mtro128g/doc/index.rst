.. zephyr:board:: matouch_mtro128g

Overview
********

The Makerfabs Matouch MTRO128G is a ESP32-S3 powered rotary encode knob with a 1.28 inch 240x240 color LCD display in the centre.

Hardware
********

.. include:: ../../../espressif/common/soc-esp32s3-features.rst
   :start-after: espressif-soc-esp32s3-features

- LCD: 240x240 GC9A01
- Capacitive Touch: CST816D
- Expansion Interface
   - 1x I2C
   - 1x UART
   - 2x GPIO

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

.. _`Makerfabs-Matouch-MTRO128G`: https://wiki.makerfabs.com/MaTouch_ESP32_S3_Rotary_IPS_Display_1.28_GC9A01.html
.. _`Schematics`: https://github.com/Makerfabs/MaTouch-ESP32-S3-RotaryIPS-Display1.28-GC9A01
