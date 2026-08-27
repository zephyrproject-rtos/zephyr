.. zephyr:board:: xiao_esp32c5

Overview
********

`Seeed Studio XIAO ESP32C5`_ is a compact development board based on the ESP32-C5 SoC, with
dual-band Wi-Fi 6, Bluetooth LE, and 802.15.4 (Zigbee/Thread) support. It provides USB-C for
programming and debugging (USB Serial/JTAG), battery charging, an external U.FL antenna connector,
and the standard XIAO 14-pin castellated footprint.

Hardware
********

The board uses the ESP32-C5-WROOM-1 module (8 MB flash and 8MB PSRAM) in the XIAO form factor.

.. include:: ../../../espressif/common/soc-esp32c5-features.rst
   :start-after: espressif-soc-esp32c5-features

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The board uses the standard XIAO pinout. Default mapping of D0-D10 to SoC GPIOs is as follows:

.. rst-class:: rst-columns

- D0 : GPIO1
- D1 : GPIO0
- D2 : GPIO25
- D3 : GPIO7
- D4 : GPIO23
- D5 : GPIO24
- D6 : GPIO11
- D7 : GPIO12
- D8 : GPIO8
- D9 : GPIO9
- D10 : GPIO10

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

.. _`Seeed Studio XIAO ESP32C5`: https://wiki.seeedstudio.com/xiao_esp32c5_getting_started/
