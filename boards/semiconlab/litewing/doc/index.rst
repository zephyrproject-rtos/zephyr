.. zephyr:board:: litewing

Overview
********

LiteWing is a compact, WiFi-controlled drone based on the ESP32-S3 microcontroller. Designed for hobbyists, makers, and engineers, LiteWing offers a simple yet powerful platform for drone experimentation and development. It is an open-hardware project, making it easy to modify and expand.

Hardware
********

.. include:: ../../../espressif/common/soc-esp32s3-features.rst
   :start-after: espressif-soc-esp32s3-features

- 4x Coreless 720 DC Rotors
- MPU6050 IMU
- 1S 1000-mAh LiPo Cell
- Expansion Interface
   - 1x I2C
   - 1x SPI
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

.. _`LiteWing`: https://circuitdigest.com/wiki/litewing/
.. _`Schematics`: https://github.com/Makerfabs/MaTouch-ESP32-S3-RotaryIPS-Display1.28-GC9A01
.. _`Kicad Board Design Files`: https://github.com/Circuit-Digest/LiteWing/blob/be00f916ec33d5a4a76cdd4e53558e346165428c/hardware/LiteWingV2.5C/DroneV2.5C_Schematics.pdf
