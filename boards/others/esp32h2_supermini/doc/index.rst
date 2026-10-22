.. zephyr:board:: esp32h2_supermini

ESP32-H2 SuperMini
##################

Overview
********

The ESP32-H2 SuperMini is a compact development board based on the Espressif ESP32-H2 RISC-V SoC.
It features ultra-low power consumption and supports IEEE 802.15.4 (Thread/Zigbee) and Bluetooth 5 (LE).
The board is extremely small, making it ideal for compact IoT devices, wearables, and battery-powered applications.

Hardware
********

.. include:: ../../../espressif/common/soc-esp32h2-features.rst
   :start-after: espressif-soc-esp32h2-features

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

WS2812 RGB LED
**************

The board features an onboard WS2812 RGB LED connected to GPIO 8.
This board uses the ``i2s`` peripheral to drive the LED natively without bit-banging, ensuring perfect timing even under heavy interrupt load from the Bluetooth stack.

To control the LED, enable the ``led_strip`` API in your project configuration by adding the following flags to your ``prj.conf``:

.. code-block:: kconfig

   CONFIG_LED_STRIP=y
   CONFIG_WS2812_STRIP_I2S=y
   CONFIG_I2S=y

.. note::
   There is no need to manually enable ``CONFIG_DMA=y``. The Zephyr I2S driver for ESP32 automatically selects the required General DMA (GDMA) dependencies.
