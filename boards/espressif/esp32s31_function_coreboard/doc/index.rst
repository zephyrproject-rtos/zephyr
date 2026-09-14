.. zephyr:board:: esp32s31_function_coreboard

Overview
********

ESP32-S31-Function-CoreBoard-1 is a development board based on the
ESP32-S31-WROOM-3 module, which carries the ESP32-S31, a dual-core RISC-V SoC
with dual-band Wi-Fi 6, Bluetooth LE 5.4 and IEEE 802.15.4 connectivity,
working with external SPI flash and PSRAM.

Hardware
********

The board exposes two USB Type-C ports, a USB-to-UART bridge used for power
and serial console and the ESP32-S31 USB Serial/JTAG port used for power,
flashing and debugging, plus a USB 2.0 Type-A host port wired to the USB OTG
high-speed interface.

Besides the radio interfaces, the board carries a 10/100/1000 Mbps Ethernet
port with its PHY and transformer, an ES8311 mono audio codec with an onboard
microphone, and an NS4150B class D amplifier. An addressable RGB LED is driven
by GPIO60 and the BOOT button is on GPIO61. The serial console runs on GPIO58
(TX) and GPIO59 (RX).

Most of the remaining I/O pins are broken out to the 40-pin J2 header, and the
J5 headers allow measuring the module current consumption.

.. include:: ../../../espressif/common/soc-esp32s31-features.rst
   :start-after: espressif-soc-esp32s31-features

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

.. _ESP32-S31-Function-CoreBoard-1 User Guide:
   https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s31/esp32-s31-function-coreboard-1/user_guide.html
