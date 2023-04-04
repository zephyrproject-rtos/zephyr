.. _stm32h573i_dk_board:

ST STM32H573I-DK Discovery
##########################

Overview
********

The STM32H573I-DK Discovery kit is designed as a complete demonstration and
development platform for STMicroelectronics Arm |reg| Cortex |reg|-M33 core-based
STM32H573IIK3Q microcontroller with TrustZone |reg|. Here are some highlights of
the STM32H573I-DK Discovery board:


- STM32H573IIK3Q microcontroller featuring 2 Mbytes of Flash memory and 640 Kbytes of SRAM in 176-pin BGA package
- 1.54-inch 240x240 pixels TFT-LCD with LED  backlight and touch panel
- USB Type-C |trade| Host and device with USB power-delivery controller
- SAI Audio DAC stereo with one audio jacks for input/output,
- ST MEMS digital microphone with PDM interface
- Octo-SPI interface connected to 152Mbit Octo-SPI NORFlash memory device (MX25LM51245GXDI00 from MACRONIX)
- 10/100-Mbit Ethernet,
- microSD  |trade|
- A Wi‑Fi® add-on board
- Board connectors

  - STMod+ expansion connector with fan-out expansion board for Wi‑Fi |reg|, Grove and mikroBUS |trade| compatible connectors
  - Pmod |trade| expansion connector
  - Audio MEMS daughterboard expansion connector
  - ARDUINO |reg| Uno V3 expansion connector

- Flexible power-supply options

  - ST-LINK
  - USB VBUS
  - external sources

- On-board STLINK-V3E debugger/programmer with USB re-enumeration capability:

  - mass storage
  - Virtual COM port
  - debug port

- 4 user LEDs
- User and reset push-buttons

.. image:: img/stm32h573i_dk.jpg
   :align: center
   :alt: STM32H573I-DK Discovery

More information about the board can be found at the `STM32H573I-DK Discovery website`_.

Hardware
********

The STM32H573xx devices are an high-performance microcontrollers family (STM32H5
Series) based on the high-performance Arm |reg| Cortex |reg|-M33 32-bit RISC core.
They operate at a frequency of up to 250 MHz.

- Core: ARM |reg| 32-bit Cortex |reg| -M33 CPU with TrustZone |reg| and FPU.
- Performance benchmark:

  - 375 DMPIS/MHz (Dhrystone 2.1)

- Security

  - Arm |reg| TrustZone |reg| with ARMv8-M mainline security extension
  - Up to 8 configurable SAU regions
  - TrustZone |reg| aware and securable peripherals
  - Flexible lifecycle scheme with secure debug authentication
  - Preconfigured immutable root of trust (ST-iROT)
  - SFI (secure firmware installation)
  - Secure data storage with hardware unique key (HUK)
  - Secure firmware upgrade support with TF-M
  - 2x AES coprocessors including one with DPA resistance
  - Public key accelerator, DPA resistant
  - On-the-fly decryption of Octo-SPI external memories
  - HASH hardware accelerator
  - True random number generator, NIST SP800-90B compliant
  - 96-bit unique ID
  - Active tampers
  - True Random Number Generator (RNG) NIST SP800-90B compliant

- Clock management:

  - 25 MHz crystal oscillator (HSE)
  - 32 kHz crystal oscillator for RTC (LSE)
  - Internal 64 MHz (HSI) trimmable by software
  - Internal low-power 32 kHz RC (LSI)( |plusminus| 5%)
  - Internal 4 MHz oscillator (CSI), trimmable by software
  - Internal 48 MHz (HSI48) with recovery system
  - 3 PLLs for system clock, USB, audio, ADC

- Power management

  - Embedded regulator (LDO) with three configurable range output to supply the digital circuitry
  - Embedded SMPS step-down converter

- RTC with HW calendar, alarms and calibration
- Up to 139 fast I/Os, most 5 V-tolerant, up to 10 I/Os with independent supply down to 1.08 V
- Up to 16 timers and 2 watchdogs

  - 12x 16-bit
  - 2x 32-bit timers with up to 4 IC/OC/PWM or pulse counter and quadrature (incremental) encoder input
  - 6x 16-bit low-power 16-bit timers (available in Stop mode)
  - 2x watchdogs
  - 2x SysTick timer

- Memories

  - Up to 2 MB Flash, 2 banks read-while-write
  - 1 Kbyte OTP (one-time programmable)
  - 640 KB of SRAM including 64 KB with hardware parity check and 320 Kbytes with flexible ECC
  - 4 Kbytes of backup SRAM available in the lowest power modes
  - Flexible external memory controller with up to 16-bit data bus: SRAM, PSRAM, FRAM, SDRAM/LPSDR SDRAM, NOR/NAND memories
  - 1x OCTOSPI memory interface with on-the-fly decryption and support for serial PSRAM/NAND/NOR, Hyper RAM/Flash frame formats
  - 2x SD/SDIO/MMC interfaces

- Rich analog peripherals (independent supply)

  - 2x 12-bit ADC with up to 5 MSPS in 12-bit
  - 2x 12-bit D/A converters
  - 1x Digital temperature sensor

