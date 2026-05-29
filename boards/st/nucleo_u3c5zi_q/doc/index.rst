.. zephyr:board:: nucleo_u3c5zi_q

Overview
********

The Nucleo U3C5ZI-Q board, featuring an ARM® Cortex®-M33 with TrustZone®
based STM32U3C5ZI MCU, provides an affordable and flexible way for users to
try out new concepts and build prototypes by choosing from the various
combinations of performance and power consumption features.
Here are some highlights of the Nucleo U3C5ZI-Q board:

- STM32U3C5ZI microcontroller in an LQFP144 package
- Two types of extension resources:

   - Arduino® Uno V3 connectivity
   - ST morpho extension pin headers for full access to all STM32U3 I/Os

- On-board ST-LINK debugger/programmer with USB re-enumeration capability:
   mass storage, Virtual COM port, and debug port
- Flexible board power supply:

   - USB VBUS or external source (3.3 V, 5 V, 7 - 12 V)

- Two push-buttons: USER and RESET
- Three user LEDs
- 32.768 kHz crystal oscillator
- USB full-speed device interface
- CAN FD connector
- M.2 Key A serial memory expansion connector

More information about the board can be found at the `NUCLEO-U3C5ZI-Q website`_.

Hardware
********

The STM32U3C5xx devices are an ultra-low-power microcontrollers family (STM32U3
Series) based on the high-performance Arm® Cortex®-M33 32-bit RISC core.
They operate at a frequency of up to 96 MHz.

- Includes ST state-of-the-art patented technology
- Ultra-low-power with FlexPowerControl:

   - 1.71 V to 3.6 V power supply
   - -40 °C to +105 °C temperature range
   - VBAT mode: supply for RTC, 32 x 32-bit backup registers
   - 1.6 μA Stop 3 mode with 8-Kbyte SRAM
   - 3.15 μA Stop 3 mode with full SRAM
   - 3.5 μA Stop 2 mode with 8-Kbyte SRAM
   - 5.7 μA Stop 2 mode with full SRAM
   - 12 μA/MHz Run mode @ 3.3 V, 48 MHz (While(1) SMPS step-down converter
     mode)
   - 15.5 μA/MHz Run mode @ 3.3 V, 48 MHz (CoreMark® SMPS step-down
     converter mode)
   - 20 μA/MHz Run mode @ 3.3 V, 96 MHz (CoreMark® SMPS step-down
     converter mode)
   - Brownout reset

- Core:

  - Arm® 32-bit Cortex®-M33 CPU with TrustZone® and FPU

- ART Accelerator:

  - 8-Kbyte instruction cache allowing 0-wait-state execution from flash and external memories:
    frequency up to 96 MHz, MPU, 144 DMIPS and DSP instructions

- Mathematical coprocessor:

  - Hardware signal processor (HSP) for digital signal and artificial intelligence processing

- Power management:

  - Embedded regulator (LDO) and SMPS step-down converter supporting switch on-the-fly and voltage scaling

- Benchmarks:

  - 395.4 CoreMark® (4.12 CoreMark®/MHz)

- Memories:

  - 2-Mbyte flash memory with ECC, 2 banks read-while-write
  - 640 Kbytes of SRAM including 384 Kbytes with hardware parity check
  - OCTOSPI external memory interface supporting SRAM, PSRAM, NOR, NAND, and FRAM memories

- Security and cryptography:

  - Arm® TrustZone® and securable I/Os, memories, and peripherals
  - Flexible life cycle scheme with RDP and password protected debug
  - Root of trust due to unique boot entry and secure hide protection area (HDP)
  - Secure firmware installation (SFI) from embedded root secure services (RSS)
  - Secure data storage with hardware unique key (HUK)
  - Secure firmware upgrade
  - Support of Trusted firmware for Cortex® M (TF-M)
  - Two AES coprocessors, one with side channel attack resistance (SCA) (SAES)
  - Public key accelerator, SCA resistant
  - Key hardware protection
  - Attestation keys
  - HASH hardware accelerator
  - True random number generator, NIST SP800-90B compliant
  - 96-bit unique ID
  - 512-byte OTP (one-time programmable)
  - Antitamper protection

- Clock management:

   - 4 to 50 MHz crystal oscillator
   - 32.768 kHz crystal oscillator for RTC (LSE)
   - Internal 16 MHz factory-trimmed RC (±1 %)
   - Internal low-power RC with frequency 32 kHz or 250 Hz (±5 %)
   - 2 internal multispeed 3 MHz to 96 MHz oscillators
   - Internal 48 MHz with clock recovery
   - Accurate MSI in PLL-mode and up to 96 MHz with 32.768 kHz, 16 MHz, or 32 MHz crystal oscillator

