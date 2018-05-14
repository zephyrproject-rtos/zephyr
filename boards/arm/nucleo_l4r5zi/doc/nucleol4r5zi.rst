.. _nucleo_l45rzi_board:

ST Nucleo L4R5ZI
################

Overview
********

The Nucleo L4R5ZI board features an ARM Cortex-M4 based STM32L4R5ZI MCU
with a wide range of connectivity support and configurations. Here are
some highlights of the Nucleo L4R5ZI board:


- STM32 microcontroller in LQFP144 package
- Two types of extension resources:
       - Arduino Uno V3 connectivity
       - ST morpho extension pin headers for full access to all STM32 I/Os
- On-board ST-LINK/V2-1 debugger/programmer with SWD connector
- Flexible board power supply:
       - USB VBUS or external source(3.3V, 5V, 7 - 12V)
       - Power management access point
- Three LEDs: USB communication (LD1), user LED (LD2), power LED (LD3)
- Two push-buttons: USER and RESET

More information about the board can be found at the `Nucleo L4R5ZI website`_.

Hardware
********

The STM32L4R5ZI SoC provides the following hardware IPs:

- Ultra-low-power with FlexPowerControl (down to 125 nA Standby mode and 110 uA/MHz run mode)
- Core: ARM |reg| 32-bit Cortex |reg|-M4 CPU with FPU, frequency up to 120 MHz
- Clock Sources:
        - 4 to 48 MHz crystal oscillator
        - 32 kHz crystal oscillator for RTC (LSE)
        - Internal 16 MHz factory-trimmed RC ( |plusminus| 1%)
        - Internal low-power 32 kHz RC ( |plusminus| 5%)
        - Internal multispeed 100 kHz to 48 MHz oscillator, auto-trimmed by
          LSE (better than  |plusminus| 0.25 % accuracy)
        - 3 PLLs for system clock, USB, audio, ADC
- RTC with HW calendar, alarms and calibration
- 16x timers:
        - 2x 16-bit advanced motor-control
        - 2x 32-bit and 5x 16-bit general purpose
        - 2x 16-bit basic
        - 2x low-power 16-bit timers (available in Stop mode)
        - 2x watchdogs
        - SysTick timer
- Up to 136 fast I/Os, most 5 V-tolerant, up to 14 I/Os with independent supply down to 1.08 V
- Memories
        - Up to 2 MB Flash, 2 banks read-while-write, proprietary code readout protection
        - 640 Kbytes of SRAM including 64 Kbytes with hardware parity check
        - External memory interface for static memories supporting SRAM, PSRAM, NOR and NAND memories
        - 2 x OctoSPI memory interface
- 4x digital filters for sigma delta modulator
- Rich analog peripherals (independent supply)
        - 12-bit ADC 5 Msps, up to 16-bit with hardware oversampling, 200 μA/Msps
        - 2x 12-bit DAC, low-power sample and hold
        - 2x operational amplifiers with built-in PGA
        - 2x ultra-low-power comparators
- 20x communication interfaces
        - USB OTG 2.0 full-speed, LPM and BCD
        - 2x SAIs (serial audio interface)
        - 4x I2C FM+(1 Mbit/s), SMBus/PMBus
        - 6x USARTs (ISO 7816, LIN, IrDA, modem)
        - 3x SPIs (5x SPIs with the dual OctoSPI)
        - CAN (2.0B Active) and SDMMC

- 14-channel DMA controller
- True random number generator
- CRC calculation unit, 96-bit unique ID
- 8- to 14-bit camera interface up to 32 MHz (black and white) or 10 MHz (color)
- Development support: serial wire debug (SWD), JTAG, Embedded Trace Macrocell |trade|


More information about L4R5ZI can be found here:
       - `STM32L4R5ZI on www.st.com`_
       - `STM32L4Rxxx and STM32L4Sxxx reference manual`_

Supported Features
==================

The Zephyr nucleo_l4r5zi board configuration supports the following hardware features:

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
| SPI       | on-chip    | spi                                 |
+-----------+------------+-------------------------------------+

Other hardware features are not yet supported on this Zephyr port.

The default configuration can be found in the defconfig file:

	``boards/arm/nucleo_l4r5zi/nucleo_l4r5zi_defconfig``


Connections and IOs
===================

Nucleo L4R5ZI Board has 8 GPIO controllers. These controllers are responsible for pin muxing,
input/output, pull-up, etc.

For details please refer to `STM32 Nucleo-144 (L4R5ZI) board User Manual`_.

Default Zephyr Peripheral Mapping:
----------------------------------

- UART_1_TX : PA9
- UART_1_RX : PA10
- UART_2_TX : PA2
- UART_2_RX : PA3
- UART_3_TX : PB10
- UART_3_RX : PB11
- UART_11_TX : PG7
- UART_11_RX : PG8
- I2C_1_SCL : PB6
- I2C_1_SDA : PB7
- SPI_1_NSS : PA4
- SPI_1_SCK : PB3
- SPI_1_MISO : PA6
- SPI_1_MOSI : PA7
- SPI_2_NSS : PB12
- SPI_2_SCK : PB13
- SPI_2_MISO : PB14
- SPI_2_MOSI : PB15
- SPI_3_NSS : PB12
- SPI_3_SCK : PC10
- SPI_3_MISO : PC11
- SPI_3_MOSI : PC12
- PWM_2_CH1 : PA0
- USER_PB : PC13
- LD1 : PB7
- LD2 : PC7

System Clock
------------

Nucleo L4R5ZI System Clock could be driven by internal or external oscillator,
as well as main PLL clock. By default System clock is driven by PLL clock at 32Mhz,
driven by 16MHz high speed internal oscillator.

Serial Port
-----------

The Zephyr console output is assigned to UART11. Default settings are 115200 8N1.



.. _Nucleo L4R5ZI website:
   http://www.st.com/en/evaluation-tools/nucleo-l4r5zi.html

.. _STM32 Nucleo-144 (L4R5ZI) board User Manual:
   http://www.st.com/resource/en/user_manual/dm00368330.pdf

.. _STM32L4R5ZI on www.st.com:
   http://www.st.com/en/microcontrollers/stm32l4r5zi.html

.. _STM32L4Rxxx and STM32L4Sxxx reference manual:
   http://www.st.com/resource/en/reference_manual/dm00310109.pdf
