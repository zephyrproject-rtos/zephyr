.. zephyr:board:: nucleo_wba65ri

Overview
********

NUCLEO-WBA65RI is a Bluetooth® Low Energy, 802.15.4 and Zigbee® wireless
and ultra-low-power board embedding a powerful and ultra-low-power radio
compliant with the Bluetooth® Low Energy SIG specification v5.4
with IEEE 802.15.4-2015 and Zigbee® specifications.

The ARDUINO® Uno V3 connectivity support and the ST morpho headers allow the
easy expansion of the functionality of the STM32 Nucleo open development
platform with a wide choice of specialized shields.

- Ultra-low-power wireless STM32WBA65RI microcontroller based on the Arm®
  Cortex®‑M33 core, featuring 2 Mbyte of flash memory and 512 Kbytes of SRAM in
  a VFQFPN68 package

- MCU RF board (MB2130):

  - 2.4 GHz RF transceiver supporting Bluetooth® specification v5.4
  - Arm® Cortex® M33 CPU with TrustZone®, MPU, DSP, and FPU
  - Integrated PCB antenna

- Three user LEDs
- Three user and one reset push-buttons

- Board connectors:

  - 2 USB Type-C
  - ARDUINO® Uno V3 expansion connector
  - ST morpho headers for full access to all STM32 I/Os

- Flexible power-supply options: ST-LINK USB VBUS or external sources
- On-board STLINK-V3MODS debugger/programmer with USB re-enumeration capability:
  mass storage, Virtual COM port, and debug port

Hardware
********

The STM32WBA65xx multiprotocol wireless and ultralow power devices embed a
powerful and ultralow power radio compliant with the Bluetooth® SIG Low Energy
specification 5.4. They contain a high-performance Arm Cortex-M33 32-bit RISC
core. They operate at a frequency of up to 100 MHz.

- Includes ST state-of-the-art patented technology

- Ultra low power radio:

  - 2.4 GHz radio
  - RF transceiver supporting Bluetooth® Low Energy 5.4 specification
    IEEE 802.15.4-2015 PHY and MAC, supporting Thread, Matter and Zigbee®
  - Proprietary protocols
  - RX sensitivity: -96 dBm (Bluetooth® Low Energy at 1 Mbps)
    and -100 dBm (IEEE 802.15.4 at 250 kbps)
  - Programmable output power, up to +10 dBm with 1 dB steps
  - Support for external PA
  - Integrated balun to reduce BOM
  - Suitable for systems requiring compliance with radio frequency regulations
    ETSI EN 300 328, EN 300 440, FCC CFR47 Part 15 and ARIB STD-T66

- Ultra low power platform with FlexPowerControl:

  - 1.71 to 3.6 V power supply
  - - 40 °C to 85 °C temperature range
  - Autonomous peripherals with DMA, functional down to Stop 1 mode
  - TBD nA Standby mode (16 wake-up pins)
  - TBD nA Standby mode with RTC
  - TBD µA Standby mode with 64 KB SRAM
  - TBD µA Stop 2 mode with 64 KB SRAM
  - TBD µA/MHz Run mode at 3.3 V
  - Radio: Rx TBD mA / Tx at 0 dBm TBD mA

- Core: Arm® 32-bit Cortex®-M33 CPU with TrustZone®, MPU, DSP, and FPU
- ART Accelerator™: 8-Kbyte instruction cache allowing 0-wait-state execution
  from flash memory (frequency up to 100 MHz, 150 DMIPS)
- Power management: embedded regulator LDO and SMPS step-down converter
- Supporting switch on-the-fly and voltage scaling

- Benchmarks:

  - 1.5 DMIPS/MHz (Drystone 2.1)
  - 410 CoreMark® (4.10 CoreMark/MHz)

- Clock sources:

  - 32 MHz crystal oscillator
  - 32 kHz crystal oscillator (LSE)
  - Internal low-power 32 kHz (±5%) RC
  - Internal 16 MHz factory trimmed RC (±1%)
  - PLL for system clock and ADC

