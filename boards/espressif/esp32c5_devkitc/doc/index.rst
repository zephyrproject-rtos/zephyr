.. zephyr:board:: esp32c5_devkitc

Overview
********

ESP32-C5-DevKitC-1 is an entry-level development board based on ESP32-C5-WROOM-1,
a general-purpose module with a 8 MB SPI flash and 4 MB PSRAM. This board integrates complete
dual-band Wi-Fi 6, Bluetooth LE, Zigbee, and Thread functions.
For more information, check `ESP32-C5-DevKitC-1`_.

Hardware
********

Most of the I/O pins are broken out to the pin headers on both sides for easy interfacing.
Developers can either connect peripherals with jumper wires or mount ESP32-C5-DevKitC-1 on
a breadboard.

.. include:: ../../../espressif/common/soc-esp32c5-features.rst
   :start-after: espressif-soc-esp32c5-features

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

.. _`ESP32-C5-DevKitC-1`: https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32c5/esp32-c5-devkitc-1/user_guide.html
