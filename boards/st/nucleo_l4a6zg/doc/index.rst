.. zephyr:board:: nucleo_l4a6zg

Overview
********

The Nucleo L4A6ZG board features an ARM Cortex-M4 based STM32L4A6ZG MCU
with a wide range of connectivity support and configurations. Here are
some highlights of the Nucleo L4A6ZG board:


- STM32 microcontroller in QFP144 package
- USB OTG FS with Micro-AB connector
- Two types of extension resources:

  - Arduino Uno V3 connectivity
  - ST morpho extension pin headers for full access to all STM32 I/Os

- On-board ST-LINK/V2-1 debugger/programmer with SWD connector
- Flexible board power supply:

  - USB VBUS or external source(3.3V, 5V, 7 - 12V)
  - Power management access point

- 8 LEDs: user LEDs (LD1, LD2, LD3), communication LED (LD4), USB
  power fault(LD5), power LED (LD6), USB FS OTG (LD7, LD8)
- 2 push buttons: USER and RESET

More information about the board can be found at the `Nucleo L4A6ZG website`_.

Hardware
********

The STM32L4A6ZG SoC provides the following hardware capabilities:

- Ultra-low-power with FlexPowerControl (down to 108 nA Standby mode and 91 uA/MHz run mode)
- Core: ARM |reg| 32-bit Cortex |reg|-M4 CPU with FPU, frequency up to 80 MHz, 100DMIPS/1.25DMIPS/MHz (Dhrystone 2.1)
- Clock Sources:

  - 4 to 48 MHz crystal oscillator
  - 32 kHz crystal oscillator for RTC (LSE)
  - Internal 16 MHz factory-trimmed RC ( |plusminus| 1%)
  - Internal low-power 32 kHz RC ( |plusminus| 5%)
  - Internal multispeed 100 kHz to 48 MHz oscillator, auto-trimmed by
    LSE (better than  |plusminus| 0.25 % accuracy)
  - 3 PLLs for system clock, USB, audio, ADC

- RTC with HW calendar, alarms and calibration
- LCD 8 x 40 or 4 x 44 with step-up converter
- Up to 24 capacitive sensing channels: support touchkey, linear and rotary touch sensors
- 16x timers:

  - 2x 16-bit advanced motor-control
  - 2x 32-bit and 5x 16-bit general purpose
  - 2x 16-bit basic
  - 2x low-power 16-bit timers (available in Stop mode)
  - 2x watchdogs
  - SysTick timer

- Up to 136 fast I/Os, most 5 V-tolerant, up to 14 I/Os with independent supply down to 1.08 V
- Memories

  - Up to 1 MB Flash, 2 banks read-while-write, proprietary code readout protection
  - Up to 320 KB of SRAM including 64 KB with hardware parity check
  - External memory interface for static memories supporting SRAM, PSRAM, NOR and NAND memories
  - Quad SPI memory interface

- 4x digital filters for sigma delta modulator
- Rich analog peripherals (independent supply)

  - 3x 12-bit ADC 5 MSPS, up to 16-bit with hardware oversampling, 200 uA/MSPS
  - 2x 12-bit DAC, low-power sample and hold
  - 2x operational amplifiers with built-in PGA
  - 2x ultra-low-power comparators

- 20x communication interfaces

  - USB OTG 2.0 full-speed, LPM and BCD
  - 2x SAIs (serial audio interface)
  - 4x I2C FM+(1 Mbit/s), SMBus/PMBus
  - 5x U(S)ARTs (ISO 7816, LIN, IrDA, modem)
  - 1x LPUART
  - 3x SPIs (4x SPIs with the Quad SPI)
  - 2x CAN (2.0B Active) and SDMMC interface
  - SWPMI single wire protocol master I/F
  - IRTIM (Infrared interface)

- 14-channel DMA controller
- True random number generator
- CRC calculation unit, 96-bit unique ID
- AES and HASH hardware accelerators
- Development support: serial wire debug (SWD), JTAG, Embedded Trace Macrocell |trade|


More information about STM32L4A6ZG can be found here:

- `STM32L4A6ZG on www.st.com`_
- `STM32L4A6 reference manual`_

Supported Features
==================

