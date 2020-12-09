.. _stm32l562e_dk_board:

ST STM32L562E-DK Discovery
##########################

Overview
********

The STM32L562E-DK Discovery kit is designed as a complete demonstration and
development platform for STMicroelectronics Arm |reg| Cortex |reg|-M33 core-based
STM32L562QEI6QU microcontroller with TrustZone |reg|. Here are some highlights of
the STM32L562E-DK Discovery board:


- STM32L562QEI6QU microcontroller featuring 512 Kbytes of Flash memory and 256 Kbytes of SRAM in BGA132 package
- 1.54" 240 x 240 pixel-262K color TFT LCD module with parallel interface and touch-control panel
- USB Type-C |trade| Sink device FS
- On-board energy meter: 300 nA to 150 mA measurement range with a dedicated USB interface
- SAI Audio CODEC
- MEMS digital microphones
- 512-Mbit Octal-SPI Flash memory
- Bluetooth |reg| V4.1 Low Energy module
- iNEMO 3D accelerometer and 3D gyroscope
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

- 2 user LEDs
- User and reset push-buttons

.. image:: img/stm32l562e_dk.jpg
   :width: 460px
   :align: center
   :height: 474px
   :alt: STM32L562E-DK Discovery

More information about the board can be found at the `STM32L562E-DK Discovery website`_.

Hardware
********

The STM32L562xx devices are an ultra-low-power microcontrollers family (STM32L5
Series) based on the high-performance Arm |reg| Cortex |reg|-M33 32-bit RISC core.
They operate at a frequency of up to 110 MHz.

- Ultra-low-power with FlexPowerControl (down to 108 nA Standby mode and 62 uA/MHz run mode)
- Core: ARM |reg| 32-bit Cortex |reg| -M33 CPU with TrustZone |reg| and FPU.
- Performance benchmark:

  - 1.5 DMPIS/MHz (Drystone 2.1)
  - 442 CoreMark |reg| (4.02 CoreMark |reg| /MHZ)

- Security

  - Arm |reg| TrustZone |reg| and securable I/Os memories and peripherals
  - Flexible life cycle scheme with RDP (readout protection)
  - Root of trust thanks to unique boot entry and hide protection area (HDP)
  - Secure Firmware Installation thanks to embedded Root Secure Services
  - Secure Firmware Update support with TF-M
  - AES coprocessor
  - Public key accelerator
  - On-the-fly decryption of Octo-SPI external memories
  - HASH hardware accelerator
  - Active tamper and protection temperature, voltage and frequency attacks
  - True Random Number Generator NIST SP800-90B compliant
  - 96-bit unique ID
  - 512-byte One-Time Programmable for user data

- Clock management:

  - 4 to 48 MHz crystal oscillator
  - 32 kHz crystal oscillator for RTC (LSE)
  - Internal 16 MHz factory-trimmed RC ( |plusminus| 1%)
  - Internal low-power 32 kHz RC ( |plusminus| 5%)
  - Internal multispeed 100 kHz to 48 MHz oscillator, auto-trimmed by
    LSE (better than  |plusminus| 0.25 % accuracy)
  - 3 PLLs for system clock, USB, audio, ADC

- Power management

  - Embedded regulator (LDO) with three configurable range output to supply the digital circuitry
  - Embedded SMPS step-down converter
  - External SMPS support

- RTC with HW calendar, alarms and calibration
- Up to 114 fast I/Os, most 5 V-tolerant, up to 14 I/Os with independent supply down to 1.08 V
- Up to 22 capacitive sensing channels: support touchkey, linear and rotary touch sensors
- Up to 16 timers and 2 watchdogs

  - 2x 16-bit advanced motor-control
  - 2x 32-bit and 5x 16-bit general purpose
  - 2x 16-bit basic
  - 3x low-power 16-bit timers (available in Stop mode)
  - 2x watchdogs
  - 2x SysTick timer

- Memories

  - Up to 512 MB Flash, 2 banks read-while-write
  - 512 KB of SRAM including 64 KB with hardware parity check
  - External memory interface for static memories supporting SRAM, PSRAM, NOR, NAND and FRAM memories
  - OCTOSPI memory interface

