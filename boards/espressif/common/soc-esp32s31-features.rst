:orphan:

.. espressif-soc-esp32s31-features

ESP32-S31 Features
==================

ESP32-S31 is a dual-band Wi-Fi 6 SoC integrating 2.4 GHz and 5 GHz Wi-Fi 6,
Bluetooth 5 (LE) and the 802.15.4 protocol. It consists of two high-performance
(HP) 32-bit RISC-V processors and a low-power (LP) 32-bit RISC-V processor.
It has 512 KB of internal SRAM and works with external flash and PSRAM.

ESP32-S31 includes the following features:

- Dual-core 32-bit RISC-V high-performance microcontroller
- 512 KB of internal SRAM
- WiFi 802.11 ax 2.4GHz and 5GHz (dual-band)
- Fully compatible with IEEE 802.11b/g/n/a/ac protocol
- Bluetooth LE: Bluetooth 5 certified
- Internal co-existence mechanism between Wi-Fi and Bluetooth to share the same antenna
- IEEE 802.15.4 (Zigbee and Thread)

Digital interfaces:

- 62x GPIOs
- 4x UART
- 1x Low-power (LP) UART
- 1x General purpose SPI
- 3x I2C
- 1x I2S
- 1x Pulse counter
- 1x USB OTG controller
- 1x USB Serial/JTAG controller
- 2x SD/MMC host slots
- 2x TWAI controller, compatible with ISO 11898-1 (CAN Specification 2.0)
- LED PWM controller, up to 8 channels
- 1x Motor control PWM (MCPWM)
- 1x Parallel IO (PARLIO) interface
- General DMA controller (GDMA)

Analog interfaces:

- 2x 12-bit SAR ADC, up to 8 channels each
- 1x temperature sensor

Security:

- Secure boot
- Flash encryption
- Cryptographic hardware acceleration: AES-128/256, SHA, RSA, HMAC, Digital signature
- Random number generator (RNG)

Low-Power CPU (LP CORE)
=======================

The ESP32-S31 SoC integrates a Low-Power Core (LP CORE) alongside the
High-Performance Cores (HP CORE). The LP Core features ultra low power
consumption, an interrupt controller, a debug module and a system bus
interface for memory and peripheral access.

The LP Core is in sleep mode by default. It has two application scenarios:

- Power insensitive scenario: When the High-Performance CPU (HP Core) is active, the LP Core can assist the HP CPU with some speed and efficiency-insensitive controls and computations.
- Power sensitive scenario: When the HP CPU is in the power-down state to save power, the LP Core can be woken up to handle some external wake-up events.

The LP Core support is fully integrated with :ref:`sysbuild`. The user can enable the LP Core by adding
the following configuration to the project:

.. code:: cfg

   CONFIG_ESP32_ULP_COPROC_ENABLED=y

See :zephyr:code-sample-category:`lp-core` folder as code reference.
