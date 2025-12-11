.. zephyr:board:: heltec_wifi_lora32_v2

Overview
********

Heltec WiFi LoRa 32 is a classic IoT dev-board designed & produced by Heltec Automation(TM), it's a highly
integrated product based on ESP32 + SX127x, it has Wi-Fi, BLE, LoRa functions, also Li-Po battery management
system, 0.96" OLED are also included. See the `Heltec WiFi LoRa (V2) pages`_ for more details.

The features include the following:

- Microprocessor: ESP32 (dual-core 32-bit MCU + ULP core)
- LoRa node chip SX1276/SX1278
- Micro USB interface with a complete voltage regulator, ESD protection, short circuit protection,
  RF shielding, and other protection measures
- Onboard SH1.25-2 battery interface, integrated lithium battery management system
- Integrated WiFi, LoRa, Bluetooth three network connections, onboard Wi-Fi, Bluetooth dedicated 2.4GHz
  metal 3D antenna, reserved IPEX (U.FL) interface for LoRa use
- Onboard 0.96-inch 128*64 dot matrix OLED display
- Integrated CP2102 USB to serial port chip

Hardware
********

.. include:: ../../../espressif/common/soc-esp32-features.rst
   :start-after: espressif-soc-esp32-features

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

Utilizing Hardware Features
***************************

Onboard OLED display
====================

The onboard OLED display is of type ``ssd1306``, has 128*64 pixels and is
connected via I2C. It can therefore be used by enabling the
:ref:`ssd1306_128_shield` as shown in the following for the :zephyr:code-sample:`lvgl` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/display/lvgl
   :board: heltec_wifi_lora32_v2/esp32/procpu
   :shield: ssd1306_128x64
   :goals: flash

References
**********

.. target-notes::

.. _`Heltec WiFi LoRa (V2) pages`: https://heltec.org/project/wifi-lora-32/
.. _`Heltec WiFi LoRa (v2) Pinout Diagram`: https://resource.heltec.cn/download/WiFi_LoRa_32/WIFI_LoRa_32_V2.pdf
.. _`Heltec WiFi LoRa (v2) Schematic Diagrams`: https://resource.heltec.cn/download/WiFi_LoRa_32/V2
