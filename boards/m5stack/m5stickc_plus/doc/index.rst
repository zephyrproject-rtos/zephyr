.. zephyr:board:: m5stickc_plus

Overview
********

M5StickC PLUS, one of the core devices in M5Stacks product series, is an ESP32-based development board.

Hardware
********

M5StickC PLUS features the following integrated components:

- ESP32-PICO-D4 chip (240MHz dual core, 600 DMIPS, 520KB SRAM, Wi-Fi)
- ST7789v2, LCD TFT 1.14", 135x240 px screen
- IMU MPU-6886
- SPM-1423 microphone
- RTC BM8563
- PMU AXP192
- 120 mAh 3,7 V battery

Some of the ESP32 I/O pins are broken out to the board's pin headers for easy access.

.. include:: ../../../espressif/common/soc-esp32-features.rst
   :start-after: espressif-soc-esp32-features

Supported Features
==================

.. zephyr:board-supported-hw::

Functional Description
======================

The following table below describes the key components, interfaces, and controls
of the M5StickC PLUS board.

.. _ST7789v2: https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/datasheet/core/ST7789V.pdf
.. _MPU-6886: https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/datasheet/core/MPU-6886-000193%2Bv1.1_GHIC_en.pdf
.. _ESP32-PICO-D4: https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/datasheet/core/esp32-pico-d4_datasheet_en.pdf
.. _SPM-1423: https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/datasheet/core/SPM1423HM4H-B_datasheet_en.pdf

+------------------+-------------------------------------------------------------------------+
| Key Component    | Description                                                             |
+==================+=========================================================================+
| 32.768 kHz RTC   | External precision 32.768 kHz crystal oscillator serves as a clock with |
|                  | low-power consumption while the chip is in Deep-sleep mode.             |
+------------------+-------------------------------------------------------------------------+
| ESP32-PICO-D4    | This `ESP32-PICO-D4`_ module provides complete Wi-Fi and Bluetooth      |
| module           | functionalities and integrates a 4-MB SPI flash.                        |
+------------------+-------------------------------------------------------------------------+
| Diagnostic LED   | One user LED connected to the GPIO pin.                                 |
+------------------+-------------------------------------------------------------------------+
| USB Port         | USB interface. Power supply for the board as well as the                |
|                  | communication interface between a computer and the board.               |
|                  | Contains: TypeC x 1, GROVE(I2C+I/O+UART) x 1                            |
+------------------+-------------------------------------------------------------------------+
| Power Switch     | Power on/off button.                                                    |
+------------------+-------------------------------------------------------------------------+
| A/B user buttons | Two push buttons intended for any user use.                             |
+------------------+-------------------------------------------------------------------------+
| LCD screen       | Built-in LCD TFT display \(`ST7789v2`_, 1.14", 135x240 px\) controlled  |
|                  | by the SPI interface                                                    |
+------------------+-------------------------------------------------------------------------+
| MPU-6886         | The `MPU-6886`_ is a 6-axis MotionTracking device that combines a       |
|                  | 3-axis gyroscope and a 3-axis accelerometer.                            |
+------------------+-------------------------------------------------------------------------+
| Built-in         | The `SPM-1423`_ I2S driven microphone.                                  |
| microphone       |                                                                         |
+------------------+-------------------------------------------------------------------------+

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

M5StickC PLUS debugging is not supported due to pinout limitations.

References
**********

.. target-notes::

.. _`M5StickC PLUS schematic`: https://static-cdn.m5stack.com/resource/docs/products/core/m5stickc_plus/m5stickc_plus_sch_03.webp
.. _`ESP32-PICO-D4 Datasheet`: https://www.espressif.com/sites/default/files/documentation/esp32-pico-d4_datasheet_en.pdf
.. _`M5StickC PLUS docs`: https://docs.m5stack.com/en/core/m5stickc_plus
.. _`ESP32 Hardware Reference`: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/hw-reference/index.html
