:orphan:

.. espressif-soc-esp32-features

ESP32 Features
==============

- Dual core Xtensa microprocessor (LX6), running at 160 or 240MHz
- 520KB of SRAM
- 802.11b/g/n/e/i
- Bluetooth v4.2 BR/EDR and BLE
- Various peripherals:

  - 12-bit ADC with up to 18 channels
  - 2x 8-bit DACs
  - 10x touch sensors
  - 4x SPI
  - 2x I2S
  - 2x I2C
  - 3x UART
  - SD/SDIO/MMC host
  - Slave (SDIO/SPI)
  - Ethernet MAC
  - CAN bus 2.0
  - IR (RX/TX)
  - Motor PWM
  - LED PWM with up to 16 channels
  - Hall effect sensor
  - Temperature sensor

- Cryptographic hardware acceleration (RNG, ECC, RSA, SHA-2, AES)
- 5uA deep sleep current

Asymmetric Multiprocessing (AMP)
================================

Boards featuring the ESP32 and ESP32-S3 SoC allows 2 different applications to be executed.
Due to its dual-core architecture, each core can be enabled to execute customized tasks in stand-alone mode
and/or exchanging data over OpenAMP framework. See :zephyr:code-sample-category:`ipc` folder as code reference.

.. note::

   ** AMP and serial output support **

   In the current Zephyr ESP32 implementation, access to Zephyr-managed serial
   drivers (such as ``printk()``, logging, or the console UART) is not yet
   implemented for applications running on the APPCPU. As a result, serial
   output APIs provided by Zephyr are only available on the PROCPU.

   As a mitigation, applications running on the APPCPU may use ESP32 ROM
   functions such as ``ets_printf()`` to emit diagnostic or debug output.

For more information, check the `ESP32 Datasheet`_ or the `ESP32 Technical Reference Manual`_.

.. _`ESP32 Datasheet`: https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf
.. _`ESP32 Technical Reference Manual`: https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf
