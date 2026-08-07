:orphan:

.. espressif-soc-esp32p4-features

ESP32-P4 Features
=================

ESP32-P4 is Espressif's high-performance MCU featuring a dual-core 32-bit RISC-V
high-performance (HP) processor running up to 400 MHz, plus a low-power (LP) 32-bit
RISC-V coprocessor. It targets human-machine interface (HMI) and edge-computing
applications with an integrated L2 cache, high-bandwidth memory interfaces, and
rich connectivity options.

ESP32-P4 includes the following features:

- Dual core 32-bit RISC-V HP processor with a clock speed of up to 400 MHz
- 32-bit RISC-V LP processor with a clock speed of up to 40 MHz
- 768 KB of high-performance SRAM
- 32 KB of low-power SRAM
- 128 KB of ROM
- L1 instruction and data caches plus a unified L2 cache
- Up to 64 MB of external PSRAM in HEX or octal mode
- Up to 16 MB of external flash

Digital interfaces:

- 55 programmable GPIOs
- 5x UART
- 1x Low-power (LP) UART
- 2x General purpose SPI
- 2x I2C
- 1x Low-power (LP) I2C
- 3x I2S
- 1x SDIO 3.0 host controller with 2 slots
- 1x USB 2.0 OTG High-Speed (HS) with embedded PHY
- 1x USB Serial/JTAG controller
- 1x 10/100 Mbit Ethernet MAC with RMII interface
- 1x MIPI-CSI camera interface
- 1x MIPI-DSI display interface
- 1x JPEG codec
- 1x 2D Pixel Processing Accelerator (PPA)
- LED PWM controller, up to 8 channels
- 2x Motor control PWM (MCPWM)
- 3x TWAI controller, compatible with ISO 11898-1 (CAN Specification 2.0)
- 1x Pulse counter
- 1x Remote control peripheral
- General DMA controller (GDMA) over AHB and AXI buses
- Event task matrix (ETM)

Analog interfaces:

- 1x 12-bit SAR ADC
- 1x temperature sensor

Timers:

- 1x 52-bit system timer
- 2x 54-bit general-purpose timers
- 2x Watchdog timers

Low Power:

- Multiple power modes designed for typical scenarios: Active, Modem-sleep, Light-sleep, Deep-sleep
- 4-channel programmable on-chip LDO regulators
- Power Management Unit (PMU) with dedicated retention DMA

Security:

- Secure boot
- Flash encryption
- Cryptographic hardware acceleration: AES-128/256, SHA, RSA, ECC, HMAC, Digital signature, ECDSA
- True random number generator (TRNG)
- Key manager

Low-Power CPU (LP CORE)
=======================

The ESP32-P4 SoC has a Low-Power (LP) 32-bit RISC-V coprocessor in addition to the
dual HP cores. The LP Core features ultra low power consumption, an interrupt
controller, a debug module and a system bus interface for memory and peripheral
access.

The LP Core is in sleep mode by default. It has two application scenarios:

- Power insensitive scenario: When the HP cores are active, the LP Core can assist with some speed and efficiency-insensitive controls and computations.
- Power sensitive scenario: When the HP cores are in the power-down state to save power, the LP Core can be woken up to handle some external wake-up events.

The LP Core support is fully integrated with :ref:`sysbuild`. The user can enable the LP Core by adding
the following configuration to the project:

.. code:: cfg

   CONFIG_ESP32_ULP_COPROC_ENABLED=y

See :zephyr:code-sample-category:`lp-core` folder as code reference.

For more information, check the `ESP32-P4 Datasheet`_ or the `ESP32-P4 Technical Reference Manual`_.

.. _`ESP32-P4 Datasheet`: https://www.espressif.com/sites/default/files/documentation/esp32-p4_datasheet_en.pdf
.. _`ESP32-P4 Technical Reference Manual`: https://www.espressif.com/sites/default/files/documentation/esp32-p4_technical_reference_manual_en.pdf