- Rich analog peripherals (independent supply)

  - 3x 12-bit ADC 5 MSPS, up to 16-bit with hardware oversampling, 200 uA/MSPS
  - 2x 12-bit DAC, low-power sample and hold
  - 2x operational amplifiers with built-in PGA
  - 2x ultra-low-power comparators
  - 4x digital filters for sigma delta modulator

- 19x communication interfaces

  - USB Type-C / USB power delivery controller
  - 2.0 full-speed crystal less solution, LPM and BCD
  - 2x SAIs (serial audio interface)
  - 4x I2C FM+(1 Mbit/s), SMBus/PMBus
  - 6x USARTs (ISO 7816, LIN, IrDA, modem)
  - 3x SPIs (7x SPIs with USART and OCTOSPI in SPI mode)
  - 1xFDCAN
  - 1xSDMMC interface
  - 2x 14 channel DMA controllers

- CRC calculation unit
- Development support: serial wire debug (SWD), JTAG, Embedded Trace Macrocell |trade|


More information about STM32L562QE can be found here:

- `STM32L562QE on www.st.com`_
- `STM32L562 reference manual`_

Supported Features
==================

The Zephyr stm32l562e_dk board configuration supports the following hardware features:

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

Other hardware features are not yet supported on this Zephyr port.

The default configuration can be found in the defconfig file:
``boards/arm/stm32l562e_dk/stm32l562e_dk_defconfig``


Connections and IOs
===================

STM32L562E-DK Discovery Board has 8 GPIO controllers. These controllers are responsible for pin muxing,
input/output, pull-up, etc.

For mode details please refer to `STM32L562E-DK Discovery board User Manual`_.

Default Zephyr Peripheral Mapping:
----------------------------------

- USART_1 TX/RX : PA9/PA10
- I2C_1 SCL/SDA : PB6/PB7
- SPI_1 SCK/MISO/MOSI : PG2/PG3/PG4 (BT SPI bus)
- USER_PB : PC13
- LD10 : PG12

System Clock
------------

STM32L562E-DK System Clock could be driven by internal or external oscillator,
as well as main PLL clock. By default System clock is driven by PLL clock at
110MHz, driven by 4MHz medium speed internal oscillator.

Serial Port
-----------

STM32L562E-DK Discovery board has 6 U(S)ARTs. The Zephyr console output is
assigned to USART1. Default settings are 115200 8N1.


Programming and Debugging
*************************

Applications for the ``stm32l562e_dk`` board configuration can be built and
flashed in the usual way (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Flashing
========

STM32L562E-DK Discovery board includes an ST-LINK/V3E embedded debug tool
interface. This interface is not yet supported by the openocd version.
Instead, support can be enabled on pyocd by adding "pack" support with
the following pyocd command:

.. code-block:: console

   $ pyocd pack --update
   $ pyocd pack --install stm32l562qe

STM32L562E-DK Discovery board includes an ST-LINK/V2-1 embedded debug tool
interface.  This interface is supported by the openocd version
included in the Zephyr SDK since v0.9.2.

Flashing an application to STM32L562E-DK Discovery
--------------------------------------------------

Connect the STM32L562E-DK Discovery to your host computer using the USB port.
Then build and flash an application. Here is an example for the
:ref:`hello_world` application.

Run a serial host program to connect with your Nucleo board:

.. code-block:: console

   $ minicom -D /dev/ttyACM0

Then build and flash the application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: stm32l562e_dk
   :goals: build flash

You should see the following message on the console:

.. code-block:: console

   Hello World! stm32l562e_dk

Debugging
=========

You can debug an application in the usual way.  Here is an example for the
:ref:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: stm32l562e_dk
   :maybe-skip-config:
   :goals: debug

.. _STM32L562E-DK Discovery website:
   https://www.st.com/en/evaluation-tools/stm32l562e-dk.html

.. _STM32L562E-DK Discovery board User Manual:
   https://www.st.com/resource/en/user_manual/dm00635554.pdf

.. _STM32L562QE on www.st.com:
   https://www.st.com/en/microcontrollers/stm32l562qe.html

.. _STM32L562 reference manual:
   http://www.st.com/resource/en/reference_manual/DM00346336.pdf
