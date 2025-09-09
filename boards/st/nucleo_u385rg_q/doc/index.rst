.. zephyr:board:: nucleo_u385rg_q

Overview
********

The Nucleo U385RG board, featuring an ARM |reg| Cortex |reg| -M33 with
TrustZone |reg| based STM32U385RG MCU, provides an affordable and flexible
way for users to try out new concepts and build prototypes by choosing from
the various combinations of performance and power consumption features.
Here are some highlights of the Nucleo U385RG board:

- STM32U385RG microcontroller in an LQFP64 or LQFP48 package
- Two types of extension resources:

  - Arduino |reg| Uno V3 connectivity
  - ST morpho extension pin headers for full access to all STM32U3 I/Os

- On-board STLINK-V2EC debugger/programmer with USB re-enumeration
  capability: mass storage, Virtual COM port, and debug port
- Flexible board power supply:

  - USB VBUS or external source(3.3V, 5V, 7 - 12V)

- Two push-buttons: USER and RESET
- 32.768 kHz crystal oscillator
- Second user LED shared with ARDUINO |reg| Uno V3
- External or internal SMPS to generate Vcore logic supply
- 24 MHz or 48 MHz HSE
- User USB Device full speed, or USB SNK/UFP full speed
- Cryptography
- CAN FD transceiver
- Board connectors:

 - External SMPS experimentation dedicated connector
 - USB Type-C |reg| , Micro-B, or Mini-B connector for the ST-LINK
 - USB Type-C |reg| user connector
 - MIPI |reg| debug connector
 - CAN FD header

More information about the board can be found at the `NUCLEO_U385RG website`_.

Hardware
********

The STM32U385xx devices are an ultra-low-power microcontrollers family (STM32U3
Series) based on the high-performance Arm |reg| Cortex |reg|-M33 32-bit RISC core.
They operate at a frequency of up to 96 MHz.

- Includes ST state-of-the-art patented technology
- Ultra-low-power with FlexPowerControl:

  - 1.71 V to 3.6 V power supply
  - -40 °C to +105 °C temperature range
  - VBAT mode: supply for RTC, 32 x 32-bit backup registers
  - 1.6 μA Stop 3 mode with 8-Kbyte SRAM
  - 2.2 μA Stop 3 mode with full SRAM
  - 3.8 μA Stop 2 mode with 8-Kbyte SRAM
  - 4.5 μA Stop 2 mode with full SRAM
  - 9.5 μA/MHz Run mode @ 3.3 V (While(1) SMPS step-down converter mode)
  - 13 μA/MHz Run mode @ 3.3 V/48 MHz (CoreMark |reg| SMPS step-down converter mode)
  - 16 μA/MHz Run mode @ 3.3 V/96 MHz (CoreMark |reg| SMPS step-down converter mode)
  - Brownout reset

- Core:

  - 32-bit Arm |reg| Cortex |reg|-M33 CPU with TrustZone |reg| and FPU

- ART Accelerator:

  - 8-Kbyte instruction cache allowing 0-wait-state execution from flash and external memories:
    frequency up to 96 MHz, MPU, 144 DMIPS and DSP instructions

- Power management:

  - Embedded regulator (LDO) and SMPS step-down converter supporting switch on-the-fly and voltage scaling

- Benchmarks:

  - 1.5 DMIPS/MHz (Drystone 2.1)
  - 387 CoreMark |reg| (4.09 CoreMark/MHz at 56 MHz)
  - 500 ULPMark |trade| -CP
  - 117 ULPMark |trade| -CM
  - 202000 SecureMark |trade| -TLS

- Memories:

  - 1-Mbyte flash memory with ECC, 2 banks read-while-write
  - 256 Kbytes of SRAM including 64 Kbytes with hardware parity check
  - OCTOSPI external memory interface supporting SRAM, PSRAM, NOR, NAND, and FRAM memories

- General-purpose input/outputs:

  - Up to 82 fast I/Os with interrupt capability most 5 V-tolerant and up to 14 I/Os with independent supply down to 1.08 V

- Clock management:

  - 4 to 50 MHz crystal oscillator
  - 32.768 kHz crystal oscillator for RTC (LSE)
  - Internal 16 MHz factory-trimmed RC (±1 %)
  - Internal low-power RC with frequency 32 kHz or 250 Hz (±5 %)
  - 2 internal multispeed 3 MHz to 96 MHz oscillators
  - Internal 48 MHz with clock recovery
  - Accurate MSI in PLL-mode and up to 96 MHz with 32.768 kHz, 16 MHz, or 32 MHz crystal oscillator

- Security and cryptography:

  - Arm |reg| TrustZone |reg| and securable I/Os, memories, and peripherals
  - Flexible life cycle scheme with RDP and password protected debug
  - Root of trust due to unique boot entry and secure hide protection area (HDP)
  - Secure firmware installation (SFI) from embedded root secure services (RSS)
  - Secure data storage with hardware unique key (HUK)
  - Secure firmware upgrade
  - Support of Trusted firmware for Cortex |reg| M (TF-M)
  - Two AES coprocessors, one with side channel attack resistance (SCA) (SAES)
  - Public key accelerator, SCA resistant
  - Key hardware protection
  - Attestation keys
  - HASH hardware accelerator
  - True random number generator, NIST SP800-90B compliant
  - 96-bit unique ID
  - 512-byte OTP (one-time programmable)
  - Antitamper protection

