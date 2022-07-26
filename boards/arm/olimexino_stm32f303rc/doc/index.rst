.. _olimexino_stm32f303rc_board:

Olimexino STM32F303RC
################

Overview
********

The Olimexino STM32F303RC board features an ARM Cortex-M4 based STM32F303RC
mixed-signal MCU with FPU and DSP instructions capable of running at 72 MHz.
Here are some highlights of the Olimexino STM32F303RC board:

- STM32 microcontroller in LQFP64 package
- LSE crystal: 32.768 kHz crystal oscillator
- Flexible board power supply:

  - 5 V from USB VBUS
  - External power sources: 9 - 30 V

- Two user LED
- Two push-buttons: BOOT/USER and RESET
- SIT1050T CAN transceiver
- UEXT connector
- SWD (2x5 1.27mm) connector
- Arduino* Uno V3 connectors

More information about the board can be found at the `Olimexino STM32F303RC website`_.
Hardware
********

The Olimexino STM32F303RC provides the following hardware components:

- STM32F303RCT6 in QFP64 package
- ARM |reg| 32-bit Cortex |reg| -M4 CPU with FPU
- 72 MHz max CPU frequency
- VDD from 2.0 V to 3.6 V
- 256 KB Flash
- 40 KB SRAM
- Routine booster: 8 Kbytes of SRAM on instruction and data bus
- GPIO with external interrupt capability
- 4x12-bit ADC with 39 channels
- 2x12-bit D/A converters
- RTC
- General Purpose Timers (13)
- USART/UART (5)
- I2C (2)
- SPI (3)
- CAN
- USB 2.0 full speed interface
- Infrared transmitter
- DMA Controller

More information about the STM32F303RC can be found here:

- `STM32F303RC on www.st.com`_
- `STM32F303RC reference manual`_
- `STM32F303RC datasheet`_

Supported Features
==================

The default configuration can be found in the defconfig file:
``boards/arm/olimexino_stm32f303rc/olimexino_stm32f303rc_defconfig``

Connections and IOs
===================

The Olimexino STM32F303RC Board has 6 GPIO controllers. These controllers are
responsible for pin muxing, input/output, pull-up, etc.

Default Zephyr Peripheral Mapping:
----------------------------------

- UART_1_TX : PC4
- UART_1_RX : PC5
- UART_2_TX : PA2
- UART_2_RX : PA3
- UART_3_TX : PB10
- UART_3_RX : PB11
- I2C1_SCL : PB6
- I2C1_SDA : PB7
- I2C2_SCL : PF1
- I2C2_SDA : PF0
- SPI1_NSS : PA4
- SPI1_SCK : PA5
- SPI1_MISO : PA6
- SPI1_MOSI : PA7
- SPI2_NSS : PB12
- SPI2_SCK : PB13
- SPI2_MISO : PB14
- SPI2_MOSI : PB15
- CAN1_RX : PB8
- CAN1_TX : PB9
- USB_DM : PA11
- USB_DP : PA12
- USB_DP_PULLUP: PC12
- LD1 : PA1
- LD2 : PA5
- PWM : PA8

System Clock
------------

The STM32F303RC System Clock can be driven by an internal or
external oscillator, as well as by the main PLL clock. By default the
System Clock is driven by the PLL clock at 72 MHz. The input to the
PLL is an 8 MHz external clock supplied by the processor of the
on-board ST-LINK/V2-1 debugger/programmer.

Serial Port
-----------

The Olimexino STM32F303RC board has 3 UARTs. The Zephyr console output is assigned
to UART2. Default settings are 115200 8N1.

.. _Olimexino STM32F303RC website:
   https://www.olimex.com/Products/Duino/STM32/OLIMEXINO-STM32F3/open-source-hardware

.. _STM32F303RC on www.st.com:
   http://www.st.com/en/microcontrollers/stm32f303rc.html

.. _STM32F303RC reference manual:
   https://www.st.com/resource/en/reference_manual/dm00043574.pdf

.. _STM32F303RC datasheet:
   http://www.st.com/resource/en/datasheet/stm32f303rc.pdf
