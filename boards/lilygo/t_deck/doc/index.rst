.. zephyr:board:: t_deck

Overview
********

The LilyGO `T-Deck`_ is a handheld ESP32-S3 device with a landscape display, capacitive touch panel,
external keyboard controller, trackball, and wireless connectivity. The `T-Deck Plus`_ adds a
battery and a GNSS receiver.

Hardware
********

- ESP32-S3-WROOM-N16R8 module

  - Dual core Xtensa LX7 up to 240 MHz
  - 16 MB flash
  - 8 MB PSRAM
  - Wi-Fi 802.11 b/g/n
  - Bluetooth LE 5.0

- 2.8 inch 320x240 SPI TFT display
- Goodix GT911 capacitive touch controller
- External I2C keyboard controller with a 35-key keyboard
- Trackball with four direction inputs and a center press button
- Battery voltage measurement on IO4 through a 100k/100k resistor divider
- USB-C for power, flashing, logging, and debugging through the built-in USB JTAG
- GNSS module (T-Deck Plus only)

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

The board enters the boot ROM download mode when the center trackball button is held while resetting
or powering the device.

.. include:: ../../../espressif/common/board-variants.rst
   :start-after: espressif-board-variants

Debugging
=========

.. include:: ../../../espressif/common/openocd-debugging.rst
   :start-after: espressif-openocd-debugging

References
**********

.. target-notes::

.. _`T-Deck`: https://lilygo.cc/products/t-deck
.. _`T-Deck Plus`: https://lilygo.cc/products/t-deck-plus
