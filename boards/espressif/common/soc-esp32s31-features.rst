:orphan:

.. espressif-soc-esp32s31-features

ESP32-S31 Features
==================

ESP32-S31 is a dual-core 32-bit RISC-V SoC integrating 2.4 GHz Wi-Fi 6, Bluetooth 5.4 (LE and
Classic), IEEE 802.15.4 and a 1000 Mbps Ethernet MAC. It targets applications with rich
human-machine interfaces, audio and multi-protocol connectivity, and adds a low-power (LP) 32-bit
RISC-V coprocessor, in-package PSRAM and dedicated image processing hardware.

ESP32-S31 includes the following features:

- Dual-core 32-bit RISC-V HP processor with a clock speed of up to 320 MHz
- RV32IMAFC instruction set with Zc and Zb extensions, plus Espressif hardware loop extensions
- Single-precision floating-point unit (FPU) on each core
- 128-bit data path with SIMD instructions on one of the cores
- Private instruction cache per core and a shared data cache
- 32-bit RISC-V LP processor with a clock speed of up to 40 MHz
- 320 KB of ROM
- 512 KB of HP SRAM
- 32 KB of LP SRAM
- In-package PSRAM over an 8-bit DDR interface at up to 250 MHz
- External flash over SPI, Dual SPI, Quad SPI, Octal SPI, QPI and OPI, with concurrent flash and
  PSRAM access

Wireless connectivity:

- Wi-Fi 6 (IEEE 802.11ax), 1T1R in the 2.4 GHz band
- 20 MHz-only non-AP mode for 802.11ax, with uplink and downlink OFDMA and downlink MU-MIMO
- Compatible with IEEE 802.11b/g/n, 20 MHz and 40 MHz bandwidth, data rate up to 150 Mbps
- Bluetooth LE: Bluetooth 5.4 certified, 125 Kbps, 500 Kbps, 1 Mbps and 2 Mbps PHYs
- Bluetooth LE Audio (isochronous channels, BIS and CIS)
- Bluetooth direction finding (AoA and AoD) and Bluetooth Mesh 1.1
- Bluetooth Classic (BR/EDR): basic rate 1 Mbps and enhanced data rate 2 Mbps and 3 Mbps
- Up to 7 ACL links, up to 2 synchronous links and up to 3 piconets simultaneously, for
  classic audio profiles such as A2DP and HFP
- IEEE 802.15.4-2015 compliant radio (Thread and Zigbee), O-QPSK PHY at 250 Kbps
- Internal co-existence mechanism between Wi-Fi, Bluetooth and IEEE 802.15.4 to share the same
  antenna

Digital interfaces:

- 60 programmable GPIOs
- 4x UART
- 1x Low-power (LP) UART
- 2x General purpose SPI
- 1x Low-power (LP) SPI
- 2x I2C
- 1x Low-power (LP) I2C
- 2x I2S, with PDM and TDM support and hardware-level Bluetooth audio
- 1x SDIO host controller with 2 slots
- 1x USB 2.0 OTG High-Speed (HS) with embedded PHY
- 1x USB Serial/JTAG controller
- 1x 1000 Mbps Ethernet MAC
- TWAI controller with CAN FD support, compatible with ISO 11898-1:2015
- 1x LCD and camera controller: 8 to 24-bit parallel LCD (i80 and RGB) and 8 to 16-bit DVP
  camera
- 1x Parallel IO (PARLIO) controller
- 1x BitScrambler
- LED PWM controller, up to 8 channels
- 4x Motor control PWM (MCPWM)
- 2x Pulse counter, 4 units each
- 1x Remote control peripheral (RMT), 4 TX and 4 RX channels
- General DMA controller (GDMA) over AHB and AXI buses
- 2D-DMA controller
- Event task matrix (ETM)

Image, audio and math accelerators:

- 1x JPEG codec (encoder and decoder)
- 1x 2D Pixel Processing Accelerator (PPA)
- 1x Audio sample rate converter (ASRC)
- 1x CORDIC accelerator

Analog interfaces:

- 2x 12-bit SAR ADC, up to 16 channels
- 2x DAC
- 1x Analog voltage comparator (1 reference input and 3 comparison inputs)
- 14x Capacitive touch sensor channels
- 1x Temperature sensor

Timers:

- 1x 52-bit system timer
- 4x 54-bit general-purpose timers
- 3x Watchdog timers
- RTC timer

Low Power:

- Multiple power modes designed for typical scenarios: Active, Modem-sleep, Light-sleep,
  Deep-sleep
- Power Management Unit (PMU) with sleep retention of peripheral registers
- Brown-out detector

Security:

- Secure boot
- Flash and PSRAM encryption (XTS-AES-128 and XTS-AES-256)
- Cryptographic hardware acceleration: AES-128/256, SHA (up to SHA-512), RSA (up to 4096 bits),
  ECC (up to P-384), HMAC
- RSA digital signature (DS) and ECDSA peripherals
- Key manager with PUF-based key protection
- True random number generator (TRNG)
- Trusted Execution Environment (TEE) with Access Permission Management (APM)
- Secure debug controller
- Power glitch detector

Low-Power CPU (LP CORE)
=======================

The ESP32-S31 SoC has a Low-Power (LP) 32-bit RISC-V coprocessor in addition to the dual HP
cores. The LP Core features ultra low power consumption, an interrupt controller, a debug module
and a system bus interface for memory and peripheral access.

The LP Core is in sleep mode by default. It has two application scenarios:

- Power insensitive scenario: When the HP cores are active, the LP Core can assist with some
  speed and efficiency-insensitive controls and computations.
- Power sensitive scenario: When the HP cores are in the power-down state to save power, the LP
  Core can be woken up to handle some external wake-up events.

For more information, check the `ESP32-S31 Datasheet`_.

.. _`ESP32-S31 Datasheet`: https://documentation.espressif.com/esp32-s31_datasheet_en.html
