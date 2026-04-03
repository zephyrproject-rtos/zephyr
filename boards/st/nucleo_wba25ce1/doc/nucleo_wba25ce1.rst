.. zephyr:board:: nucleo_wba25ce1

Overview
********

NUCLEO-WBA25CE1 is a Bluetooth® Low Energy wireless and ultra-low-power board
embedding a powerful and ultra-low-power radio compliant with the Bluetooth®
Low Energy SIG specification v5.3.

The ARDUINO® Uno V3 connectivity support and the ST morpho headers allow the
easy expansion of the functionality of the STM32 Nucleo open development
platform with a wide choice of specialized shields.

- Ultra-low-power wireless STM32WBA25CE1 microcontroller based on the Arm®
  Cortex®-M33 core, featuring 512 Kbytes of flash memory and 96 Kbytes of SRAM in
  a UFQFPN48 package

- MCU RF board (MB1863):

  - 2.4 GHz RF transceiver supporting Bluetooth® specification v5.3
  - Arm® Cortex®-M33 CPU with TrustZone®, MPU, DSP, and FPU
  - Integrated PCB antenna

- Three user LEDs
- Three user and one reset push-buttons
- 8-pin SOP Quad-SPI NOR flash memory footprint

- Board connectors:

  - USB Type-C®
  - ARDUINO® Uno V3 expansion connector
  - ST morpho headers for full access to all STM32 I/Os

- Flexible power-supply options: ST-LINK USB VBUS or external sources
- On-board STLINK-V3MODS debugger/programmer with USB re-enumeration capability:
  mass storage, Virtual COM port, and debug port

Hardware
********

The STM32WBA2xxx multiprotocol wireless and ultra-low-power devices embed a powerful
and ultra-low-power 2.4 GHz RADIO compliant with the Bluetooth® LE specifications and
with IEEE 802.15.4-2015. They contain a high-performance Arm® Cortex®-M33 32-bit RISC
core. They operate at a frequency of up to 64 MHz.

- Includes ST state-of-the-art patented technology

- Ultra low power radio:

  - 2.4 GHz radio
  - RF transceiver supporting Bluetooth® LE, IEEE 802.15.4-2015 PHY and MAC,
    supporting Matter and Zigbee®
  - RX sensitivity: -96 dBm (Bluetooth® LE at 1 Mbps),
    -100 dBm (IEEE 802.15.4 at 250 kbps)
  - Programmable output power, up to +10 dBm with 1 dB steps
  - Support for external PA
  - Packet traffic arbitration
  - Integrated balun to reduce BOM
  - Suitable for systems requiring compliance with radio frequency regulations
    ETSI EN 300 328, EN 300 440, FCC CFR47 Part 15 and ARIB STD-T66

- Ultra low power platform with FlexPowerControl:

  - 1.71 to 3.6 V power supply
  - -40 to 85/105 °C ambiant temperature range
  - Autonomous peripherals with DMA, functional down to Stop 1 mode
  - 110 nA Standby mode (7 wake-up pins)
  - 490 nA Standby mode with 64 KB SRAM
  - 980 nA Standby mode with 64 KB SRAM, 2.4 GHz radio, RTC
  - 4.81 µA Stop mode with 64 KB SRAM, 2.4 GHz radio, RTC
  - 21 µA/MHz Run mode
  - Radio: Rx 3.58 mA / Tx at 0 dBm 4.65 mA

- Core: Arm® 32-bit Cortex®-M33 CPU with TrustZone®, MPU, DSP, and FPU
- ART Accelerator™: 4-Kbyte instruction cache allowing 0-wait-state execution
  from flash memory (frequency up to 64 MHz, 96 DMIPS)
- Power management: Embedded regulator LDO and SMPS step-down converter,
  supporting switch on-the-fly and voltage scaling

- Benchmarks:

  - 264 CoreMark® (4.12 CoreMark/MHz)

- Clock sources:

  - 32 MHz crystal oscillator
  - 32 kHz crystal oscillator (LSE)
  - Internal low-power 32 kHz (±5%) RC
  - Internal 16 MHz factory trimmed RC (±1%)
  - PLL for system clock, audio, USB and ADC

- Memories:

  - 512-Kbyte flash memory with ECC and 10 kcycles
  - 96 KB SRAM, including 32 KB with parity check
  - 512-byte (32 rows) OTP
  - One Quad-SPI memory interface

