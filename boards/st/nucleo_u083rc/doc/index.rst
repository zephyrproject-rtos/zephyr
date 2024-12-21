.. zephyr:board:: nucleo_u083rc

Overview
********

The Nucleo U083RC board, featuring an ARM Cortex-M0+ based STM32U083RC MCU,
provides an affordable and flexible way for users to try out new concepts and
build prototypes by choosing from the various combinations of performance and
power consumption features. Here are some highlights of the Nucleo U083RC
board:


- STM32U083RC microcontroller in LQFP64 package
- Two types of extension resources:

  - Arduino Uno V3 connectivity
  - ST morpho extension pin headers for full access to all STM32U0 I/Os

- On-board STLINK-V2EC debugger/programmer with USB re-enumeration
  capability: mass storage, Virtual COM port, and debug port
- Flexible board power supply:

  - USB VBUS or external source(3.3V, 5V, 7 - 12V)

- User LED shared with ARDUINO |reg| Uno V3
- Two push-buttons: USER and RESET
- USB Type-C |reg| connector for the ST-LINK

More information about the board can be found at the `NUCLEO_U083RC website`_.

Hardware
********

The STM32U083xC devices are an ultra-low-power microcontrollers family (STM32U0
Series) based on the high-performance Arm |reg| Cortex |reg|-M0+ 32-bit RISC core.
They operate at a frequency of up to 56 MHz.

- Includes ST state-of-the-art patented technology
- Ultra-low-power with FlexPowerControl:

  - 1.71 V to 3.6 V power supply
  - -40 °C to +85/125 °C temperature range
  - 130 nA VBAT mode: supply for RTC, 9 x 32-bit backup registers
  - 16 nA Shutdown mode (6 wake-up pins)
  - 30 nA Standby mode (6 wake-up pins) without RTC
  - 160 nA Standby mode with RTC
  - 825 nA Stop 2 mode with RTC
  - 695 nA Stop 2 mode without RTC
  - 4 µA wake-up from Stop mode
  - 52 µA/MHz Run mode
  - Brownout reset
  - SPI (3)
  - DMA Controller (2)

- Core:

  - 32-bit Arm |reg| Cortex |reg|-M0+ CPU, frequency up to 56 MHz

- ART Accelerator:

  - 1-Kbyte instruction cache allowing 0-wait-state execution from flash memory

- Benchmarks:

  - 1.13 DMIPS/MHz (Drystone 2.1)
  - 134 CoreMark |reg| (2.4 CoreMark/MHz at 56 MHz)
  - 407 ULPMark™-CP
  - 143 ULPMark™-PP
  - 19.7 ULPMark™-CM

- Memories:

  - 256-Kbyte single bank flash memory, proprietary code readout protection
  - 40-Kbyte SRAM with hardware parity check

- General-purpose input/outputs:

  - Up to 69 fast I/Os, most of them 5 V‑tolerant

- Clock management:

  - 4 to 48 MHz crystal oscillator
  - 32 kHz crystal oscillator for RTC (LSE)
  - Internal 16 MHz factory-trimmed RC (±1%)
  - Internal low-power 32 kHz RC (±5%)
  - Internal multispeed 100 kHz to 48 MHz oscillator,
    auto-trimmed by LSE (better than ±0.25 % accuracy)
  - Internal 48 MHz with clock recovery
  - PLL for system clock, USB, ADC

- Security:

  - Customer code protection
  - Robust read out protection (RDP): 3 protection level states
    and password-based regression (128-bit PSWD)
  - Hardware protection feature (HDP)
  - Secure boot
  - AES: 128/256-bit key encryption hardware accelerator
  - True random number generation, candidate for NIST SP 800-90B certification
  - Candidate for Arm |reg| PSA level 1 and SESIP level 3 certifications
  - 5 passive anti-tamper pins
  - 96-bit unique ID
  - True Random Number Generator (RNG) NIST SP800-90B compliant

- Up to 10 timers, 2 watchdogs and RTC:

  - 1x 16-bit advanced motor-control, 1x 32-bit and 3x 16-bit general purpose,
    2x 16-bit basic, 3x low-power 16-bit timers (available in Stop mode),
    2x watchdogs, SysTick timer
  - RTC with hardware calendar, alarms and calibration

- 3 low-power 16-bit timers (available in Stop mode).

- Up to 20 communication peripherals:

  - 1 USB 2.0 full-speed crystal-less solution with LPM and BCD
  - 7 USARTs/LPUARTs (SPI, ISO 7816, LIN, IrDA, modem)
  - 4 I2C interfaces supporting Fast-mode and Fast-mode Plus (up to 1 Mbit/s)
  - 3 SPIs, plus 4x USARTs in SPI mode
  - IRTIM (Infrared interface)

- Rich analog peripherals (independent supply):

  - 1x 12-bit ADC (0.4 µs conversion time), up to 16-bit with hardware oversampling
  - 1x 12-bit DAC output channel, low-power sample and hold
  - 1x general-purpose operational amplifier with built-in PGA (variable gain up to 16)
  - 2x ultra-low-power comparators

