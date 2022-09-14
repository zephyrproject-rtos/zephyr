.. _nucleo_wba52cg_board:

ST Nucleo WBA52CG
#################

Overview
********

NUCLEO-WBA52CG is a Bluetooth® Low Energy wireless and ultra-low-power board
embedding a powerful and ultra-low-power radio compliant with the Bluetooth®
Low Energy SIG specification v5.3.

The ARDUINO® Uno V3 connectivity support and the ST morpho headers allow the
easy expansion of the functionality of the STM32 Nucleo open development
platform with a wide choice of specialized shields.

- Ultra-low-power wireless STM32WBA52CG microcontroller based on the Arm®
  Cortex®‑M33 core, featuring 1 Mbyte of flash memory and 128 Kbytes of SRAM in
  a UFQFPN48 package

- MCU RF board (MB1863):

  - 2.4 GHz RF transceiver supporting Bluetooth® specification v5.3
  - Arm® Cortex® M33 CPU with TrustZone®, MPU, DSP, and FPU
  - Integrated PCB antenna

- Three user LEDs
- Three user and one reset push-buttons

- Board connectors:

  - USB Micro-B
  - ARDUINO® Uno V3 expansion connector
  - ST morpho headers for full access to all STM32 I/Os

- Flexible power-supply options: ST-LINK USB VBUS or external sources
- On-board STLINK-V3MODS debugger/programmer with USB re-enumeration capability:
  mass storage, Virtual COM port, and debug port

.. image:: img/nucleowba52cg.jpg
   :align: center
   :alt: Nucleo WBA52CG

More information about the board can be found at the `Nucleo WBA52CG website`_.

Hardware
********

The STM32WBA52xx multiprotocol wireless and ultralow power devices embed a
powerful and ultralow power radio compliant with the Bluetooth® SIG Low Energy
specification 5.3. They contain a high-performance Arm Cortex-M33 32-bit RISC
core. They operate at a frequency of up to 100 MHz.

- Includes ST state-of-the-art patented technology

- Ultra low power radio:

  - 2.4 GHz radio
  - RF transceiver supporting Bluetooth® Low Energy 5.3 specification
  - Proprietary protocols
  - RX sensitivity: -96 dBm (Bluetooth® Low Energy at 1 Mbps)
  - Programmable output power, up to +10 dBm with 1 dB steps
  - Integrated balun to reduce BOM
  - Suitable for systems requiring compliance with radio frequency regulations
    ETSI EN 300 328, EN 300 440, FCC CFR47 Part 15 and ARIB STD-T66

- Ultra low power platform with FlexPowerControl:

  - 1.71 to 3.6 V power supply
  - - 40 °C to 85 °C temperature range
  - Autonomous peripherals with DMA, functional down to Stop 1 mode
  - 140 nA Standby mode (16 wake-up pins)
  - 200 nA Standby mode with RTC
  - 2.4 µA Standby mode with 64 KB SRAM
  - 16.3 µA Stop mode with 64 KB SRAM
  - 45 µA/MHz Run mode at 3.3 V
  - Radio: Rx 7.4 mA / Tx at 0 dBm 10.6 mA

- Core: Arm® 32-bit Cortex®-M33 CPU with TrustZone®, MPU, DSP, and FPU
- ART Accelerator™: 8-Kbyte instruction cache allowing 0-wait-state execution
  from flash memory (frequency up to 100 MHz, 150 DMIPS)
- Power management: embedded regulator LDO supporting voltage scaling

- Benchmarks:

  - 1.5 DMIPS/MHz (Drystone 2.1)
  - 407 CoreMark® (4.07 CoreMark/MHz)

- Clock sources:

  - 32 MHz crystal oscillator
  - 32 kHz crystal oscillator (LSE)
  - Internal low-power 32 kHz (±5%) RC
  - Internal 16 MHz factory trimmed RC (±1%)
  - PLL for system clock and ADC

- Memories:

  - 1 MB flash memory with ECC, including 256 Kbytes with 100 cycles
  - 128 KB SRAM, including 64 KB with parity check
  - 512-byte (32 rows) OTP

- Rich analog peripherals (independent supply):

  - 12-bit ADC 2.5 Msps with hardware oversampling

- Communication peripherals:

  - Three UARTs (ISO 7816, IrDA, modem)
  - Two SPIs
  - Two I2C Fm+ (1 Mbit/s), SMBus/PMBus®

- System peripherals:

  - Touch sensing controller, up to 20 sensors, supporting touch key, linear,
     rotary touch sensors
  - One 16-bit, advanced motor control timer
  - Three 16-bit timers
  - One 32-bit timer
  - Two low-power 16-bit timers (available in Stop mode)
  - Two Systick timers
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

