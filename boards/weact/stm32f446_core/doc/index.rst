.. zephyr:board:: weact_stm32f446_core

Overview
********

The WeAct STM32F446 Core Board is an extremely low cost and bare-bones
development board featuring the STM32F446RE, see `STM32F446RE website`_.
This is the 64-pin variant of the STM32F446x series,
see `STM32F446x reference manual`_. More info about the board available
on `WeAct Github`_.

Hardware
********

The STM32F446RE based Core Board provides the following
hardware components:

- STM32F446RE in QFPN64 package
- ARM |reg| 32-bit Cortex |reg| -M4 CPU with FPU, Adaptive real-time
  accelerator (ART Accelerator) allowing 0-wait state execution from Flash memory
- 180 MHz max CPU frequency
- VDD from 1.7 V to 3.6 V
- 512 KB Flash
- 128 Kbytes of SRAM
- GPIO with external interrupt capability
- 3x12-bit, 2.4 MSPS ADC up to 16 channels
- 2x12-bit D/A converters
- 16-stream DMA controller
- Up to 17 Timers (twelve 16-bit, two 32-bit, two watchdog timers and a SysTick timer)
- USART/UART (4)
- I2C (4)
- SPI/I2S (4)
- CAN (2)
- SDIO
- USB 2.0 full-speed device/host/OTG controller with on-chip PHY
- SAI (Serial Audio Interface)
- SPDIFRX interface
- HDMI-CEC
- CRC calculation unit
- 96-bit unique ID
- RTC with hardware calendar


Supported Features
==================

.. zephyr:board-supported-hw::

Pin Mapping
===========

Default Zephyr Peripheral Mapping:
----------------------------------

- UART_1 TX/RX : PA9/PA10 (also available on USB-C for console)
- UART_2 TX/RX : PA2/PA3
- I2C1 SCL/SDA : PB6/PB7
- I2C2 SCL/SDA : PB10/PB11
- SPI1 SCK/MISO/MOSI : PA5/PA6/PA7
- SPI2 SCK/MISO/MOSI : PB13/PB14/PB15
- CAN1 TX/RX : PB9/PB8
- SDMMC1 D0/D1/D2/D3/CLK/CMD : PC8/PC9/PC10/PC11/PC12/PD2
- USER_PB : PC13 (Active Low)
- USER_LED : PB2 (Active High)
- USB_DM/USB_DP : PA11/PA12

OnBoard Features

- USB Type-C connector
- User LED (PB2) - Active High
- User Key (PC13) - Active Low
- BOOT0 Key for DFU mode
- RESET Key
- 8MHz HSE crystal oscillator
- 32.768kHz LSE crystal oscillator
- MicroSD card slot (SDIO interface)
- SWD debug header (3.3V, GND, SWCLK, SWDIO)
- 30×2 pin headers exposing GPIO pins

Clock Sources
-------------

The board has two external oscillators. The frequency of the slow clock (LSE) is
32.768 kHz. The frequency of the main clock (HSE) is 8 MHz.

The default configuration sources the system clock from the PLL, which is
derived from HSE, and is set at 180MHz, which is the maximum possible frequency
for the STM32F446RE.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

There are 2 main entry points for flashing STM32F4X SoCs, one using the ROM
bootloader, and another by using the SWD debug port (which requires additional
hardware). Flashing using the ROM bootloader requires a special activation
pattern, which can be triggered by using the BOOT0 pin.

Flashing
========

Installing dfu-util
-------------------

It is recommended to use at least v0.8 of `dfu-util`_. The package available in
debian/ubuntu can be quite old, so you might have to build dfu-util from source.

There is also a Windows version which works, but you may have to install the
right USB drivers with a tool like `Zadig`_.

Flashing an Application
-----------------------

Connect a USB-C cable and the board should power ON. Force the board into DFU mode
by keeping the BOOT0 switch pressed while pressing and releasing the NRST switch.

The dfu-util runner is supported on this board and so a sample can be built and
tested easily.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: weact_stm32f446_core
   :goals: build flash

.. zephyr-app-commands::
   :zephyr-app: samples/basic/button
   :board: weact_stm32f446_core
   :goals: build flash

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/fs/fs_sample
   :board: weact_stm32f446_core
   :goals: build flash


Debugging
=========

The board can be debugged by installing the included 100 mil (0.1 inch) header,
and attaching an SWD debugger to the 3V3 (3.3V), GND, SCK, and DIO
pins on that header.

References
**********

.. target-notes::

.. _board release notes:
   https://github.com/WeActStudio/WeActStudio.STM32F4_64Pin_CoreBoard/blob/master/README.md

.. _Zadig:
   https://zadig.akeo.ie/

.. _WeAct Github:
   https://github.com/WeActStudio/WeActStudio.STM32F4_64Pin_CoreBoard

.. _dfu-util:
   http://dfu-util.sourceforge.net/build.html

.. _STM32F446RE website:
   https://www.st.com/en/microcontrollers-microprocessors/stm32f446re.html

.. _STM32F446x reference manual:
   https://www.st.com/resource/en/reference_manual/rm0390-stm32f446xx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
