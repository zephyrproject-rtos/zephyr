.. zephyr:board:: nucleo_u545re_q

Overview
********
The Nucleo U545RE Q board, featuring an ARM Cortex-M33 based STM32U545RE MCU,
provides an affordable and flexible way for users to try out new concepts and
build prototypes by choosing from the various combinations of performance and
power consumption features. Here are some highlights of the Nucleo U545RE Q
board:


- STM32U545RE microcontroller in an LQFP64 package
- 32.768 kHz crystal oscillator
- 24 MHz or 48 MHz HSE
- External or internal SMPS to generate Vcore logic supply
- Two types of extension resources:

  - Arduino Uno V3 connectivity
  - ST morpho extension pin headers for full access to all STM32 I/Os

- On-board ST-LINK/V3E debugger/programmer with USB re-enumeration
  capability: mass storage, Virtual COM port, and debug port
- Flexible power-supply options:

  - USB VBUS
  - external source (3.3V, 5V, 7 - 12V)
  - STLINK-V3E

- External or internal SMPS to generate Vcore logic supply
- One users LEDs
- Two push-buttons: USER and RESET
- Cryptography
- CAN FD transceiver
- User USB Device full speed, or USB SNK/UFP full speed

More information about the board can be found at the `NUCLEO_U545RE website`_.

Hardware
********
The STM32U545xx devices belong to an ultra-low-power microcontrollers family (STM32U5
Series) based on the high-performance Arm® Cortex®-M33 32-bit RISC core. They operate
at a frequency of up to 160 MHz.

- Includes ST state-of-the-art patented technology
- Ultra-low-power with FlexPowerControl:

  - 1.71 V to 3.6 V power supply
  - -40 °C to +85/125 °C temperature range
  - Low-power background-autonomous mode (LPBAM): autonomous peripherals with DMA,
    functional down to Stop 2 mode
  - VBAT mode: supply for RTC, 32 x 32-bit backup registers and 2-Kbyte backup SRAM
  - 90 nA Shutdown mode (23 wake-up pins)
  - 200 nA Standby mode (23 wake-up pins)
  - 370 nA Standby mode with RTC
  - 1.4 μA Stop 3 mode with 16-Kbyte SRAM
  - 2.2 µA Stop 3 mode with full SRAM
  - 3.0 µA Stop 2 mode with 16-Kbyte SRAM
  - 4.6 µA Stop 2 mode with full SRAM
  - 16.3 μA/MHz Run mode @ 3.3 V

- Core:

  - Arm® 32-bit Cortex®-M33 CPU with TrustZone®, MPU, DSP, and FPU

- ART Accelerator:

  - 8-Kbyte instruction cache allowing 0-wait-state execution from flash and
    external memories: up to 160 MHz, 240 DMIPS
  - 4-Kbyte data cache for external memories

- Power management:

  - Embedded regulator (LDO) and SMPS step-down converter supporting switchon-the-fly
    and voltage scaling

- Benchmarks:

  - 1.5 DMIPS/MHz (Dhrystone 2.1)
  - 651 CoreMark® (4.07 CoreMark®/MHz)
  - 464 ULPMark™-CP
  - 125 ULPMark™-PP
  - 54 ULPMark™-CM
  - 137000 SecureMark™-TLS
  - 512-Kbyte flash memory with ECC, 2 banks read-while-write, and 100 kcycles
  - 274-Kbyte SRAM including up to 64-Kbyte SRAM with ECC ON
  - 1 Octo-SPI memory interface

- Security and cryptography:

  - SESIP3 and PSA Level 3 Certified Assurance Target
  - Arm® TrustZone® and securable I/Os, memories and peripherals
  - Flexible life cycle scheme with RDP and password protected debug
  - Root of trust thanks to unique boot entry and secure hide protection area (HDP)
  - Secure firmware installation (SFI) thanks to embedded root-secure services (RSS)
  - Secure data storage with hardware-unique key (HUK)
  - Secure firmware upgrade support with TF-M
  - 2 AES coprocessors including one withDPA resistance
  - Public key accelerator, DPA resistant
  - On-the-fly decryption of Octo-SPI external memories
  - HASH hardware accelerator
  - True random number generator, NIST SP800-90B compliant
  - 96-bit unique ID
  - 512-byte OTP (one-time programmable)
  - Active tampers

- Clock management:

  - 4 to 50 MHz crystal oscillator
  - 32 kHz crystal oscillator for RTC (LSE)
  - Internal 16 MHz factory-trimmed RC (±1%)
  - Internal low-power 32 kHz RC (±5%)
  - 2 internal multispeed 100 kHz to 48 MHz oscillators, including one auto-trimmed
    by LSE (better than ±0.25% accuracy)
  - Internal 48 MHz with clock recovery
  - 3 PLL for system clock, USB, audio, ADC

- General-purpose input/outputs:

  - Up to 82 fast I/Os with interrupt capability most 5V-tolerant and up to 14 I/Os
    with independent supply down to 1.08 V