The Zephyr nucleo_l4a6zg board configuration supports the following hardware features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| AES       | on-chip    | crypto                              |
+-----------+------------+-------------------------------------+
| FLASH     | on-chip    | flash memory                        |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | gpio                                |
+-----------+------------+-------------------------------------+
| I2C       | on-chip    | i2c                                 |
+-----------+------------+-------------------------------------+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| PINMUX    | on-chip    | pinmux                              |
+-----------+------------+-------------------------------------+
| PWM       | on-chip    | pwm                                 |
+-----------+------------+-------------------------------------+
| RTC       | on-chip    | rtc                                 |
+-----------+------------+-------------------------------------+
| SPI       | on-chip    | spi                                 |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port-polling;                |
|           |            | serial port-interrupt               |
+-----------+------------+-------------------------------------+
| WATCHDOG  | on-chip    | System Window Watchdog              |
+-----------+------------+-------------------------------------+

Other hardware features are not yet supported on this Zephyr port.

The default configuration can be found in the defconfig file:
:zephyr_file:`boards/st/nucleo_l4a6zg/nucleo_l4a6zg_defconfig`


Connections and IOs
===================

Nucleo L4A6ZG Board has 8 GPIO controllers. These controllers are responsible for pin muxing,
input/output, pull-up, etc.

For more details please refer to `STM32 Nucleo-144 board User Manual`_.

Default Zephyr Peripheral Mapping:
----------------------------------

- LPUART_1 TX/RX : PG7/PG8 (ST-Link Virtual COM Port)
- UART_3 TX/RX : PD8/PD9 (Arduino Serial)
- I2C_1 SCL/SDA : PB8/PB7 (Arduino I2C)
- SPI_1 SCK/MISO/MOSI/NSS : PA5/PA6/PA7/PD14 (Arduino SPI)
- USER_PB : PC13
- PWM_15_CH1 : PB14 (Red LED)
- LD1 : PC7 (Green LED)
- LD2 : PB7 (Blue LED)
- LD3 : PB14 (Red LED)

System Clock
------------

Nucleo L4A6ZG system clock could be driven by internal or external oscillator,
as well as main PLL clock. By default, system clock is driven by PLL at 80MHz, which is
driven by 16MHz high speed internal oscillator (HSI). High speed external oscillator
(HSE) is not soldered on the board, so it cannot be used to drive the PLL.

Serial Port
-----------

Nucleo L4A6ZG board has 5 UARTs. The Zephyr console output is assigned to LPUART1,
which is connected to the onboard ST-LINK/V2-1. Virtual COM port interface.
Default settings are 115200 8N1.


Programming and Debugging
*************************

Nucleo L4A6ZG board includes an ST-LINK/V2-1 embedded debug tool interface.

Applications for the ``nucleo_l4a6zg`` board configuration can be built and
flashed in the usual way (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Flashing
========

The board is configured to be flashed using west `STM32CubeProgrammer`_ runner,
so its :ref:`installation <stm32cubeprog-flash-host-tools>` is required.

Alternatively, OpenOCD or JLink can also be used to flash the board using
the ``--runner`` (or ``-r``) option:

.. code-block:: console

   $ west flash --runner openocd
   $ west flash --runner jlink

Flashing an application to Nucleo L4A6ZG
----------------------------------------

Connect the Nucleo L4A6ZG to your host computer using the ST-LINK USB port.
Then build and flash an application. Here is an example for the
:zephyr:code-sample:`hello_world` application.

Run a serial host program to connect with your Nucleo board:

.. code-block:: console

   $ minicom -D /dev/ttyUSB0

Then build and flash the application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: nucleo_l4a6zg
   :goals: build flash

You should see the following message on the console:

.. code-block:: console

   Hello World! nucleo_l4a6zg

Debugging
=========

You can debug an application in the usual way.  Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: nucleo_l4a6zg
   :maybe-skip-config:
   :goals: debug

.. _Nucleo L4A6ZG website:
   https://www.st.com/en/evaluation-tools/nucleo-l4a6zg.html

.. _STM32 Nucleo-144 board User Manual:
   https://www.st.com/resource/en/user_manual/dm00368330.pdf

.. _STM32L4A6ZG on www.st.com:
   https://www.st.com/en/microcontrollers-microprocessors/stm32l4a6zg.html

.. _STM32L4A6 reference manual:
   https://www.st.com/resource/en/reference_manual/dm00083560.pdf

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html
