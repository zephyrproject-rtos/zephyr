.. zephyr:board:: esp_sensairshuttle

Overview
********

ESP-SensairShuttle is a development kit from Espressif and Bosch Sensortec for motion sensing,
environmental sensing and voice interaction. The mainboard is built around the
ESP32-C5-WROOM-1-N16R8 module, with 16 MB SPI flash and 8 MB PSRAM, and provides dual-band
Wi-Fi 6, Bluetooth LE, Zigbee and Thread. Bosch sensors are attached through a Shuttle Board 3.0
connector. For more information, check `ESP-SensairShuttle`_.

Hardware
********

The mainboard carries:

- ESP32-C5-WROOM-1-N16R8 module
- 1.83 inch 240x284 SPI LCD (ST7789P3) with capacitive touch (CST816T)
- Analog microphone amplifier connected to an ADC input
- Speaker connector driven by a differential PDM signal through an audio amplifier
- Shuttle Board 3.0 connector (9+7 pins, 1.27 mm) with a 3.3 V / 1.8 V supply jumper
- 4-pin I2C header, 4-pin GPIO header and 3-pin header for an external WS2812 RGB LED strip
- USB Type-C for power, flashing, USB Serial/JTAG console and Li-ion battery charging
- Boot button and power switch

.. include:: ../../../espressif/common/soc-esp32c5-features.rst
   :start-after: espressif-soc-esp32c5-features

Pin Mapping
===========

+---------+-----------------------------------------------------------+
| GPIO    | Function                                                  |
+=========+===========================================================+
| GPIO0   | Shuttle INT2                                              |
+---------+-----------------------------------------------------------+
| GPIO1   | Speaker amplifier enable (active high)                    |
+---------+-----------------------------------------------------------+
| GPIO2   | I2C SDA (Shuttle, touch, I2C header)                      |
+---------+-----------------------------------------------------------+
| GPIO3   | I2C SCL (Shuttle, touch, I2C header)                      |
+---------+-----------------------------------------------------------+
| GPIO4   | GPIO header                                               |
+---------+-----------------------------------------------------------+
| GPIO5   | LCD and microphone power enable (active low)              |
+---------+-----------------------------------------------------------+
| GPIO6   | Microphone (ADC1 channel 5)                               |
+---------+-----------------------------------------------------------+
| GPIO7   | Speaker PDM positive                                      |
+---------+-----------------------------------------------------------+
| GPIO8   | Speaker PDM negative                                      |
+---------+-----------------------------------------------------------+
| GPIO9   | Shuttle SDO (I2C address select)                          |
+---------+-----------------------------------------------------------+
| GPIO10  | Shuttle CS                                                |
+---------+-----------------------------------------------------------+
| GPIO11  | UART0 TX (test pad)                                       |
+---------+-----------------------------------------------------------+
| GPIO12  | UART0 RX (test pad)                                       |
+---------+-----------------------------------------------------------+
| GPIO13  | USB D-                                                    |
+---------+-----------------------------------------------------------+
| GPIO14  | USB D+                                                    |
+---------+-----------------------------------------------------------+
| GPIO23  | LCD SPI MOSI                                              |
+---------+-----------------------------------------------------------+
| GPIO24  | LCD SPI SCLK                                              |
+---------+-----------------------------------------------------------+
| GPIO25  | LCD SPI CS                                                |
+---------+-----------------------------------------------------------+
| GPIO26  | LCD data/command                                          |
+---------+-----------------------------------------------------------+
| GPIO27  | RGB LED header data (WS2812)                              |
+---------+-----------------------------------------------------------+
| GPIO28  | Boot button and Shuttle INT1                              |
+---------+-----------------------------------------------------------+

The LCD reset line is not connected to the SoC and the LCD backlight is powered from the same
switched supply as the LCD, controlled by GPIO5.

The LP core console uses the LP UART on GPIO4 (RX) and GPIO5 (TX). GPIO5 also controls the LCD
and microphone power, so that supply is off while the LP UART drives the pin.

Shuttle Boards
==============

The Shuttle Board connector shares the I2C bus with the touch controller and the I2C header.
Only one shuttle board can be attached at a time. The shuttle supply jumper must be fitted,
otherwise the shuttle board is not powered.

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

The board has no reset button. To force the download mode, hold the Boot button while turning
the power switch on.

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

.. _`ESP-SensairShuttle`: https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32c5/esp-sensairshuttle/index.html
