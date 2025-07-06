.. zephyr:board:: pandora_stm32l475

Overview
********

The STM32L475 Pandora board features an ARM Cortex-M4 based STM32L475VE MCU
with a wide range of connectivity support and configurations. Here are
some highlights of the STM32L475 Pandora board:


- STM32L475Vx microcontroller featuring 1 Mbyte of Flash memory, 128 Kbytes of RAM in LQFP100 package
- On-board ST-LINK/V2-1 supporting USB re-enumeration capability
- Three different interfaces supported on USB:

    - Virtual com port
    - Mass storage
    - Debug port

- Pushbutton (reset)
- Four directions Joystick with selection
- USB OTG FS with micro-AB connector
- SAI Audio DAC, Stereo with output jack
- Digital microphone, accelerometer, magnetometer and gyroscope MEMS
- 128-Mbit Quad-SPI Flash memory
- MCU current ammeter with 4 ranges and auto-calibration
- Connector for external board or RF-EEPROM
- Four power supply options:

    - ST-LINK/V2-1
    - USB FS connector
    - External 5 V
    - CR2032 battery (not provided)

More information about the board can be found at the `STM32L475 Pandora website`_.

Hardware
********

The STM32L475VE SoC provides the following hardware features:

- Ultra-low-power with FlexPowerControl (down to 130 nA Standby mode and 100 uA/MHz run mode)
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
- 16x timers:
    - 2x 16-bit advanced motor-control
    - 2x 32-bit and 7x 16-bit general purpose
    - 2x 16-bit basic
    - 2x low-power 16-bit timers (available in Stop mode)
    - 2x watchdogs
    - SysTick timer
- Up to 82 fast I/Os, most 5 V-tolerant, up to 14 I/Os with independent supply down to 1.08 V
- Memories
    - Up to 1 MB Flash, 2 banks read-while-write, proprietary code readout protection
    - Up to 128 KB of SRAM including 32 KB with hardware parity check
    - External memory interface for static memories supporting SRAM, PSRAM, NOR and NAND memories
    - Quad SPI memory interface
- 4x digital filters for sigma delta modulator
- Rich analog peripherals (independent supply)
    - 3x 12-bit ADC 5 MSPS, up to 16-bit with hardware oversampling, 200 uA/MSPS
    - 2x 12-bit DAC, low-power sample and hold
    - 2x operational amplifiers with built-in PGA
    - 2x ultra-low-power comparators
- 18x communication interfaces
    - USB OTG 2.0 full-speed, LPM and BCD
    - 2x SAIs (serial audio interface)
    - 3x I2C FM+(1 Mbit/s), SMBus/PMBus
    - 6x USARTs (ISO 7816, LIN, IrDA, modem)
    - 3x SPIs (4x SPIs with the Quad SPI)
    - CAN (2.0B Active) and SDMMC interface
    - SWPMI single wire protocol master I/F
- 14-channel DMA controller
- True random number generator
- CRC calculation unit, 96-bit unique ID
- Development support: serial wire debug (SWD), JTAG, Embedded Trace Macrocell |trade|


Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

STM32L475 Pandora Board has 8 GPIO controllers. These controllers are responsible for pin muxing,
input/output, pull-up, etc.

For more details please refer to `STM32L475 Pandora board User Manual`_.

Default Zephyr Peripheral Mapping:
----------------------------------

- UART_1_TX : PA9
- UART_1_RX : PA10
- LED_R : PE7
- LED_G : PE8
- LED_B : PE9

System Clock
------------

STM32L475 Pandora System Clock could be driven by an internal or external oscillator,
as well as the main PLL clock. By default the System clock is driven by the PLL clock at 80MHz,
driven by 16MHz high speed internal oscillator.

Serial Port
-----------

STM32L475 Pandora board has 6 U(S)ARTs. The Zephyr console output is assigned to UART2.
Default settings are 115200 8N1.


Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Flashing
========

STM32L475 Pandora board includes an ST-LINK/V2-1 embedded debug tool interface.
This interface is supported by the openocd version included in Zephyr SDK.

Flashing an application to STM32L475 Pandora
--------------------------------------------

Connect the STM32L475 Pandora to your host computer using the USB
port, then run a serial host program to connect with your Discovery
board. For example:

.. code-block:: console

   $ minicom -D /dev/ttyACM0

Then, build and flash in the usual way. Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: stm32l475ve_pandora
   :goals: build flash

You should see the following message on the console:

.. code-block:: console

   Hello World! arm

Debugging
=========

You can debug an application in the usual way.  Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: stm32l475ve_pandora
   :maybe-skip-config:
   :goals: debug

.. _STM32L475 Pandora website:
   http://www.openedv.com/docs/boards/iot/zdyz_panduola.html

.. _STM32L475 Pandora board User Manual:
   http://www.openedv.com/thread-284556-1-1.html
