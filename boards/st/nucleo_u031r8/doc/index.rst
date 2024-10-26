.. zephyr:board:: nucleo_u031r8

Overview
********

The Nucleo U031R8 board, featuring an ARM Cortex-M0+ based STM32U031R8 MCU,
provides an affordable and flexible way for users to try out new concepts and
build prototypes by choosing from the various combinations of performance and
power consumption features. Here are some highlights of the Nucleo U031R8
board:


- STM32U031R8 microcontroller in LQFP48 package
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

More information about the board can be found at the `NUCLEO_U031R8 website`_.

Hardware
********

The STM32U031x4/6/8 devices are an ultra-low-power microcontrollers family (STM32U0
Series) based on the high-performance Arm |reg| Cortex |reg|-M0+ 32-bit RISC core.
They operate at a frequency of up to 56 MHz.

- Includes ST state-of-the-art patented technology
- Ultra-low-power with FlexPowerControl:

  - 1.71 V to 3.6 V power supply
  - -40 °C to +85/125 °C temperature range
  - 130 nA VBAT mode: supply for RTC, 9 x 32-bit backup registers
  - 16 nA Shutdown mode (4 wake-up pins)
  - 30 nA Standby mode (6 wake-up pins) without RTC
  - 160 nA Standby mode with RTC
  - 630 nA Stop 2 mode with RTC
  - 515 nA Stop 2 mode without RTC
  - 4 µA wake-up from Stop mode
  - 52 µA/MHz Run mode
  - Brownout reset

- Core:

  - 32-bit Arm |reg| Cortex |reg|-M0+ CPU, frequency up to 56 MHz

- ART Accelerator:

  - 1-Kbyte instruction cache allowing 0-wait-state execution from flash memory

- Benchmarks:

  - 1.13 DMIPS/MHz (Drystone 2.1)
  - 134 CoreMark |reg| (2.4 CoreMark/MHz at 56 MHz)
  - 430 ULPMark™-CP
  - 167 ULPMark™-PP
  - 20.3 ULPMark™-CM

- Memories:

  - 64-Kbyte single bank flash memory, proprietary code readout protection
  - 12-Kbyte SRAM with hardware parity check

- General-purpose input/outputs:

  - Up to 53 fast I/Os, most of them 5 V‑tolerant

- Clock management:

  - 4 to 48 MHz crystal oscillator
  - 32 kHz crystal oscillator for RTC (LSE)
  - Internal 16 MHz factory-trimmed RC (±1%)
  - Internal low-power 32 kHz RC (±5%)
  - Internal multispeed 100 kHz to 48 MHz oscillator,
    auto-trimmed by LSE (better than ±0.25 % accuracy)
  - PLL for system clock, ADC

- Security:

  - Customer code protection
  - Robust read out protection (RDP): 3 protection level states
    and password-based regression (128-bit PSWD)
  - Hardware protection feature (HDP)
  - Secure boot
  - True random number generation, candidate for NIST SP 800-90B certification
  - Candidate for Arm |reg| PSA level 1 and SESIP level 3 certifications
  - 5 passive anti-tamper pins
  - 96-bit unique ID

- Up to 9 timers, RTC, and 2 watchdogs :

  - 1x 16-bit advanced motor-control, 1x 32-bit and 3x 16-bit general purpose,
    2x 16-bit basic, 2x low-power 16-bit timers (available in Stop mode),
    2x watchdogs, SysTick timer
  - RTC with hardware calendar, alarms and calibration

- Up to 16 communication peripherals:

  - 6x USARTs/LPUARTs (SPI, ISO 7816, LIN, IrDA, modem)
  - 3x I2C interfaces supporting Fast-mode and Fast-mode Plus (up to 1 Mbit/s)
  - 2x SPIs, plus 4x USARTs in SPI mode
  - IRTIM (Infrared interface)

- Rich analog peripherals (independent supply):

  - 1x 12-bit ADC (0.4 µs conversion time), up to 16-bit with hardware oversampling
  - 1x 12-bit DAC output channel, low-power sample and hold
  - 1x general-purpose operational amplifier with built-in PGA (variable gain up to 16)
  - 1x ultra-low-power comparator

- ECOPACK2 compliant packages

More information about STM32U031R8 can be found here:

- `STM32U031R8 on www.st.com`_
- `STM32U031R8 reference manual`_

Supported Features
==================

The Zephyr _nucleo_u031r8_ board configuration supports the following hardware features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| CLOCK     | on-chip    | reset and clock control             |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | gpio                                |
+-----------+------------+-------------------------------------+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| ADC       | on-chip    | adc                                 |
+-----------+------------+-------------------------------------+
| DAC       | on-chip    | DAC Controller                      |
+-----------+------------+-------------------------------------+
| I2C       | on-chip    | i2c                                 |
+-----------+------------+-------------------------------------+
| PWM       | on-chip    | pwm                                 |
+-----------+------------+-------------------------------------+


Other hardware features are not yet supported on this Zephyr port.

The default configuration can be found in the defconfig file:
:zephyr_file:`boards/st/nucleo_u031r8/nucleo_u031r8_defconfig`


Connections and IOs
===================

Nucleo U031R8 Board has 10 GPIO controllers. These controllers are responsible
for pin muxing, input/output, pull-up, etc.

For more details please refer to `STM32U031 User Manual`_.

Default Zephyr Peripheral Mapping:
----------------------------------

- DAC1_OUT1 : PA4
- LD1 : PA5
- UART_1_TX : PA9
- UART_1_RX : PA10
- UART_2_TX : PA2
- UART_2_RX : PA3
- USER_PB : PC13

System Clock
------------

Nucleo U031R8 System Clock could be driven by internal or external oscillator,
as well as main PLL clock. By default System clock is driven by PLL clock at
48MHz, driven by 4MHz medium speed internal oscillator.

Serial Port
-----------

Nucleo U031R8 board has 4 U(S)ARTs. The Zephyr console output is assigned to
USART2. Default settings are 115200 8N1.


Programming and Debugging
*************************

Nucleo U031R8 board includes an ST-LINK/V3 embedded debug tool interface.
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
   $ pyocd pack --install stm32u0


Flashing an application to Nucleo U031R8
------------------------------------------

Connect the Nucleo U031R8 to your host computer using the USB port.
Then build and flash an application. Here is an example for the
:zephyr:code-sample:`hello_world` application.

Run a serial host program to connect with your Nucleo board:

.. code-block:: console

   $ minicom -D /dev/ttyACM0

Then build and flash the application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: nucleo_u031r8
   :goals: build flash

You should see the following message on the console:

.. code-block:: console

   Hello World! nucleo_u031r8

Debugging
=========

Default flasher for this board is openocd. It could be used in the usual way.
Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: nucleo_u031r8
   :goals: debug

Note: Check the ``build/tfm`` directory to ensure that the commands required by these scripts
(``readlink``, etc.) are available on your system. Please also check ``STM32_Programmer_CLI``
(which is used for initialization) is available in the PATH.

.. _NUCLEO_U031R8 website:
  https://www.st.com/en/evaluation-tools/nucleo-u031r8.html

.. _STM32U031 User Manual:
   https://www.st.com/resource/en/user_manual/um3261-stm32u0-series-safety-manual-stmicroelectronics.pdf

.. _STM32U031R8 on www.st.com:
   https://www.st.com/en/microcontrollers-microprocessors/stm32u031r8

.. _STM32U031R8 reference manual:
   https://www.st.com/resource/en/reference_manual/rm0503-stm32u0-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html
