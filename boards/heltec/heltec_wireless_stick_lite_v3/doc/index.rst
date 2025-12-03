.. zephyr:board:: heltec_wireless_stick_lite_v3

Overview
********

HelTec Wireless Stick Lite (V3) is a development board with Wi-Fi, Bluetooth and LoRa support.
It is designed and produced by HelTec Automation(TM). See the `Heltec Wireless Stick Lite (v3) pages`_ for more details.

Hardware
********

The main hardware features are:

- ESP32-S3FN8 low-power MCU-based SoC (dual-core Xtensa® 32-bit LX7 microprocessor, five stage pipeline rack Structure, main frequency up to 240 MHz).
- Semtech SX1262 LoRa node chip
- Type-C USB interface with a complete voltage regulator, ESD protection, short circuit protection, RF shielding, and other protection measures (note: you need an USB-A to USB-C cable if you want to power-up the board from USB).
- Onboard SH1.25-2 battery interface, integrated lithium battery management system (charge and discharge management, overcharge protection, battery power detection, USB / battery power automatic switching).
- Integrated WiFi and Bluetooth interfaces with 2.4GHz metal spring antenna and reserved IPEX (U.FL) interface for LoRa use.
- Integrated CP2102 USB to serial port chip, convenient for program downloading, debugging information printing.
- Good RF circuit design and low-power design.

.. include:: ../../../espressif/common/soc-esp32s3-features.rst
   :start-after: espressif-soc-esp32s3-features

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

.. figure:: heltec_wireless_stick_lite_v3_pinout.webp
   :width: 600px
   :align: center
   :alt: HelTec Wireless Stick Lite (V3) Pinout

   Pinout (Credit: Chengdu HelTec Automation Technology Co., Ltd.)

.. table:: HelTec Wireless Stick Lite (V3) Pinout
   :widths: auto

   +--------+---------+-----------------------------+
   | Header | Function| Description                 |
   +========+=========+=============================+
   | J2.1   | Ve      |                             |
   +--------+---------+-----------------------------+
   | J2.2   | GND     |                             |
   +--------+---------+-----------------------------+
   | J2.3   |         |                             |
   +--------+---------+-----------------------------+
   | J2.4   | U0RXD   | Zephyr Console+Shell        |
   +--------+---------+-----------------------------+
   | J2.5   | U0TXD   | Zephyr Console+Shell        |
   +--------+---------+-----------------------------+
   | J2.6   |         |                             |
   +--------+---------+-----------------------------+
   | J2.7   |         |                             |
   +--------+---------+-----------------------------+
   | J2.8   | GPIO35  | PWM LED Control             |
   +--------+---------+-----------------------------+
   | J2.9   | GPIO36  | Vext Control                |
   +--------+---------+-----------------------------+
   | J2.10  | GPIO37  | ADC Control                 |
   +--------+---------+-----------------------------+
   | J2.11  |         |                             |
   +--------+---------+-----------------------------+
   | J2.12  | GPIO39  |                             |
   +--------+---------+-----------------------------+
   | J2.13  | GPIO40  |                             |
   +--------+---------+-----------------------------+
   | J2.14  | GPIO41  |                             |
   +--------+---------+-----------------------------+
   | J2.15  | GPIO42  |                             |
   +--------+---------+-----------------------------+
   | J2.16  | GPIO45  |                             |
   +--------+---------+-----------------------------+
   | J2.17  | GPIO46  |                             |
   +--------+---------+-----------------------------+
   | J2.18  | ADC1_CH0| Battery Voltage Measurement |
   +--------+---------+-----------------------------+
   | J2.19  |         |                             |
   +--------+---------+-----------------------------+
   | J2.20  |         |                             |
   +--------+---------+-----------------------------+
   | J3.1   | 5V      |                             |
   +--------+---------+-----------------------------+
   | J3.2   | 3V3     |                             |
   +--------+---------+-----------------------------+
   | J3.3   | GND     |                             |
   +--------+---------+-----------------------------+
   | J3.4   | GPIO47  |                             |
   +--------+---------+-----------------------------+
   | J3.5   | GPIO48  |                             |
   +--------+---------+-----------------------------+
   | J3.6   | GPIO0   | User Switch                 |
   +--------+---------+-----------------------------+
   | J3.7   |         |                             |
   +--------+---------+-----------------------------+
   | J3.8   |         |                             |
   +--------+---------+-----------------------------+
   | J3.9   | U1RXD   | UART 1                      |
   +--------+---------+-----------------------------+
   | J3.10  | GPIO21  |                             |
   +--------+---------+-----------------------------+
   | J3.11  |         |                             |
   +--------+---------+-----------------------------+
   | J3.12  | U1TXD   | UART 1                      |
   +--------+---------+-----------------------------+
   | J3.13  |         |                             |
   +--------+---------+-----------------------------+
   | J3.14  | NC      | Reset Switch                |
   +--------+---------+-----------------------------+
   | J3.15  |         |                             |
   +--------+---------+-----------------------------+
   | J3.16  |         |                             |
   +--------+---------+-----------------------------+
   | J3.17  |         |                             |
   +--------+---------+-----------------------------+
   | J3.18  |         |                             |
   +--------+---------+-----------------------------+
   | J3.19  | TWAI_TX | CAN (optional)              |
   +--------+---------+-----------------------------+
   | J3.20  | TWAI_RX | CAN (optional)              |
   +--------+---------+-----------------------------+

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

.. _`Heltec Wireless Stick Lite (v3) pages`: https://heltec.org/project/wireless-stick-lite-v2/
.. _`Heltec Wireless Stick Lite (v3) Pinout Diagram`: https://resource.heltec.cn/download/Wireless_Stick_Lite_V3/HTIT-WSL_V3.png
.. _`Heltec Wireless Stick Lite (v3) Schematic Diagrams`: https://resource.heltec.cn/download/Wireless_Stick_Lite_V3/HTIT-WSL_V3_Schematic_Diagram.pdf
.. _`ESP-IDF Programming Guide`: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/index.html
