.. zephyr:board:: esp32s3_touch_lcd_1_47

Overview
********

The `ESP32-S3-Touch-LCD-1.47`_ is an ESP32-S3 development board from Waveshare with a 1.47-inch
172x320 IPS LCD, capacitive touch, a microSD card slot and a Li-Po battery charger.

The schematic and pinout can be found in the `ESP32-S3-Touch-LCD-1.47 Schematic`_ reference.

Hardware
********

- ESP32-S3R8 SoC (8 MB octal PSRAM) with 16 MB external flash
- 1.47-inch 172x320 IPS LCD driven over SPI (JD9853 controller)
- Capacitive touch panel (AXS5106L controller)
- microSD card slot (4-bit SDMMC)
- ETA6098 Li-Po battery charger with battery voltage measurement
- USB Type-C connector for power, flashing and console (USB Serial/JTAG)
- BOOT and RESET buttons

.. include:: ../../../espressif/common/soc-esp32s3-features.rst
   :start-after: espressif-soc-esp32s3-features

Supported Features
==================

.. zephyr:board-supported-hw::

The JD9853 display controller is driven by the ``sitronix,st7789v`` driver. The LCD backlight is
connected to a PWM channel (``pwm-lcd0`` alias) and is off until the application turns it on.

The AXS5106L touch controller is not supported yet. Its I2C bus is shared with the ``SCL`` and
``SDA`` pins of the pin header and is enabled as ``i2c0``.

The battery voltage is measured on ADC2, which is shared with the Wi-Fi radio. Readings may fail
while Wi-Fi is in use.

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

.. _`ESP32-S3-Touch-LCD-1.47`: https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-1.47
.. _`ESP32-S3-Touch-LCD-1.47 Schematic`: https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-1.47/ESP32-S3-Touch-LCD-1.47-Schematic.pdf
