.. zephyr:board:: heltec_wifi_lora32_v3

Overview
********

Heltec WiFi LoRa 32 is a classic IoT dev-board designed & produced by Heltec Automation(TM), it's a highly
integrated product based on ESP32-S3 + SX1262, it has Wi-Fi, BLE, LoRa functions, also Li-Po battery management
system, 0.96" OLED are also included. See the `Heltec WiFi LoRa (V3) pages`_ for more details.

The features include the following:

- ESP32-S3FN8 low-power MCU-based SoC (dual-core 32-bit MCU + ULP core)
- Semtech SX1262 LoRa node chip
- Type-C USB interface with a complete voltage regulator, ESD protection, short circuit protection,
  RF shielding, and other protection measures
- Onboard SH1.25-2 battery interface, integrated lithium battery management system
- Integrated WiFi, LoRa, Bluetooth three network connections, onboard Wi-Fi, Bluetooth dedicated 2.4GHz
  metal 3D antenna, reserved IPEX (U.FL) interface for LoRa use
- Onboard 0.96-inch 128*64 dot matrix OLED display
- Integrated CP2102 USB to serial port chip

Hardware
********

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

.. _`Heltec WiFi LoRa (V3) pages`: https://heltec.org/project/wifi-lora-32-v3/
.. _`Heltec WiFi LoRa (v3) documentation`: https://resource.heltec.cn/download/WiFi_LoRa_32_V3
.. _`ESP-IDF Programming Guide`: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/index.html
