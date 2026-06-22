:orphan:

.. espressif-soc-esp32c3-features

ESP32-C3 Features
=================

ESP32-C3 is a single-core Wi-Fi and Bluetooth 5 (LE) microcontroller SoC,
based on the open-source RISC-V architecture. It strikes the right balance of power,
I/O capabilities and security, thus offering the optimal cost-effective
solution for connected devices.
The availability of Wi-Fi and Bluetooth 5 (LE) connectivity not only makes the device configuration easy,
but it also facilitates a variety of use-cases based on dual connectivity.

The features include the following:

- 32-bit core RISC-V microcontroller with a maximum clock speed of 160 MHz
- 802.11b/g/n/
- A Bluetooth LE subsystem that supports features of Bluetooth 5 and Bluetooth Mesh
- 384 KB ROM
- 400 KB SRAM (16 KB for cache)
- 8 KB SRAM in RTC
- 22 x programmable GPIOs
- Various peripherals:

  - Full-speed USB Serial/JTAG controller
  - TWAI® compatible with CAN bus 2.0
  - General DMA controller (GDMA)
  - 2x 12-bit SAR ADC with up to 6 channels
  - 3x SPI
  - 2x UART
  - 1x I2S
  - 1x I2C
  - 2 x 54-bit general-purpose timers
  - 3 x watchdog timers
  - 1 x 52-bit system timer
  - Remote Control Peripheral (RMT)
  - LED PWM controller (LEDC) with up to 6 channels
  - Temperature sensor

- Cryptographic hardware acceleration (RNG, ECC, RSA, SHA-2, AES)

For more information, check the `ESP32-C3 Datasheet`_ or the `ESP32-C3 Technical Reference Manual`_.

.. _`ESP32-C3 Datasheet`: https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf
.. _`ESP32-C3 Technical Reference Manual`: https://espressif.com/sites/default/files/documentation/esp32-c3_technical_reference_manual_en.pdf
