.. zephyr:board:: m5stack_unitc6l

Overview
********

M5Stack Unit6L is a compact development board built around the Stamp C6LoRa module
with an ESP32-C6 SoC and 16 MB SPI flash. It integrates Wi-Fi 6, Bluetooth LE,
IEEE 802.15.4 (Thread/Zigbee), and an SX1262 LoRa transceiver. For more
information, see the `M5Stack Unit C6L documentation`_.

Hardware
********

M5Stack Unit6L features:

- ESP32-C6 Stamp C6LoRa module (16 MB flash)
- USB Type-C console and programming via the built-in USB Serial/JTAG interface
- 2.4 GHz Wi-Fi 6 and Bluetooth LE
- IEEE 802.15.4 radio for Thread/Zigbee
- SX1262 LoRa transceiver (868/915 MHz)
- 0.66" SSD1306 OLED display (64 x 48, SPI)
- WS2812C RGB LED (GPIO2, I2S)
- Passive buzzer (GPIO11, PWM)
- User button (PI4IOE5V6408 P0)
- HY2.0-4P Grove Port A (I2C on GPIO4/GPIO5 via bit-banged bus)

The OLED shares SPI2 with the SX1262 LoRa modem (MOSI/SCK) and uses a
separate chip select (GPIO6), data/command (GPIO18), and reset (GPIO15).

.. include:: ../../../espressif/common/soc-esp32c6-features.rst
   :start-after: espressif-soc-esp32c6-features

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

.. _`M5Stack Unit C6L documentation`: https://docs.m5stack.com/en/unit/Unit_C6L
