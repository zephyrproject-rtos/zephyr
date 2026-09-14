:orphan:

.. espressif-soc-esp32c61-features

ESP32-C61 Features
==================

ESP32-C61 is a low-cost SoC integrating 2.4 GHz Wi-Fi 6 and Bluetooth 5 (LE)
with reliable security features. It consists of a high-performance 32-bit
RISC-V processor, which can be clocked up to 160 MHz. It has a 256KB ROM,
a 320KB SRAM, and works with external flash.

ESP32-C61 includes the following features:

- 32-bit core RISC-V microcontroller with a clock speed of up to 160 MHz
- 320 KB of internal RAM
- Wi-Fi 802.11ax 2.4GHz
- Fully compatible with IEEE 802.11b/g/n protocol
- Bluetooth LE: Bluetooth 5 certified
- Internal co-existence mechanism between Wi-Fi and Bluetooth to share the same antenna

Digital interfaces:

- 30x GPIOs
- 3x UART
- 1x General purpose SPI
- 1x I2C
- 1x I2S
- 1x USB Serial/JTAG controller
- 1x SDIO 2.0 slave controller
- LED PWM controller, up to 6 channels
- General DMA controller (GDMA), with 2 transmit channels and 2 receive channels
- Event task matrix (ETM)

Analog interfaces:

- 1x 12-bit SAR ADC, up to 4 channels
- 1x temperature sensor

Timers:

- 1x 52-bit system timer
- 2x 54-bit general-purpose timers
- 3x Watchdog timers

Low Power:

- Four power modes designed for typical scenarios: Active, Modem-sleep, Light-sleep, Deep-sleep

Security:

- Secure boot
- Flash encryption
- Cryptographic hardware acceleration: (ECC, ECDSA, SHA)
- Random number generator (RNG)

For more information, check the `ESP32-C61 Datasheet`_ or the `ESP32-C61 Technical Reference Manual`_.

.. _`ESP32-C61 Datasheet`: https://www.espressif.com/sites/default/files/documentation/esp32-c61_datasheet_en.pdf
.. _`ESP32-C61 Technical Reference Manual`: https://espressif.com/sites/default/files/documentation/esp32-c61_technical_reference_manual_en.pdf
