.. zephyr:board:: xh_s3e_ai

Overview
********

The XH-S3E-AI (silkscreen ``XH-S3E-AI-V1.0``) is a compact ESP32-S3 based AI
audio development board built around the `Sparkleiot XH-S3E module`_
(functionally equivalent to an ESP32-S3-WROOM-1 with 16 MB flash and 8 MB
octal PSRAM). It is aimed at voice / "Xiaozhi AI" style applications and
integrates:

- An I2S MEMS microphone (INMP441-class)
- An NS4168 I2S class-D amplifier (MAX98357A-compatible) driving an external
  speaker (JST connector)
- A single WS2812 addressable RGB LED
- A ``BOOT`` button (GPIO0), ``Volume Up`` / ``Volume Down`` buttons, and a
  ``RST`` (chip reset) button
- A 2.54 mm pin header breaking out I2C (``IO41/SDA``, ``IO42/SCL``), UART0
  (``RXO``/``TXO``), and power (``3V3``, ``VIN``, ``GND``)

It provides Wi-Fi 4 (2.4 GHz) and Bluetooth LE 5 through the ESP32-S3.

Hardware
********

.. include:: ../../../espressif/common/soc-esp32s3-features.rst
   :start-after: espressif-soc-esp32s3-features

Supported Features
==================

.. zephyr:board-supported-hw::

Pin Mapping
===========

The following peripheral-to-GPIO assignments are used by this board's
devicetree.

.. list-table::
   :header-rows: 1

   * - Peripheral
     - Signal
     - GPIO
   * - Microphone (I2S1)
     - WS
     - GPIO4
   * - Microphone (I2S1)
     - SCK / BCLK
     - GPIO5
   * - Microphone (I2S1)
     - SD (data in)
     - GPIO6
   * - Speaker amp (I2S0)
     - DIN
     - GPIO7
   * - Speaker amp (I2S0)
     - BCLK
     - GPIO15
   * - Speaker amp (I2S0)
     - LRCLK / WS
     - GPIO16
   * - RGB LED (WS2812)
     - DIN (via SPI2 MOSI)
     - GPIO48
   * - I2C0
     - SDA
     - GPIO41
   * - I2C0
     - SCL
     - GPIO42
   * - UART0
     - TX
     - GPIO43
   * - UART0
     - RX
     - GPIO44
   * - BOOT button
     - \-
     - GPIO0
   * - Volume Up button
     - \-
     - GPIO40
   * - Volume Down button
     - \-
     - GPIO39

.. note::

   All of the above assignments are verified on hardware (console, RGB LED,
   speaker amplifier, microphone, and the BOOT / Volume Up / Volume Down
   buttons).

   The microphone is an I2S MEMS mic operating as an I2S **receive master**:
   its bit/word clock is driven from the I2S RX unit, so the pin control uses
   the ``I2S1_I_WS``/``I2S1_I_BCK`` (RX-unit) clock signals rather than the
   ``I2S1_O_*`` (TX-unit) signals. Using the TX-unit signals leaves the mic
   unclocked. See ``xh_s3e_ai-pinctrl.dtsi``.

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

.. _`Sparkleiot XH-S3E module`: https://manuals.plus/sparkleiot/xh-s3e-ultra-low-power-wifi-module-manual
