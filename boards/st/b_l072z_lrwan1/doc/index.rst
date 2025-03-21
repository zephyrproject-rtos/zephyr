.. zephyr:board:: b_l072z_lrwan1

Overview
********

This Discovery kit features an all-in-one open module CMWX1ZZABZ-091 (by Murata).
The module is powered by an STM32L072CZ and an SX1276 transceiver.

This kit provides:

- CMWX1ZZABZ-091 LoRa* / Sigfox* module (Murata)

        - Embedded ultra-low-power STM32L072CZ Series MCUs, based on
          Arm* Cortex* -M0+ core, with 192 Kbytes of Flash
          memory, 20 Kbytes of RAM, 6 Kbytes of EEPROM
        - Frequency range: 860 MHz - 930 MHz
        - USB 2.0 FS
        - 4-channel,12-bit ADC, 2xDAC
        - 6-bit timers, LP-UART, I2C and SPI
        - Embedded SX1276 transceiver
        - LoRa* , FSK, GFSK, MSK, GMSK and OOK modulations (+ Sigfox* compatibility)
        - +14 dBm or +20 dBm selectable output power
        - 157 dB maximum link budget
        - Programmable bit rate up to 300 kbit/s
        - High sensitivity: down to -137 dBm
        - Bullet-proof front end: IIP3 = -12.5 dBm
        - 89 dB blocking immunity
        - Low Rx current of 10 mA, 200 nA register retention
        - Fully integrated synthesizer with a resolution of 61 Hz
        - Built-in bit synchronizer for clock recovery
        - Sync word recognition
        - Preamble detection
        - 127 dB+ dynamic range RSSI

- SMA and U.FL RF interface connectors
- Including 50 ohm SMA RF antenna
- On-board ST-LINK/V2-1 supporting USB re-enumeration capability

- USB ST-LINK functions:
- Board power supply:

        - Through USB bus or external VIN/3.3 V supply voltage or batteries
- 3xAAA-type-battery holder for standalone operation
- 7 LEDs:

        - 4 general-purpose LEDs
        - A 5 V-power LED
        - An ST-LINK-communication LED
        - A fault-power LED
        - 2 push-buttons (user and reset)
- Arduino* Uno V3 connectors

More information about the board can be found at the `B-L072Z-LRWAN1 website`_.

Hardware
********

The STM32L072CZ SoC provides the following hardware IPs:

- Ultra-low-power (down to 0.29 µA Standby mode and 93 uA/MHz run mode)
- Core: ARM* 32-bit Cortex*-M0+ CPU, frequency up to 32 MHz
- Clock Sources:

        - 1 to 32 MHz crystal oscillator
        - 32 kHz crystal oscillator for RTC (LSE)
        - Internal 16 MHz factory-trimmed RC ( |plusminus| 1%)
        - Internal low-power 37 kHz RC ( |plusminus| 5%)
        - Internal multispeed low-power 65 kHz to 4.2 MHz RC
- RTC with HW calendar, alarms and calibration
- Up to 24 capacitive sensing channels: support touchkey, linear and rotary touch sensors
- 11x timers:

        - 2x 16-bit with up to 4 channels
        - 2x 16-bit with up to 2 channels
        - 1x 16-bit ultra-low-power timer
        - 1x SysTick
        - 1x RTC
        - 2x 16-bit basic for DAC
        - 2x watchdogs (independent/window)
- Up to 84 fast I/Os, most 5 V-tolerant.
- Memories

        - Up to 192 KB Flash, 2 banks read-while-write, proprietary code readout protection
        - Up to 20 KB of SRAM
        - External memory interface for static memories supporting SRAM, PSRAM, NOR and NAND memories
- Rich analog peripherals (independent supply)

        - 1x 12-bit ADC 1.14 MSPS
        - 2x 12-bit DAC
        - 2x ultra-low-power comparators
- 11x communication interfaces

        - USB 2.0 full-speed device, LPM and BCD
        - 3x I2C FM+(1 Mbit/s), SMBus/PMBus
        - 4x USARTs (ISO 7816, LIN, IrDA, modem)
        - 6x SPIs (4x SPIs with the Quad SPI)
- 7-channel DMA controller
- True random number generator
- CRC calculation unit, 96-bit unique ID
- Development support: serial wire debug (SWD), JTAG, Embedded Trace Macrocell*


More information about STM32L072CZ can be found here:

- `STM32L072CZ on www.st.com`_
- `STM32L0x2 reference manual`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

B-L072Z-LRWAN1 Discovery kit has GPIO controllers. These controllers are responsible for pin muxing,
input/output, pull-up, etc.

Available pins:
---------------

For detailed information about available pins please refer to `B-L072Z-LRWAN1 website`_.

Default Zephyr Peripheral Mapping:
----------------------------------

- UART_1_TX/RX: PA9/PA10 (Arduino Serial)
- UART_2_TX/RX: PA2/PA3 (ST-Link Virtual COM Port)
- SPI1 NSS/SCK/MISO/MOSI: PA15/PB3/PA6/PA7 (Semtech SX1276 LoRa* Transceiver)
- SPI2 NSS/SCK/MISO/MOSI: PB12/PB13/PB14/PB15 (Arduino SPI)
- I2C1 SCL/SDA : PB8/PB9 (Arduino I2C)

System Clock
------------

B-L072Z-LRWAN1 Discovery board System Clock is at 32MHz.

Serial Port
-----------

B-L072Z-LRWAN1 Discovery board has 2 U(S)ARTs. The Zephyr console output is assigned to UART2.
Default settings are 115200 8N1.

USB device
----------

B-L072Z-LRWAN1 Discovery board has 1 USB device controller. However,
the USB data lines are not connected to the MCU by default. To connect
the USB data lines to the MCU, short solder bridges SB15 and SB16.

Programming and Debugging
*************************

B-L072Z-LRWAN1 Discovery board includes an ST-LINK/V2-1 embedded debug tool interface.

Applications for the ``b_l072z_lrwan1`` board configuration can be built and
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

Flashing an application to B-L072Z-LRWAN1 Discovery board
---------------------------------------------------------

Here is an example for the :zephyr:code-sample:`hello_world` application.

Connect the B-L072Z-LRWAN1 Discovery board to a STLinkV2 to your host computer using the USB port, then
run a serial host program to connect with your board. For example:

.. code-block:: console

   $ minicom -D /dev/ttyACM0

Then build and flash the application:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: b_l072z_lrwan1
   :goals: build flash

You should see the following message on the console:

.. code-block:: console

   $ Hello World! arm

Debugging
=========

You can debug an application in the usual way.  Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: b_l072z_lrwan1
   :maybe-skip-config:
   :goals: debug

.. _B-L072Z-LRWAN1 website:
   https://www.st.com/en/evaluation-tools/b-l072z-lrwan1.html

.. _STM32L072CZ on www.st.com:
   https://www.st.com/en/microcontrollers/stm32l072cz.html

.. _STM32L0x2 reference manual:
   https://www.st.com/resource/en/reference_manual/DM00108281.pdf

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html
