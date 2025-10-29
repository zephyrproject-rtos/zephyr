:orphan:

.. espressif-soc-esp32c2-features

ESP32-C2 Features
=================

ESP32-C2 (ESP8684 core) is a low-cost, Wi-Fi 4 & Bluetooth 5 (LE) chip. Its unique design
makes the chip smaller and yet more powerful than ESP8266. ESP32-C2 is built around a RISC-V
32-bit, single-core processor, with 272 KB of SRAM (16 KB dedicated to cache) and 576 KB of ROM.
ESP32-C2 has been designed to target simple, high-volume, and low-data-rate IoT applications,
such as smart plugs and smart light bulbs. ESP32-C2 offers easy and robust wireless connectivity,
which makes it the go-to solution for developing simple, user-friendly and reliable
smart-home devices.

Features include the following:

- 32-bit core RISC-V microcontroller with a maximum clock speed of 120 MHz
- 2 MB or 4 MB in chip (ESP8684) or in package (ESP32-C2) flash
- 272 KB of internal RAM
- 802.11b/g/n
- A Bluetooth LE subsystem that supports features of Bluetooth 5 and Bluetooth Mesh
- Various peripherals:

  - General DMA controller (GDMA)
  - LED PWM controller, with up to 6 channels
  - 14 programmable GPIOs
  - 3 SPI
  - 2 UART
  - 1 I2C Master
  - 1 12-bit SAR ADC, up to 5 channels
  - 1 temperature sensor
  - 1 54-bit general-purpose timer
  - 2 watchdog timers
  - 1 52-bit system timer

- Cryptographic hardware acceleration (RNG, ECC, RSA, SHA-2, AES)

For detailed information check the `ESP8684 Datasheet`_ or the `ESP8684 Technical Reference Manual`_.

.. _`ESP8684 Datasheet`: https://www.espressif.com/sites/default/files/documentation/esp8684_datasheet_en.pdf
.. _`ESP8684 Technical Reference Manual`: https://www.espressif.com/sites/default/files/documentation/esp8684_technical_reference_manual_en.pdf
