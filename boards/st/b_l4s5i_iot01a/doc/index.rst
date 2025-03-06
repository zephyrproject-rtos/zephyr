.. zephyr:board:: b_l4s5i_iot01a

Overview
********

The B_L4S5I_IOT01A Discovery kit features an ARM Cortex-M4 based STM32L4S5VI MCU
with a wide range of connectivity support and configurations. Here are
some highlights of the B_L4S5I_IOT01A Discovery kit:


- STM32L4S5VIT6 microcontroller featuring 2 Mbyte of Flash memory, 640 Kbytes of RAM in LQFP100 package
- On-board ST-LINK/V2-1 supporting USB re-enumeration capability
- Three different interfaces supported on USB:

    - Virtual com port
    - Mass storage
    - Debug port

- ARDUINO ® Uno V3  and Pmod TM expansion connector
- 4 LEDs (2 for user, wifi, BLE)
- 2 push-buttons (user and reset)
- USB OTG FS with micro-AB connector
- Dynamic NFC tag
- 2 digital omnidirectional microphones
- Capacitive digital sensor for relative humidity and temperature
- Time-of-flight and gesture-detection sensors
- High-performance 3-axis magnetometer
- 3D accelerometer and 3D gyroscope
- 64-Mbit Quad-SPI Flash memory
- Bluetooth ® 4.1 module
- 802.11 b/g/n compliant Wi‐Fi ® module
- MCU current ammeter with 4 ranges and auto-calibration

- Flexible power supply options:
    - ST-LINK/V2-1
    - USB FS connector
    - External 5 V


More information about the board can be found at the `B L4S5I IOT01A Discovery kit website`_.

Hardware
********

The STM32L4S5VI SoC provides the following hardware features:

- Ultra-low-power with FlexPowerControl (down to 130 nA Standby mode and 100 uA/MHz run mode)
- Core: ARM |reg| 32-bit Cortex |reg|-M4 CPU with FPU, frequency up to 120 MHz, 100DMIPS/1.25DMIPS/MHz (Dhrystone 2.1)
- Clock Sources:
    - 4 to 48 MHz crystal oscillator
    - 32 kHz crystal oscillator for RTC (LSE)
    - Internal 16 MHz factory-trimmed RC ( |plusminus| 1%)
    - Internal low-power 32 kHz RC ( |plusminus| 5%)
    - Internal multispeed 100 kHz to 48 MHz oscillator, auto-trimmed by
      LSE (better than  |plusminus| 0.25 % accuracy)
    - 3 PLLs for system clock, USB, audio, ADC
- RTC with HW calendar, alarms and calibration
- Up to 21 capacitive sensing channels: support touchkey, linear and rotary touch sensors
- 16x timers:
    - 2x 16-bit advanced control
    - 2x 32-bit and 5x 16-bit general purpose
    - 2x 16-bit basic
    - 2x low-power 16-bit timers (available in Stop mode)
    - 2x watchdogs
    - SysTick timer
- Up to 83 fast I/Os, most 5 V-tolerant
- Memories
    - Up to 2 MB Flash, 2 banks read-while-write, proprietary code readout protection
    - Up to 640 KB of SRAM including 32 KB with hardware parity check
    - External memory interface for static memories supporting SRAM, PSRAM, NOR and NAND memories
    - Octo SPI memory interface
- 4x digital filters for sigma delta modulator
- Rich analog peripherals (independent supply)
    - 1x 12-bit ADC 5 MSPS, up to 16-bit with hardware oversampling, 200 uA/MSPS
    - 2x 12-bit DAC, low-power sample and hold
    - 2x operational amplifiers with built-in PGA
    - 2x ultra-low-power comparators
- 18x communication interfaces
    - USB OTG 2.0 full-speed, LPM and BCD
    - 2x SAIs (serial audio interface)
    - 4x I2C FM+(1 Mbit/s), SMBus/PMBus
    - 6x USARTs (ISO 7816, LIN, IrDA, modem)
    - 3x SPIs (4x SPIs with the Quad SPI)
    - CAN (2.0B Active) and SDMMC interface
    - SDMMC I/F
    - DCMI camera interface
- 14-channel DMA controller with multiplex request router
- True random number generator
- CRC calculation unit, 96-bit unique ID
- AES and HASH hardware accelerators
- Development support: serial wire debug (SWD), JTAG, Embedded Trace Macrocell |trade|


More information about STM32L4S5VI can be found here:
       - `STM32L4S5VI on www.st.com`_
       - `STM32L4S5 reference manual`_


Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

B_L4S5I_IOT01A Discovery kit has 9 GPIO controllers (from A to I). These controllers are responsible for pin muxing,
input/output, pull-up, etc.

For more details please refer to `B L47S5I IOT01A board User Manual`_.

Default Zephyr Peripheral Mapping:
----------------------------------

- UART_1 TX/RX : PB6/PB7 (ST-Link Virtual Port Com)
- UART_4 TX/RX : PA0/PA1 (Arduino Serial)
- I2C1 SCL/SDA : PB8/PB9 (Arduino I2C)
- I2C2 SCL/SDA : PB10/PB11 (Sensor I2C bus)
- SPI1 NSS/SCK/MISO/MOSI : PA2/PA5/PA6/PA7 (Arduino SPI)
- SPI3 SCK/MISO/MOSI : PC10/PC11/PC12 (BT SPI bus)
- PWM_2_CH1 : PA15
- LD1 : PA5
- LD2 : PB14
- user button : PC13

System Clock
------------

B_L4S5I_IOT01A Discovery System Clock could be driven by an internal or external oscillator,
as well as the main PLL clock. By default the System clock is driven by the PLL clock at 80MHz,
driven by 16MHz high speed internal oscillator.

Serial Port
-----------

B_L4S5I_IOT01A Discovery kit has 4 U(S)ARTs. The Zephyr console output is assigned to UART1.
Default settings are 115200 8N1.


Programming and Debugging
*************************

B_L4S5I_IOT01A Discovery kit includes an ST-LINK/V2-1 embedded debug tool interface.

Flashing
========

The board is configured to be flashed using west `STM32CubeProgrammer`_ runner,
so its :ref:`installation <stm32cubeprog-flash-host-tools>` is required.

Alternatively, OpenOCD or JLink can also be used to flash the board using
the ``--runner`` (or ``-r``) option:

.. code-block:: console

   $ west flash --runner openocd
   $ west flash --runner jlink

Flashing an application to B_L4S5I_IOT01A Discovery kit
-------------------------------------------------------

Connect the B_L4S5I_IOT01A Discovery kit to your host computer using the USB
port, then run a serial host program to connect with your Discovery
board. For example:

.. code-block:: console

   $ minicom -D /dev/ttyACM0

Then, build and flash in the usual way. Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: b_l4s5i_iot01a
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
   :board: b_l4s5i_iot01a
   :maybe-skip-config:
   :goals: debug

.. _B L4S5I IOT01A Discovery kit website:
   https://www.st.com/en/evaluation-tools/b-l4s5i-iot01a.html

.. _B L47S5I IOT01A board User Manual:
   https://www.st.com/resource/en/user_manual/dm00698410.pdf

.. _STM32L4S5VI on www.st.com:
   https://www.st.com/en/microcontrollers-microprocessors/stm32l4s5vi.html

.. _STM32L4S5 reference manual:
   https://www.st.com/resource/en/reference_manual/dm00310109.pdf

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html
