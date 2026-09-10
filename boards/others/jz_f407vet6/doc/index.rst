.. zephyr:board:: jz_f407vet6

Overview
********

The JZ-F407VET6 is an industrial control board featuring an Arm® Cortex®‑M4
based STM32F407VE MCU with a wide range of connectivity support targeting
industrial automation applications. Here are some highlights of the
JZ-F407VET6 board:

- STM32 microcontroller in LQFP100 package
- 25 MHz high-speed external crystal
- 32.768 kHz RTC crystal
- Flexible board power supply:

  - 5V DC barrel jack
  - USB Mini connector

- Three user LEDs
- Three user push-buttons
- RS232 interface with DB9 connector
- RS485 interface with Modbus support
- Dual CAN bus interfaces with TJA1050 transceivers
- 10/100 Ethernet with RMII PHY
- MicroSD card slot
- W25Q64JV 8MB SPI NOR flash
- AT24C02 256-byte I2C EEPROM
- USB Host (USB-A connector)
- USB Device (USB Mini connector)
- Two 16-pin extension headers
- 8-pin NRF24L01 header
- 3-pin DS18B20 header
- Coin cell battery holder for VBAT
- SWD/JTAG debug interface

Hardware
********

The JZ-F407VET6 board provides the following hardware components:

- STM32F407VET6 in LQFP100 package
- ARM 32-bit Cortex-M4 CPU with FPU
- 168 MHz max CPU frequency
- VDD from 1.8 V to 3.6 V
- 512 KB Flash
- 192 KB SRAM including 64 KB of core coupled memory
- 4 KB backup SRAM
- GPIO with external interrupt capability
- 12-bit ADC with 24 channels
- 12-bit DAC
- RTC
- Advanced-control Timers
- General Purpose Timers
- Watchdog Timers
- USART/UART
- I2C
- SPI
- SDIO
- CAN
- USB 2.0 OTG FS
- USB 2.0 OTG HS/FS
- 10/100 Ethernet MAC
- CRC calculation unit
- True random number generator
- DMA Controller

More information about STM32F407VE can be found here:

- `STM32F407VE on www.st.com`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The JZ-F407VET6 board has 5 GPIO controllers. These controllers are responsible
for pin muxing, input/output, pull-up, etc.

Default Zephyr Peripheral Mapping:
----------------------------------

.. rst-class:: rst-columns

- USART_1 TX/RX : PA9/PA10 (RS232)
- USART_2 TX/RX/DE : PD5/PD6/PD7 (RS485)
- I2C1 SCL/SDA : PB8/PB9
- SPI2 SCK/MISO/MOSI : PB10/PC2/PC3
- SPI2 CS (Flash) : PE3
- CAN1 RX/TX : PD0/PD1
- CAN2 RX/TX : PB5/PB6
- ETH : PA1, PA2, PA7, PB11, PB12, PB13, PC1, PC4, PC5
- SDIO D0-D3/CK/CMD : PC8-PC11/PC12/PD2
- SDIO CD : PD3
- USB OTG FS DM/DP : PA11/PA12
- USB OTG HS DM/DP : PB14/PB15
- LED1 : PE13
- LED2 : PE14
- LED3 : PE15
- SW1 : PE10
- SW2 : PE11
- SW3 : PE12

System Clock
------------

The JZ-F407VET6 system clock could be driven by an internal or external
oscillator, as well as by the main PLL clock. By default, the system clock
is driven by the PLL clock at 168 MHz, driven by a 25 MHz high-speed
external clock.

Serial Port
-----------

The JZ-F407VET6 board has 6 UARTs. The Zephyr console output is assigned
to USART1. Default settings are 115200 8N1.

USART1 is accessible via an RS232 transceiver connected to a DB9 connector.

USART2 is configured for RS485 with Modbus support, using PD7 as the
driver-enable pin.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``jz_f407vet6`` board configuration can be built and
flashed in the usual way (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Flashing
========

The JZ-F407VET6 board can be flashed using an external ST-LINK/V2 or
compatible JTAG programmer connected to the JTAG header.

Alternatively, OpenOCD or JLink can also be used to flash the board using
the ``--runner`` (or ``-r``) option:

.. code-block:: console

   $ west flash --runner openocd
   $ west flash --runner jlink

Flashing an application to JZ-F407VET6
--------------------------------------

Here is an example for the :zephyr:code-sample:`hello_world` application.

Run a serial host program to connect with your board via the DB9 RS232 port:

.. code-block:: console

   $ minicom -D /dev/ttyUSB0

Build and flash the application:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: jz_f407vet6
   :goals: build flash

You should see the following message on the console:

.. code-block:: console

   $ Hello World! jz_f407vet6/stm32f407xx

Debugging
=========

You can debug an application in the usual way. Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: jz_f407vet6
   :maybe-skip-config:
   :goals: debug

.. _STM32F407VE on www.st.com:
   https://www.st.com/en/microcontrollers/stm32f407ve.html