- Up to 15 timers and 2 watchdogs :

  - 1x 16-bit advanced motor-control
  - 3x 32-bit and 3x 16-bit general purpose
  - 2x 16-bit basic
  - 4x low-power 16-bit timers (available in Stop mode)
  - 2x watchdogs
  - 2x SysTick timer
  - RTC with hardware calendar
  - Alarms
  - Calibration

- Up to 19 communication peripherals:

  - 1 USB 2.0 full-speed controller
  - 1 SAI (serial audio interface)
  - 3 I2C FM+(1 Mbit/s), SMBus/PMBus |trade|
  - 2 I3C (SDR), with support of I2C FM+ mode
  - 2 USARTs and 2 UARTs (SPI, ISO 7816, LIN, IrDA, modem), 1 LPUART
  - 3 SPIs (6 SPIs including 1 with OCTOSPI + 2 with USART)
  - 1 CAN FD controller
  - 1 SDMMC interface
  - 1 audio digital filter with sound-activity detection

- 12-channel GPDMA controller, functional in Sleep and Stop modes (up to Stop 2)
- Up to 21 capacitive sensing channels:

  - Support touch key, linear, and rotary touch sensors

- Rich analog peripherals (independent supply):

  - 2x 12-bit ADC 2.5 Msps, with hardware oversampling
  - 12-bit DAC module with 2 D/A converters, low-power sample and hold, autonomous in Stop 1 mode
  - 2 operational amplifiers with built-in PGA
  - 2 ultralow-power comparators

- CRC calculation unit
- Debug:

  - Development support: serial-wire debug (SWD), JTAG, Embedded Trace Macrocell |trade| (ETM)

- ECOPACK2 compliant packages

More information about STM32U385RG can be found here:

- `STM32U385RG on www.st.com`_
- `STM32U385RG reference manual`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

Nucleo U385RG Board has 14 GPIO controllers. These controllers are responsible
for pin muxing, input/output, pull-up, etc.

For more details please refer to `STM32U385RG board user manual`_.

Default Zephyr Peripheral Mapping:
----------------------------------

- DAC1_OUT1 : PA4
- I2C1 SCL/SDA : PB6/PB7 (Arduino I2C)
- FDCAN1_TX : PA12
- FDCAN1_RX : PA11
- LD4 : PA5
- LPUART_1_TX : PA2
- LPUART_1_RX : PA3
- SPI3 NSS/SCK/MISO/MOSI : PA15/PB3/PB4/PB5 (Arduino SPI)
- UART_1_TX : PA9
- UART_1_RX : PA10
- UART_3_TX : PC10
- UART_3_RX : PC11
- USER_PB : PC13

System Clock
------------

Nucleo U385RG System Clock could be driven by internal or external oscillator,
as well as main PLL clock. By default System clock is driven by PLL clock at
48MHz, driven by 4MHz medium speed internal oscillator.

Serial Port
-----------

Nucleo U385RG board has 4 U(S)ARTs, 1 LPUART. The Zephyr console output is assigned to
USART1. Default settings are 115200 8N1.

CAN
---
The STM32U385RG_Q does not have an onboard CAN transceiver. In
order to use the CAN bus on the this board, an external CAN bus
transceiver must be connected to ``PA11`` (``FDCAN1_RX``) and ``PA12``
(``FDCAN1_TX``).


Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Nucleo U385RG board includes an ST-LINK/V3 embedded debug tool interface.
This probe allows to flash the board using various tools.

Flashing
========

The board is configured to be flashed using west `STM32CubeProgrammer`_ runner,
so its :ref:`installation <stm32cubeprog-flash-host-tools>` is required.

Alternatively, JLink or pyOCD can also be used to flash the board using
the ``--runner`` (or ``-r``) option:

.. code-block:: console

   $ west flash --runner pyocd
   $ west flash --runner jlink

For pyOCD, additional target information needs to be installed
by executing the following pyOCD commands:

.. code-block:: console

   $ pyocd pack --update
   $ pyocd pack --install stm32u3


Flashing an application to Nucleo U385RG
----------------------------------------

Connect the Nucleo U385RG to your host computer using the USB port.
Then build and flash an application. Here is an example for the
:zephyr:code-sample:`hello_world` application.

Run a serial host program to connect with your Nucleo board:

.. code-block:: console

   $ minicom -D /dev/ttyACM0

Then build and flash the application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: nucleo_u385rg_q
   :goals: build flash

You should see the following message on the console:

.. code-block:: console

   Hello World! nucleo_u385rg_q

Debugging
=========

Default debugger for this board is OpenOCD. It can be used in the usual way.
Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: nucleo_u385rg_q
   :goals: debug

Note: Check the ``build/tfm`` directory to ensure that the commands required by these scripts
(``readlink``, etc.) are available on your system. Please also check ``STM32_Programmer_CLI``
(which is used for initialization) is available in the PATH.

.. _NUCLEO_U385RG website:
  https://www.st.com/en/evaluation-tools/nucleo-u385rg.html

.. _STM32U385RG board user manual:
   https://www.st.com/resource/en/user_manual/um3062-stm32u3u5-nucleo64-board-mb1841-stmicroelectronics.pdf

.. _STM32U385RG on www.st.com:
   https://www.st.com/en/microcontrollers-microprocessors/stm32u385rg

.. _STM32U385RG reference manual:
   https://www.st.com/resource/en/reference_manual/rm0487-stm32u3-series-armbased-32bit-mcus-stmicroelectronics.pdf

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html