- ECOPACK2 compliant packages

More information about STM32U083RC can be found here:

- `STM32U083RC on www.st.com`_
- `STM32U083 reference manual`_

Supported Features
==================

The Zephyr nucleo_u083rc board configuration supports the following hardware features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| CLOCK     | on-chip    | reset and clock control             |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | gpio                                |
+-----------+------------+-------------------------------------+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port-polling;                |
|           |            | serial port-interrupt               |
+-----------+------------+-------------------------------------+
| WATCHDOG  | on-chip    | independent watchdog                |
+-----------+------------+-------------------------------------+
| ADC       | on-chip    | adc                                 |
+-----------+------------+-------------------------------------+
| DAC       | on-chip    | DAC Controller                      |
+-----------+------------+-------------------------------------+
| I2C       | on-chip    | i2c                                 |
+-----------+------------+-------------------------------------+
| PWM       | on-chip    | pwm                                 |
+-----------+------------+-------------------------------------+
| RTC       | on-chip    | Real Time Clock                     |
+-----------+------------+-------------------------------------+
| SPI       | on-chip    | spi                                 |
+-----------+------------+-------------------------------------+
| USB       | on-chip    | USB full-speed host/device bus      |
+-----------+------------+-------------------------------------+
| DMA       | on-chip    | Direct Memory Access Controller     |
+-----------+------------+-------------------------------------+
| RNG       | on-chip    | True Random number generator        |
+-----------+------------+-------------------------------------+
| AES       | on-chip    | crypto                              |
+-----------+------------+-------------------------------------+
| LPTIM     | on-chip    | Low Power Timer                     |
+-----------+------------+-------------------------------------+

Other hardware features are not yet supported on this Zephyr port.

The default configuration can be found in the defconfig file:
:zephyr_file:`boards/st/nucleo_u083rc/nucleo_u083rc_defconfig`


Connections and IOs
===================

Nucleo U083RC Board has 10 GPIO controllers. These controllers are responsible
for pin muxing, input/output, pull-up, etc.

For more details please refer to `STM32U083 User Manual`_.

Default Zephyr Peripheral Mapping:
----------------------------------

- DAC1_OUT1 : PA4
- I2C1_SCL : PB8
- I2C1_SDA : PB9
- LPUART_1_TX : PG7
- LPUART_1_RX : PG8
- SPI1_NSS : PA4
- SPI1_SCK : PA5
- SPI1_MISO : PA6
- SPI1_MOSI : PA7
- UART_2_TX : PA2
- UART_2_RX : PA3
- USER_PB : PC13

System Clock
------------

Nucleo U083RC System Clock could be driven by internal or external oscillator,
as well as main PLL clock. By default System clock is driven by PLL clock at
48MHz, driven by 4MHz medium speed internal oscillator.

Serial Port
-----------

Nucleo U083RC board has 7 U(S)ARTs. The Zephyr console output is assigned to
USART2. Default settings are 115200 8N1.


Programming and Debugging
*************************

Nucleo U083RC board includes an ST-LINK/V3 embedded debug tool interface.
This probe allows to flash the board using various tools.

Flashing
========

The board is configured to be flashed using west `STM32CubeProgrammer`_ runner,
so its :ref:`installation <stm32cubeprog-flash-host-tools>` is required.

Alternatively, JLink or pyOCD can also be used to flash the board using
the ``--runner`` (or ``-r``) option:

.. code-block:: console

   $ west flash --runner jlink
   $ west flash --runner pyocd

For pyOCD, additional target information needs to be installed.
This can be done by executing the following commands.

.. code-block:: console

   $ pyocd pack --update
   $ pyocd pack --install stm32u0


Flashing an application to Nucleo U083RC
------------------------------------------

Connect the Nucleo U083RC to your host computer using the USB port.
Then build and flash an application. Here is an example for the
:zephyr:code-sample:`hello_world` application.

Run a serial host program to connect with your Nucleo board:

.. code-block:: console

   $ minicom -D /dev/ttyACM0

Then build and flash the application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: nucleo_u083rc
   :goals: build flash

You should see the following message on the console:

.. code-block:: console

   Hello World! nucleo_u083rc/stm32u083xx

Debugging
=========

Default flasher for this board is openocd. It could be used in the usual way.
Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: nucleo_u083rc
   :goals: debug

Note: Check the ``build/tfm`` directory to ensure that the commands required by these scripts
(``readlink``, etc.) are available on your system. Please also check ``STM32_Programmer_CLI``
(which is used for initialization) is available in the PATH.

.. _NUCLEO_U083RC website:
   https://www.st.com/en/evaluation-tools/nucleo-u083rc.html

.. _STM32U083 User Manual:
   https://www.st.com/resource/en/user_manual/um3261-stm32u0-series-safety-manual-stmicroelectronics.pdf

.. _STM32U083RC on www.st.com:
   https://www.st.com/en/microcontrollers-microprocessors/stm32u083rc

.. _STM32U083 reference manual:
   https://www.st.com/resource/en/reference_manual/rm0503-stm32u0-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html