- Memories:

  - 2 MB flash memory with ECC, including 256 Kbytes with 100 cycles
  - 512 KB SRAM, including 64 KB with parity check
  - 512-byte (32 rows) OTP

- Rich analog peripherals (independent supply):

  - 12-bit ADC 2.5 Msps with hardware oversampling

- Communication peripherals:

  - Four UARTs (ISO 7816, IrDA, modem)
  - Three SPIs
  - Four I2C Fm+ (1 Mbit/s), SMBus/PMBus®

- System peripherals:

  - Touch sensing controller, up to 24 sensors, supporting touch key, linear,
     rotary touch sensors
  - One 16-bit, advanced motor control timer
  - Three 16-bit timers
  - Two 32-bit timer
  - Two low-power 16-bit timers (available in Stop mode)
  - Two Systick timers
  - RTC with hardware calendar and calibration
  - Two watchdogs
  - 8-channel DMA controller, functional in Stop mode

- Security and cryptography:

  - Arm® TrustZone® and securable I/Os, memories, and peripherals
  - Flexible life cycle scheme with RDP and password protected debug
  - Root of trust thanks to unique boot entry and secure hide protection area (HDP)
  - SFI (secure firmware installation) thanks to embedded RSS (root secure services)
  - Secure data storage with root hardware unique key (RHUK)
  - Secure firmware upgrade support with TF-M
  - Two AES co-processors, including one with DPA resistance
  - Public key accelerator, DPA resistant
  - HASH hardware accelerator
  - True random number generator, NIST SP800-90B compliant
  - 96-bit unique ID
  - Active tampers
  - CRC calculation unit

- Up to 86 I/Os (most of them 5 V-tolerant) with interrupt capability

- Development support:

  - Serial wire debug (SWD), JTAG

- ECOPACK2 compliant package

More information about STM32WBA series can be found here:

- `STM32WBA Series on www.st.com`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

Nucleo WBA65RI Board has 4 GPIO controllers. These controllers are responsible for pin muxing,
input/output, pull-up, etc.

Default Zephyr Peripheral Mapping:
----------------------------------

.. rst-class:: rst-columns

- USART_1 TX/RX : PB12/PA8
- I2C_1_SCL : PB2
- I2C_1_SDA : PB1
- USER_PB : PC13
- LD1 : PD8
- SPI_1_NSS : PA12 (arduino_spi)
- SPI_1_SCK : PB4 (arduino_spi)
- SPI_1_MISO : PB3 (arduino_spi)
- SPI_1_MOSI : PA15 (arduino_spi)

System Clock
------------

Nucleo WBA65RI System Clock could be driven by internal or external oscillator,
as well as main PLL clock. By default System clock is driven by HSE+PLL clock at 100MHz.

Serial Port
-----------

Nucleo WBA65RI board has 3 U(S)ARTs. The Zephyr console output is assigned to USART1.
Default settings are 115200 8N1.


Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Nucleo WBA65RI board includes an ST-LINK/V3 embedded debug tool interface.
It could be used for flash and debug using either OpenOCD or STM32Cube ecosystem tools.

Flashing
========

The board is configured to be flashed using west `STM32CubeProgrammer`_ runner,
so its :ref:`installation <stm32cubeprog-flash-host-tools>` is required.

Alternatively, openocd can also be used to flash the board using
the ``--runner`` (or ``-r``) option:

.. code-block:: console

   $ west flash --runner openocd

Flashing an application to Nucleo WBA65RI
-----------------------------------------

Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: nucleo_wba65ri
   :goals: build flash

You will see the LED blinking every second.

Debugging
=========

Debugging using OpenOCD
-----------------------

You can debug an application in the usual way using OpenOCD. Here is an example for the
:zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: nucleo_wba65ri
   :maybe-skip-config:
   :goals: debug

.. _STM32WBA Series on www.st.com:
   https://www.st.com/en/microcontrollers-microprocessors/stm32wba-series.html

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html