- Up to 17 timers and 2 watchdogs:

  - 2 16-bit advanced motor-control, 4 32-bit, 5 16-bit, 4 low-power 16-bit
    (available in Stop mode), 2 SysTick timers and 2 watchdogs
  - RTC with hardware calendar and calibration

- Up to 19 communication peripherals

  - 1 USB full-speed selectable host or device controller
  - 1 SAI (serial-audio interface)
  - 4 I2C FM+(1 Mbit/s), SMBus/PMBus®
  - 5 USART/UART/LPUART (SPI, ISO 7816, LIN, IrDA, modem)
  - 3 SPI (+1 with OCTOSPI +2 with USART)
  - 1 CAN FD controller
  - 1 SDMMC interface
  - 1 multi-function digital filter (2 filters) + 1 audio digital filter with
    sound-activity detection
  - Parallel synchronous slave interface

- 16- and 4-channel DMA controllers, functional in Stop mode
- Graphic features :

  - 1 digital camera interface

- Mathematical co-processor:

  - CORDIC for trigonometric functions acceleration
  - Filter mathematical accelerator (FMAC)

- Up to 20 capacitive sensing channels:

  - Support touch key, linear and rotary touch sensors

- Rich analog peripherals (independent supply) :

  - 14-bit ADC 2.5-Msps with hardware oversampling
  - 12-bit ADC 2.5-Msps, with hardware oversampling, autonomous in Stop 2 mode
  - 2 12-bit DAC, low-power sample and hold
  - 1 operational amplifier with built-in PGA
  - 1 ultra-low-power comparator

- CRC calculation unit
- Debug:

  - Development support: serial-wire debug (SWD), JTAG, Embedded Trace Macrocell™ (ETM)

- ECOPACK2 compliant packages

More information about STM32U545RE can be found here:

- `STM32U545RE on www.st.com`_
- `STM32U545RE reference manual`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

Nucleo U545RE Board has 7 GPIO controllers. These controllers are responsible
for pin muxing, input/output, pull-up, etc.

For more details please refer to `STM32U545RE board user manual`_.

Default Zephyr Peripheral Mapping:
----------------------------------

- ADC1 IN5/IN6 : PA0/PA1 (Arduino A0/A1)
- DAC1_OUT1 : PA4
- FDCAN1_TX : PA12
- FDCAN1_RX : PA11
- I2C1 SCL/SDA : PB6/PB7 (Arduino I2C)
- I2C2 SCL/SDA : PB10/PB14
- LD2 : PA5
- LPUART_1_TX : PA2
- LPUART_1_RX : PA3
- SPI1 NSS/SCK/MISO/MOSI : PA4/PA5/PA6/PA7 (Arduino SPI)
- USART_1_TX : PA9
- USART_1_RX : PA10
- USER_PB : PC13

System Clock
------------

Nucleo U545RE Q System Clock could be driven by internal or external oscillator,
as well as main PLL clock. By default System clock is driven by PLL clock at
160MHz, driven by 4MHz medium speed internal oscillator.

Serial Port
-----------

Nucleo U545RE Q board has 5 U(S)ARTs. The Zephyr console output is assigned to
USART1. Default settings are 115200 8N1.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Nucleo U545RE-Q board includes an ST-LINK/V3 embedded debug tool interface.
This probe allows to flash the board using various tools.

Flashing
========

The board is configured to be flashed using west `STM32CubeProgrammer`_ runner,
so its :ref:`installation <stm32cubeprog-flash-host-tools>` is required.

Alternatively JLink, or pyOCD can also be used to flash the board using
the ``--runner`` (or ``-r``) option:

.. code-block:: console

   $ west flash --runner pyocd
   $ west flash --runner jlink

For pyOCD, additional target information needs to be installed
by executing the following pyOCD commands:

.. code-block:: console

   $ pyocd pack --update
   $ pyocd pack --install stm32u5

Flashing an application to Nucleo U545RE
----------------------------------------

Connect the Nucleo U545RE to your host computer using the USB port.
Then build and flash an application. Here is an example for the
:zephyr:code-sample:`hello_world` application.

Run a serial host program to connect with your Nucleo board:

.. code-block:: console

   $ minicom -D /dev/ttyACM0

Then build and flash the application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: nucleo_u545re_q
   :goals: build flash

You should see the following message on the console:

.. code-block:: console

   Hello World! nucleo_u545re_q/stm32u545xx

Debugging
=========

Default debugger for this board is pyOCD. It can be used in the usual way.
Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: nucleo_u545re_q
   :goals: debug

.. _NUCLEO_U545RE website:
   https://www.st.com/en/evaluation-tools/nucleo-u545re-q.html

.. _STM32U545RE board user manual:
   https://www.st.com/resource/en/user_manual/um3062-stm32u3u5-nucleo64-board-mb1841-stmicroelectronics.pdf

.. _STM32U545RE on www.st.com:
   https://www.st.com/en/microcontrollers-microprocessors/stm32u545re.html

.. _STM32U545RE reference manual:
   https://www.st.com/resource/en/reference_manual/rm0456-stm32u5-series-armbased-32bit-mcus-stmicroelectronics.pdf

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html