- Analog peripherals (independent supply):

  - 12-bit ADC 2.5 Msps up to 16-bit with
    hardware oversampling

- Communication peripherals:

  - One USB full-speed selectable host or device controller
  - One SAI (serial audio interface)
  - One USART (ISO 7816, IrDA, modem)
  - One LPUART (ISO 7816, modem)
  - One SPI
  - Two I2C Fm+ (1 Mbit/s), SMBus/PMBus®

- System peripherals:

  - Two 16-bit timers
  - One 32-bit timer
  - Two low-power 16-bit timers (available in Stop mode)
  - Two SysTick timers
  - RTC with hardware calendar and calibration
  - One watchdog
  - 8-channel DMA controller, functional in Stop mode

- Security and cryptography:

  - Arm® TrustZone® and securable I/Os, memories, and peripherals
  - Flexible life cycle scheme with RDP and password protected debug
  - Root of trust thanks to unique boot entry and secure hide protection area (HDP)
  - Secure boot and secure firmware update
  - AES accelerator
  - Public key accelerator, ECC and RSA, SCA resistant
  - HASH hardware accelerator
  - True random number generator, NIST SP800-90B compliant
  - 96-bit unique ID
  - Antitamper protections

- Development support:

  - Serial wire debug (SWD), JTAG
  - Embedded trace (ETM)

- ECOPACK2 compliant package

More information about STM32WBA series can be found here:

- `STM32WBA Series on www.st.com`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

Nucleo WBA25CE1 Board has 4 GPIO controllers. These controllers are responsible for pin muxing,
input/output, pull-up, etc.

Default Zephyr Peripheral Mapping:
----------------------------------

.. rst-class:: rst-columns

- USART_1 TX/RX : PA6/PA12
- I2C_3_SCL : PB2
- I2C_3_SDA : PA11
- USER_PB_1 : PA1
- USER_PB_2 : PC13
- USER_PB_3 : PA2
- LD1 : PA7
- LD2 : PB12
- LD3 : PB15
- SPI_3_NSS : PA5 (arduino_spi)
- SPI_3_SCK : PA8 (arduino_spi)
- SPI_3_MISO : PA10 (arduino_spi)
- SPI_3_MOSI : PA9 (arduino_spi)

System Clock
------------

Nucleo WBA25CE1 System Clock could be driven by internal or external oscillator,
as well as main PLL clock. By default System clock is driven by HSE+PLL clock at 64 MHz.

Serial Port
-----------

Nucleo WBA25CE1 board has 1 USART. The Zephyr console output is assigned to USART1.
Default settings are 115200 8N1.


Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Nucleo WBA25CE1 board includes an ST-LINK/V3 embedded debug tool interface.
It could be used for flash and debug using STM32Cube ecosystem tools.

Flashing
========

The board is configured to be flashed using west `STM32CubeProgrammer`_ runner,
so its :ref:`installation <stm32cubeprog-flash-host-tools>` is required.

Flashing an application to Nucleo WBA25CE1
------------------------------------------

Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: nucleo_wba25ce1
   :goals: build flash

You will see the LED blinking every second.

Debugging
=========

Debugging using ST-LINK GDB server
----------------------------------

You can debug an application using the ST-LINK GDB server. For that you need to have
The `STM32CubeCLT`_ installed . see :ref:`stm32cubeclt-host-tools` for more details.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: nucleo_wba25ce1
   :maybe-skip-config:
   :goals: debug

Debugging using STM32CubeIDE
----------------------------

You can debug an application using a STM32WBA compatible version of STM32CubeIDE.

For that:

- Create an empty STM32WBA project by going to File > New > STM32 project
- Select your MCU, click Next, and select an Empty project.
- Right click on your project name, select Debug as > Debug configurations
- In the new window, create a new target in STM32 Cortex-M C/C++ Application
- Select the new target and enter the path to zephyr.elf file in the C/C++ Application field
- Check Disable auto build
- Run debug

.. _STM32WBA Series on www.st.com:
   https://www.st.com/en/microcontrollers-microprocessors/stm32wba-series.html

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html

.. _STM32CubeCLT:
   https://www.st.com/en/development-tools/stm32cubeclt.html
