.. zephyr:board:: m5stack_cores3

Overview
********

M5Stack CoreS3 is an ESP32-based development board from M5Stack. It is the third generation of the M5Stack Core series.
M5Stack CoreS3 SE is the compact version of CoreS3. It has the same form factor as the original M5Stack,
and some features were reduced from CoreS3.

Hardware
********

M5Stack CoreS3/CoreS3 SE features consist of:

- ESP32-S3 chip (dual-core Xtensa LX7 processor @240MHz, WIFI, OTG and CDC functions)
- PSRAM 8MB
- Flash 16MB
- LCD ISP 2", 320x240 pixel ILI9342C
- Capacitive multi touch FT6336U
- Speaker 1W AW88298
- Dual Microphones ES7210 Audio decoder
- RTC BM8563
- USB-C
- SD-Card slot
- PMIC AXP2101
- Battery 500mAh 3.7 V (Not available for CoreS3 SE)
- Camera 30W pixel GC0308 (Not available for CoreS3 SE)
- Geomagnetic sensor BMM150 (Not available for CoreS3 SE)
- Proximity sensor LTR-553ALS-WA (Not available for CoreS3 SE)
- 6-Axis IMU BMI270 (Not available for CoreS3 SE)

.. include:: ../../../espressif/common/soc-esp32s3-features.rst
   :start-after: espressif-soc-esp32s3-features

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

.. _`M5Stack CoreS3 Documentation`: http://docs.m5stack.com/en/core/CoreS3
.. _`M5Stack CoreS3 Schematic`: https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/datasheet/core/K128%20CoreS3/Sch_M5_CoreS3_v1.0.pdf
.. _`M5Stack CoreS3 SE Documentation`: https://docs.m5stack.com/en/core/M5CoreS3%20SE
.. _`M5Stack CoreS3 SE Schematic`: https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/products/core/M5CORES3%20SE/M5_CoreS3SE.pdf
