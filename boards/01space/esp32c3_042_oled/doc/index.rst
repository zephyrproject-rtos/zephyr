.. zephyr:board:: esp32c3_042_oled

Overview
********

ESP32-C3 0.42 OLED is a mini development board based on the `Espressif ESP32-C3`_
RISC-V WiFi/Bluetooth dual-mode chip.

For more details see the `01space ESP32C3 0.42 OLED`_ Github repo.

Hardware
********

This board is based on the ESP32-C3-FH4 with WiFi and BLE support.
It features:

* RISC-V SoC @ 160MHz with 4MB flash and 400kB RAM
* WS2812B RGB serial LED
* 0.42-inch OLED over I2C
* Qwiic I2C connector
* One pushbutton
* Onboard ceramic chip antenna
* On-chip USB-UART converter

.. note::

   The RGB led is not supported on this Zephyr board yet.

.. note::

   The ESP32-C3 does not have native USB, it has an on-chip USB-serial converter
   instead.

.. include:: ../../../espressif/common/soc-esp32c3-features.rst
   :start-after: espressif-soc-esp32c3-features

Connections and IOs
===================

See the following image:

.. figure:: img/esp32c3_042_oled_pinout.webp
   :align: center
   :alt: 01space ESP32C3 0.42 OLED Pinout

   01space ESP32C3 0.42 OLED Pinout

It also features a 0.42 inch OLED display, driven by a SSD1306-compatible chip.
It is connected over I2C: SDA on GPIO5, SCL on GPIO6.

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

.. _`Espressif ESP32-C3`: https://www.espressif.com/en/products/socs/esp32-c3
.. _`01space ESP32C3 0.42 OLED`: https://github.com/01Space/ESP32-C3-0.42LCD