- Up to 35 I/Os (most of them 5 V-tolerant) with interrupt capability

- Development support:

  - Serial wire debug (SWD), JTAG

- ECOPACK2 compliant package

More information about STM32WB55RG can be found here:

- `STM32WBA52CG on www.st.com`_
- `STM32WBA52CG datasheet`_
- `STM32WBA52CG reference manual`_

Supported Features
==================

The Zephyr nucleo_wba52cg board configuration supports the following hardware features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port-polling;                |
|           |            | serial port-interrupt               |
+-----------+------------+-------------------------------------+
| PINMUX    | on-chip    | pinmux                              |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | gpio                                |
+-----------+------------+-------------------------------------+
| I2C       | on-chip    | i2c                                 |
+-----------+------------+-------------------------------------+
| SPI       | on-chip    | spi                                 |
+-----------+------------+-------------------------------------+
| ADC       | on-chip    | adc                                 |
+-----------+------------+-------------------------------------+
| WATCHDOG  | on-chip    | independent watchdog                |
+-----------+------------+-------------------------------------+
| RNG       | on-chip    | True Random number generator        |
+-----------+------------+-------------------------------------+

Other hardware features are not yet supported on this Zephyr port.

The default configuration can be found in the defconfig file:
:zephyr_file:`boards/st/nucleo_wba52cg/nucleo_wba52cg_defconfig`

Connections and IOs
===================

Nucleo WBA52CG Board has 4 GPIO controllers. These controllers are responsible for pin muxing,
input/output, pull-up, etc.

Default Zephyr Peripheral Mapping:
----------------------------------

.. rst-class:: rst-columns

- USART_1 TX/RX : PB12/PA8
- I2C_1_SCL : PB2
- I2C_1_SDA : PB1
- USER_PB : PC13
- LD1 : PB4
- SPI_1_NSS : PA12 (arduino_spi)
- SPI_1_SCK : PB4 (arduino_spi)
- SPI_1_MISO : PB3 (arduino_spi)
- SPI_1_MOSI : PA15 (arduino_spi)

System Clock
------------

Nucleo WBA52CG System Clock could be driven by internal or external oscillator,
as well as main PLL clock. By default System clock is driven by HSE+PLL clock at 100MHz.

Serial Port
-----------

Nucleo WBA52CG board has 1 U(S)ARTs. The Zephyr console output is assigned to USART1.
Default settings are 115200 8N1.


Programming and Debugging
*************************

Nucleo WBA52CG board includes an ST-LINK/V3 embedded debug tool interface.
It could be used for flash and debug using either OpenOCD or STM32Cube ecosystem tools.

OpenOCD Support
===============

For now, openocd support is available only on upstream OpenOCD. You can check
`OpenOCD official Github mirror`_.
In order to use it, you should clone and compile it following usual README
guidelines.
Once it is done, you can set the OPENOCD and OPENOCD_DEFAULT_PATH variables in
:zephyr_file:`boards/st/nucleo_wba52cg/board.cmake` to point the build
to the paths of the OpenOCD binary and its scripts,  before
including the common openocd.board.cmake file:

   .. code-block:: none

      set(OPENOCD "<path_to_opneocd_repo>/src/openocd" CACHE FILEPATH "" FORCE)
      set(OPENOCD_DEFAULT_PATH <path_to_opneocd_repo>/tcl)
      include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)

Flashing
========

STM32CubeProgrammer is configured as flashing tool by default.
If available OpenOCD could be used. Same process applies with both tools.

Flashing an application to Nucleo WBA52CG
-----------------------------------------

Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: nucleo_wba52cg
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
   :board: nucleo_wba52cg
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

.. _Nucleo WBA52CG website:
   https://www.st.com/en/evaluation-tools/nucleo-wba52cg.html

.. _STM32WBA52CG on www.st.com:
   https://www.st.com/en/microcontrollers-microprocessors/stm32wba52cg.html

.. _STM32WBA52CG datasheet:
   https://www.st.com/resource/en/datasheet/stm32wba52cg.pdf

.. _STM32WBA52CG reference manual:
   https://www.st.com/resource/en/reference_manual/rm0493-multiprotocol-wireless-bluetooth-lowenergy-armbased-32bit-mcu-stmicroelectronics.pdf

.. _OpenOCD official Github mirror:
   https://github.com/openocd-org/openocd/commit/870769b0ba9f4dae6ada9d8b1a40d75bd83aaa06
