.. zephyr:board:: esp32c61_devkitc

Overview
********

ESP32-C61-DevKitC-1 is an entry-level development board based on ESP32-C61-WROOM-1,
a general-purpose module with an 8 MB SPI flash and 2 MB PSRAM. This board
integrates complete 2.4 GHz Wi-Fi 6 and Bluetooth LE functions. For more
information, check `ESP32-C61-DevKitC-1`_.

Hardware
********

Most of the I/O pins are broken out to the pin headers on both sides for easy
interfacing. Developers can either connect peripherals with jumper wires or mount
ESP32-C61-DevKitC-1 on a breadboard.

.. include:: ../../../espressif/common/soc-esp32c61-features.rst
   :start-after: espressif-soc-esp32c61-features

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

.. _`ESP32-C61-DevKitC-1`: https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32c61/esp32-c61-devkitc-1/user_guide.html
