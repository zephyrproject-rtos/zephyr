:orphan:

.. espressif-soc-esp32c5-features

ESP32-C5 Features
=================

ESP32-C5 is Espressif's first dual-band Wi-Fi 6 SoC integrating 2.4 GHz and 5 GHz Wi-Fi 6,
Bluetooth 5.4 (LE) and the 802.15.4 protocol. ESP32-C5 achieves an industry-leading RF performance,
with reliable security features and multiple memory resources for IoT products.
It consists of a high-performance (HP) 32-bit RISC-V processor, which can be clocked up to 240 MHz,
and a low-power (LP) 32-bit RISC-V processor, which can be clocked up to 20 MHz.
It has a 384KB SRAM and works with external flash and PSRAM (up to 16 MB).

ESP32-C5 includes the following features:

- 32-bit core RISC-V microcontroller with a clock speed of up to 240 MHz
- 384 KB of internal SRAM
- WiFi 802.11 ax 2.4GHz and 5GHz (dual-band)
- Fully compatible with IEEE 802.11b/g/n/a/ac protocol
- Bluetooth LE: Bluetooth 5.4 certified
- Internal co-existence mechanism between Wi-Fi and Bluetooth to share the same antenna
- IEEE 802.15.4 (Zigbee and Thread)

Digital interfaces:

- 29x GPIOs
- 2x UART
- 1x Low-power (LP) UART
- 1x General purpose SPI
- 1x I2C
- 1x Low-power (LP) I2C
- 1x I2S
- 1x Pulse counter
- 1x USB Serial/JTAG controller
- 2x TWAI controller, compatible with ISO 11898-1 (CAN Specification 2.0)
- LED PWM controller, up to 6 channels
- 1x Motor control PWM (MCPWM)
- General DMA controller (GDMA), with 3 transmit channels and 3 receive channels
- Event task matrix (ETM)

Analog interfaces:

- 1x 12-bit SAR ADC, up to 6 channels
- 1x temperature sensor

Timers:

- 1x 52-bit system timer
- 1x 54-bit general-purpose timers
- 3x Watchdog timers
- 1x Analog watchdog timer

Low Power:

- Four power modes designed for typical scenarios: Active, Modem-sleep, Light-sleep, Deep-sleep

Security:

- Secure boot
- Flash encryption
- 4-Kbit OTP, up to 1792 bits for users
- Cryptographic hardware acceleration: (AES-128/256, ECC, HMAC, RSA, SHA, Digital signature, Hash)
- Random number generator (RNG)

Low-Power CPU (LP CORE)
=======================

The ESP32-C5 SoC has two RISC-V cores: the High-Performance Core (HP CORE) and the Low-Power Core (LP CORE).
The LP Core features ultra low power consumption, an interrupt controller, a debug module and a system bus
interface for memory and peripheral access.

The LP Core is in sleep mode by default. It has two application scenarios:

- Power insensitive scenario: When the High-Performance CPU (HP Core) is active, the LP Core can assist the HP CPU with some speed and efficiency-insensitive controls and computations.
- Power sensitive scenario: When the HP CPU is in the power-down state to save power, the LP Core can be woken up to handle some external wake-up events.

The LP Core support is fully integrated with :ref:`sysbuild`. The user can enable the LP Core by adding
the following configuration to the project:

.. code:: cfg

   CONFIG_ESP32_ULP_COPROC_ENABLED=y

See :zephyr:code-sample-category:`lp-core` folder as code reference.

For more information, check the `ESP32-C5 Datasheet`_ or the `ESP32-C5 Technical Reference Manual`_.

.. _`ESP32-C5 Datasheet`: https://www.espressif.com/sites/default/files/documentation/esp32-c5_datasheet_en.pdf
.. _`ESP32-C5 Technical Reference Manual`: https://espressif.com/sites/default/files/documentation/esp32-c5_technical_reference_manual_en.pdf