- General-purpose input/outputs:

   - Up to 114 fast I/Os with interrupt capability, most 5 V-tolerant, and up to 14 I/Os with independent supply down to 1.08 V

- Up to 17 timers and 2 watchdogs:

   - 2x 16-bit advanced motor-control, 3x 32-bit and 4x 16-bit general purpose, 2x 16-bit basic, 4x low-power 16-bit timers (available in Stop mode), 2x watchdogs, 2x SysTick timer
   - RTC with hardware calendar, alarms, and calibration

- Up to 23 communication peripherals:

   - 1 USB 2.0 full-speed controller
   - 1 SAI (serial audio interface)
   - 4 I2C FM+ (1 Mbit/s), SMBus/PMBus®
   - 2 I3C (SDR), with support of I2C FM+ mode
   - 3 USARTs and 2 UARTs (SPI, ISO 7816, LIN, IrDA, modem), 1 LPUART
   - 4 SPIs (7 SPIs including 1 with OCTOSPI + 2 with USART)
   - 2 CAN FD controllers
   - 1 SDMMC interface
   - 1 audio digital filter with sound-activity detection

- 12-channel GPDMA controller, functional in Sleep and Stop modes

- Up to 24 capacitive sensing channels:

   - Support touch key, linear, and rotary touch sensors

- Rich analog peripherals (independent supply):

   - 2x 12-bit ADCs 2.5 Msps, with hardware oversampling
   - 12-bit DAC module with 2 D/A converters, low-power sample and hold, autonomous in Stop 1 mode
   - 2 operational amplifiers with built-in PGA
   - 2 ultra-low-power comparators

- CRC calculation unit

- Debug:

  - Development support: serial-wire debug (SWD), JTAG, Embedded Trace Macrocell™ (ETM)

- ECOPACK2 compliant packages

More information about STM32U3C5ZI can be found here:

- `STM32U3C5ZI on www.st.com`_
- `STM32U3 reference manual`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

Nucleo U3C5ZI-Q board has multiple GPIO controllers. These controllers are
responsible for pin muxing, input/output, pull-up, and related configuration.

For more details please refer to the `NUCLEO-U3C5ZI-Q data brief`_.

Default Zephyr Peripheral Mapping:
----------------------------------

- ADC2_IN1 (V-sense) : PC2
- CAN FD 1 TX/RX : PF8/PF7
- FDCAN transceiver standby : PD7
- LD (User LED) : PA5
- UART_1_TX : PA9
- UART_1_RX : PA10
- USB_DM/USB_DP : PA11/PA12
- USER_PB : PC13
- VBUS detect : PF12

System Clock
------------

Nucleo U3C5ZI-Q system clock could be driven by internal or external oscillator,
as well as main PLL clock. By default, system clock is driven by PLL clock at
96 MHz.

Serial Port
-----------

Nucleo U3C5ZI-Q board has multiple U(S)ARTs and one LPUART. The Zephyr console
output is assigned to USART1. Default settings are 115200 8N1.

CAN
---

The board exposes CAN FD signals through FDCAN1 pins on ``PF7``
(``FDCAN1_RX``) and ``PF8`` (``FDCAN1_TX``).


Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Nucleo U3C5ZI-Q board includes an ST-LINK embedded debug interface.
This probe allows flashing the board using various tools.

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

Flashing an application to Nucleo U3C5ZI-Q
------------------------------------------

Connect the Nucleo U3C5ZI-Q to your host computer using the USB port.
Then build and flash an application. Here is an example for the
:zephyr:code-sample:`hello_world` application.

Run a serial host program to connect with your Nucleo board:

.. code-block:: console

   $ minicom -D /dev/ttyACM0

Then build and flash the application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: nucleo_u3c5zi_q
   :goals: build flash

You should see a hello message on the console.

.. code-block:: console

   Hello World! nucleo_u3c5zi_q

Debugging
=========

Default debugger for this board is pyOCD.
JLink can also be used by specifying the runner explicitly.
Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: nucleo_u3c5zi_q
   :goals: debug

.. _NUCLEO-U3C5ZI-Q website:
   https://www.st.com/en/evaluation-tools/nucleo-u3c5zi-q.html

.. _NUCLEO-U3C5ZI-Q data brief:
   https://www.st.com/resource/en/data_brief/nucleo-u3c5zi-q.pdf

.. _STM32U3C5ZI on www.st.com:
   https://www.st.com/en/microcontrollers-microprocessors/stm32u3c5zi.html

.. _STM32U3 reference manual:
   https://www.st.com/resource/en/reference_manual/rm0487-stm32u3-series-armbased-32bit-mcus-stmicroelectronics.pdf

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html
