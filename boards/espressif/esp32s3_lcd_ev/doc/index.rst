.. zephyr:board:: esp32s3_lcd_ev

Overview
********

The ESP32-S3-LCD-EV-Board is a development kit from Espressif built around the
ESP32-S3-WROOM-1 module. This Zephyr board target supports the **Board-2**
mainboard with the **SUB3** 800×480 RGB LCD daughterboard (Sitronix ST7262E43
panel and Goodix GT1151 touch controller).

Hardware
********

.. include:: ../../../espressif/common/soc-esp32s3-features.rst
   :start-after: espressif-soc-esp32s3-features

On-board peripherals enabled by default:

* UART0 console on GPIO43/44
* 800×480 RGB565 LCD on the LCD-CAM parallel interface
* GT1151 capacitive touch on I2C address 0x14
* TCA9554 I/O expander on I2C address 0x20
* USB OTG, Wi-Fi, and Bluetooth via the PROCPU image

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

.. _`ESP32-S3-LCD-EV-Board User Guide`: https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-lcd-ev-board/user_guide.html
