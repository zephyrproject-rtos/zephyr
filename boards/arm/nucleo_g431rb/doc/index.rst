.. _nucleo_g431rb_board:

ST Nucleo G431RB
################

Overview
********

The Nucleo G431RB board features an ARM Cortex-M4 based STM32G431RB MCU
with a wide range of connectivity support and configurations. Here are
some highlights of the Nucleo G431RB board:

- STM32 microcontroller in LQFP64 package
- Arduino Uno V3 connectivity
- On-board ST-LINK/V3E debugger/programmer with SWD connector
- Flexible board power supply:

  - USB VBUS or external source(3.3V, 5V, 7 - 12V)
  - Power management access point

- Three LEDs: USB communication (LD1), power LED (LD3), user LED (LD2)
- Two push-buttons: RESET and USER

.. image:: img/nucleo_g431rb.jpg
  :width: 426px
  :align: center
  :height: 256px
  :alt: Nucleo G431RB

More information about the board can be found at the `Nucleo G431RB website`_.

Hardware
********

The STM32G431RB SoC provides the following hardware IPs:

- Ultra-low-power with FlexPowerControl (down to 28 nA Standby mode and 84
  |micro| A/MHz run mode)
- Core: ARM |reg| 32-bit Cortex |reg| -M4 CPU with FPU, frequency up to 170 MHz
- Clock Sources:

  - 4 to 48 MHz crystal oscillator (HSE)
  - 32 kHz crystal oscillator for RTC (LSE)
  - Internal 16 MHz factory-trimmed RC ( |plusminus| 1%)
  - Internal low-power 32 kHz RC ( |plusminus| 5%)
  - 2 PLLs for system clock, USB, audio, ADC

- RTC with HW calendar, alarms and calibration
- 14x timers:

  - 1x 32-bit timer and 2x 16-bit timers with up to four IC/OC/PWM or pulse counter and quadrature (incremental) encoder input
  - 2x 16-bit 8-channel advanced motor control timers, with up to 8x PWM channels, dead time generation and emergency stop
  - 1x 16-bit timer with 2x IC/OCs, one OCN/PWM, dead time generation and emergency stop
  - 2x 16-bit timers with IC/OC/OCN/PWM, dead time generation and emergency stop
  - 2x watchdog timers (independent, window)
  - 2x 16-bit basic timers
  - SysTick timer
  - 1x low-power timer

- Up to 86 fast I/Os, most 5 V-tolerant
- Memories

  - Up to 128 KB single bank Flash, proprietary code readout protection
  - Up to 22 KB of SRAM including 16 KB with hardware parity check

- Rich analog peripherals (independent supply)

  - 2x 12-bit ADC 5 MSPS, up to 16-bit with hardware oversampling, 200
    |micro| A/MSPS
  - 4x 12-bit DAC, low-power sample and hold
  - 3x operational amplifiers with built-in PGA
  - 4x ultra-fast rail-to-rail analog comparators

- 16x communication interfaces

  - 1 x FDCAN controller supporting flexible data rate
  - 3x I2C FM+(1 Mbit/s), SMBus/PMBus
  - 4x USARTs (ISO 7816, LIN, IrDA, modem)
  - 1x LPUART
  - 3x SPIs (2x with multiplexed half duplex I2S interface)
  - 1x SAI (serial audio interface)
  - USB 2.0 full-speed interface with LPM and BCD support
  - IRTIM (Infrared interface)
  - USB Type-C™ /USB power delivery controller (UCPD)

- 12-channel DMA controller
- True random number generator (RNG)
- CRC calculation unit, 96-bit unique ID
- Development support: serial wire debug (SWD), JTAG, Embedded Trace Macrocell*


More information about STM32G431RB can be found here:

- `STM32G431RB on www.st.com`_
- `STM32G4 reference manual`_

Supported Features
==================

The Zephyr nucleo_g431rb board configuration supports the following hardware features:

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
| PWM       | on-chip    | pwm                                 |
+-----------+------------+-------------------------------------+

Other hardware features are not yet supported on this Zephyr port.

The default configuration can be found in the defconfig file:
``boards/arm/nucleo_g431rb/nucleo_g431rb_defconfig``


Connections and IOs
===================

Nucleo G431RB Board has 6 GPIO controllers. These controllers are responsible for pin muxing,
input/output, pull-up, etc.

For mode details please refer to `STM32G4 Nucleo-64 board User Manual`_.

Default Zephyr Peripheral Mapping:
----------------------------------

.. rst-class:: rst-columns

- UART_1_TX : PC4
- UART_1_RX : PC5
- UART_2_TX : PA2
- UART_2_RX : PA15
- I2C_1_SCL : PB8
- I2C_1_SDA : PB9
- SPI_1_NSS : PB6
- SPI_1_SCK : PB3
- SPI_1_MISO : PA6
- SPI_1_MOSI : PA7
- SPI_2_NSS : PB12
- SPI_2_SCK : PB13
- SPI_2_MISO : PB14
- SPI_2_MOSI : PB15
- SPI_3_NSS : PA15
- SPI_3_SCK : PC10
- SPI_3_MISO : PC11
- SPI_3_MOSI : PC12
- PWM_3_CH1 : PB4
- USER_PB : PC13
- LD2 : PA5
- USB DM : PA11
- USB DP : PA12

System Clock
------------

Nucleo G431RB System Clock could be driven by internal or external oscillator,
as well as main PLL clock. By default System clock is driven by PLL clock at 150MHz,
driven by 16MHz high speed internal oscillator. The clock can be boosted to 170MHz if boost mode
is selected.

Serial Port
-----------

Nucleo G431RB board has 3 U(S)ARTs. The Zephyr console output is assigned to UART2.
Default settings are 115200 8N1.


Programming and Debugging
*************************

Applications for the ``nucleo_g431rb`` board configuration can be built and
flashed in the usual way (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Flashing
========

Nucleo G431RB board includes an ST-LINK/V3E embedded debug tool
interface. This interface is not yet supported by the openocd version.
Instead, support can be enabled on pyocd by adding "pack" support with
the following pyocd command:

.. code-block:: console

   $ pyocd pack --install stm32g431rb

Flashing an application to Nucleo G431RB
----------------------------------------

Connect the Nucleo G431RB to your host computer using the USB port,
then run a serial host program to connect with your Nucleo board.

.. code-block:: console

   $ minicom -D /dev/ttyACM0

Now build and flash an application. Here is an example for
:ref:`hello_world`.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: nucleo_g431rb
   :goals: build flash

You should see the following message on the console:

.. code-block:: console

   $ Hello World! arm


Debugging
=========

You can debug an application in the usual way.  Here is an example for the
:ref:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: nucleo_g431rb
   :maybe-skip-config:
   :goals: debug

.. _Nucleo G431RB website:
   http://www.st.com/en/evaluation-tools/nucleo-g431rb.html

.. _STM32G4 Nucleo-64 board User Manual:
   https://www.st.com/resource/en/user_manual/dm00556337.pdf

.. _STM32G431RB on www.st.com:
   http://www.st.com/en/microcontrollers/stm32g431rb.html

.. _STM32G4 reference manual:
   https://www.st.com/resource/en/reference_manual/dm00355726.pdf
