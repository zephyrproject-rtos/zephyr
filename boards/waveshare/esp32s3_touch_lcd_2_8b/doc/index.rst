.. zephyr:board:: esp32s3_touch_lcd_2_8b

Overview
********

The ESP32-S3-Touch-LCD-2.8B is an ESP32-S3 development board from Waveshare with a 2.8-inch
480x640 IPS RGB LCD, capacitive touch panel, IMU, RTC, microSD card slot and Li-Po battery
charger.

Hardware
********

ESP32-S3-Touch-LCD-2.8B features consist of:

- ESP32-S3R8 chip (dual-core Xtensa LX7 processor @240MHz, Wi-Fi, Bluetooth LE)
- PSRAM 8MB (octal)
- Flash 16MB
- LCD IPS 2.8", 480x640 pixel ST7701S (RGB interface)
- Capacitive touch GT911
- I/O expander TCA9554
- 6-Axis IMU QMI8658C
- RTC PCF85063A
- SD-Card slot (SPI mode)
- Battery charger with battery voltage measurement
- Buzzer
- BOOT and RESET buttons
- USB-C (USB Serial/JTAG)

.. include:: ../../../espressif/common/soc-esp32s3-features.rst
   :start-after: espressif-soc-esp32s3-features

Supported Features
==================

.. zephyr:board-supported-hw::

The RGB LCD panel is not supported yet. The LCD backlight is available as the ``pwm-lcd0``
PWM LED.

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

.. _`ESP32-S3-Touch-LCD-2.8B`: https://docs.waveshare.com/ESP32-S3-Touch-LCD-2.8B