- 34x communication interfaces

  - 1x USB Type-C / USB power-delivery controller
  - 1x USB 2.0 full-speed host and device
  - 4x I2C FM+ interfaces (SMBus/PMBus)
  - 1x I3C interface
  - 12x U(S)ARTS (ISO7816 interface, LIN, IrDA, modem control)
  - 1x LP UART
  - 6x SPIs including 3 muxed with full-duplex I2S
  - 5x additional SPI from 5x USART when configured in Synchronous mode
  - 2x SAI
  - 2x FDCAN
  - 1x SDMMC interface
  - 2x 16 channel DMA controllers
  - 1x 8- to 14- bit camera interface
  - 1x HDMI-CEC
  - 1x Ethernel MAC interface with DMA controller
  - 1x 16-bit parallel slave synchronous-interface

- CORDIC for trigonometric functions acceleration
- FMAC (filter mathematical accelerator)
- CRC calculation unit
- Development support: serial wire debug (SWD), JTAG, Embedded Trace Macrocell |trade|


More information about STM32H573 can be found here:

- `STM32H573 on www.st.com`_
- `STM32H573 reference manual`_

Supported Features
==================

The Zephyr STM32H573I_DK board configuration supports the following
hardware features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| CLOCK     | on-chip    | reset and clock control             |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | gpio                                |
+-----------+------------+-------------------------------------+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| PINMUX    | on-chip    | pinmux                              |
+-----------+------------+-------------------------------------+
| RNG       | on-chip    | True Random number generator        |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port-polling;                |
|           |            | serial port-interrupt               |
+-----------+------------+-------------------------------------+
| WATCHDOG  | on-chip    | independent watchdog                |
+-----------+------------+-------------------------------------+
| DAC       | on-chip    | DAC Controller                      |
+-----------+------------+-------------------------------------+
| ADC       | on-chip    | ADC Controller                      |
+-----------+------------+-------------------------------------+
| PWM       | on-chip    | PWM                                 |
+-----------+------------+-------------------------------------+
| RTC       | on-chip    | Real Time Clock                     |
+-----------+------------+-------------------------------------+

Other hardware features are not yet supported on this Zephyr port.

The default configuration can be found in the defconfig and dts files:

- Secure target:

  - :zephyr_file:`boards/arm/stm32h573i_dk/stm32h573i_dk_defconfig`
  - :zephyr_file:`boards/arm/stm32h573i_dk/stm32h573i_dk.dts`

Zephyr board options
====================

The STM32H573 is an SoC with Cortex-M33 architecture. Zephyr provides support
for building for Secure firmware.

The BOARD options are summarized below:

+----------------------+-----------------------------------------------+
|   BOARD              | Description                                   |
+======================+===============================================+
| stm32h573i_dk        | For building Secure firmware                  |
+----------------------+-----------------------------------------------+

Connections and IOs
===================

STM32H573I-DK Discovery Board has 9 GPIO controllers. These controllers are responsible for pin muxing,
input/output, pull-up, etc.

For mode details please refer to `STM32H573I-DK Discovery board User Manual`_.

Default Zephyr Peripheral Mapping:
----------------------------------

- USART_1 TX/RX : PA9/PA10 (VCP)
- USART_3 TX/RX : PB11/PB10  (Arduino USART3)
- USER_PB : PC13
- LD1 (green) : PI9
- DAC1 channel 1 output : PA4
- ADC1 channel 6 input : PF12

System Clock
------------

STM32H573I-DK System Clock could be driven by internal or external oscillator,
as well as main PLL clock. By default System clock is driven by PLL clock at
240MHz, driven by 25MHz external oscillator (HSE).

Serial Port
-----------

STM32H573I-DK Discovery board has 3 U(S)ARTs. The Zephyr console output is
assigned to USART1. Default settings are 115200 8N1.


Programming and Debugging
*************************

Applications for the ``stm32h573i_dk`` board configuration can be built and
flashed in the usual way (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Flashing
========

STM32H573I-DK Discovery board includes an ST-LINK/V3E embedded debug tool
interface. Support is available on STM32CubeProgrammer V2.13.0.

Alternatively, this interface will be supported by a next openocd version.

Flashing an application to STM32H573I-DK Discovery
--------------------------------------------------

Connect the STM32H573I-DK Discovery to your host computer using the USB port.
Then build and flash an application. Here is an example for the
:ref:`hello_world` application.

Run a serial host program to connect with your Nucleo board:

.. code-block:: console

   $ minicom -D /dev/ttyACM0

Then build and flash the application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: stm32h573i_dk
   :goals: build flash

You should see the following message on the console:

.. code-block:: console

   Hello World! stm32h573i_dk

Debugging
=========

You can debug an application in the usual way.  Here is an example for the
:ref:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: stm32h573i_dk
   :maybe-skip-config:
   :goals: debug

.. _STM32H573I-DK Discovery website:
   https://www.st.com/en/evaluation-tools/stm32h573i-dk.html

.. _STM32H573I-DK Discovery board User Manual:
   https://www.st.com/en/evaluation-tools/stm32h573i-dk.html

.. _STM32H573 on www.st.com:
   https://www.st.com/en/microcontrollers/stm32h573ii.html

.. _STM32H573 reference manual:
   https://www.st.com/resource/en/reference_manual/rm0481-stm32h563h573-and-stm32h562-armbased-32bit-mcus-stmicroelectronics.pdf

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html
