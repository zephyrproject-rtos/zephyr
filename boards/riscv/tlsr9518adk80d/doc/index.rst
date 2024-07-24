.. _tlsr9518adk80d:

Telink TLSR9518ADK80D
#####################

Overview
********

The TLSR9518A Generic Starter Kit is a hardware platform which
can be used to verify the `Telink TLSR9 series chipset`_ and develop applications
for several 2.4 GHz air interface standards including Bluetooth 5.2 (Basic data
rate, Enhanced data rate, LE, Indoor positioning and BLE Mesh),
Zigbee 3.0, Homekit, 6LoWPAN, Thread and 2.4 Ghz proprietary.

.. figure:: img/tlsr9518adk80d.jpg
     :align: center
     :alt: TLSR9518ADK80D

More information about the board can be found at the `Telink B91 Generic Starter Kit Hardware Guide`_ website.

Hardware
********

The TLSR9518A SoC integrates a powerful 32-bit RISC-V MCU, DSP, AI Engine, 2.4 GHz ISM Radio, 256
KB SRAM (128 KB of Data Local Memory and 128 KB of Instruction Local Memory first 64 KB of this memory
supports retention feature), external Flash memory, stereo audio codec, 14 bit AUX ADC,
analog and digital Microphone input, PWM, flexible IO interfaces, and other peripheral blocks required
for advanced IoT, hearable, and wearable devices.

.. figure:: img/tlsr9518_block_diagram.jpg
     :align: center
     :alt: TLSR9518ADK80D_SOC

The TLSR9518ADK80D default board configuration provides the following hardware components:

- RF conducted antenna
- 1 MB External SPI Flash memory with reset button. (Possible to mount 1/2/4 MB)
- Chip reset button
- Mini USB interface
- 4-wire JTAG
- 4 LEDs, Key matrix up to 4 keys
- 2 line-in function (Dual Analog microphone supported when switching jumper from microphone path)
- Dual Digital microphone
- Stereo line-out

Supported Features
==================

The Zephyr TLSR9518ADK80D board configuration supports the following hardware features:

+----------------+------------+------------------------------+
| Interface      | Controller | Driver/Component             |
+================+============+==============================+
| PLIC           | on-chip    | interrupt_controller         |
+----------------+------------+------------------------------+
| RISC-V Machine | on-chip    | timer                        |
| Timer (32 KHz) |            |                              |
+----------------+------------+------------------------------+
| PINCTRL        | on-chip    | pinctrl                      |
+----------------+------------+------------------------------+
| GPIO           | on-chip    | gpio                         |
+----------------+------------+------------------------------+
| UART           | on-chip    | serial                       |
+----------------+------------+------------------------------+
| PWM            | on-chip    | pwm                          |
+----------------+------------+------------------------------+
| TRNG           | on-chip    | entropy                      |
+----------------+------------+------------------------------+
| FLASH (MSPI)   | on-chip    | flash                        |
+----------------+------------+------------------------------+
| RADIO          | on-chip    | Bluetooth,                   |
|                |            | ieee802154, OpenThread       |
+----------------+------------+------------------------------+
| SPI (Master)   | on-chip    | spi                          |
+----------------+------------+------------------------------+
| I2C (Master)   | on-chip    | i2c                          |
+----------------+------------+------------------------------+
| ADC            | on-chip    | adc                          |
+----------------+------------+------------------------------+
| USB (device)   | on-chip    | usb_dc                       |
+----------------+------------+------------------------------+
| AES            | on-chip    | mbedtls                      |
+----------------+------------+------------------------------+
| PKE            | on-chip    | mbedtls                      |
+----------------+------------+------------------------------+

Board supports power-down modes: suspend and deep-sleep. For deep-sleep mode only 64KB of retention memory is available.
Board supports HW cryptography acceleration (AES and ECC till 256 bits). MbedTLS interface is used as cryptography front-end.

.. note::
   To support "button" example project PC3-KEY3 (J20-19, J20-20) jumper needs to be removed and KEY3 (J20-19) should be connected to GND (J50-23) externally.

   For the rest example projects use the default jumpers configuration.

Limitations
-----------

- Maximum 3 GPIO ports could be configured to generate external interrupts simultaneously. All ports should use different IRQ numbers.
- DMA mode is not supported by I2C, SPI and Serial Port.
- SPI Slave mode is not implemented.
- I2C Slave mode is not implemented.
- Bluetooth is not compatible with deep-sleep mode. Only suspend is allowed when Bluetooth is active.
- USB working only in active mode (No power down supported).
- During deep-sleep all GPIO's are in Hi-Z mode.
- Shell is not compatible with sleep modes.

Default configuration and IOs
=============================

System Clock
------------

The TLSR9518ADK80D board is configured to use the 24 MHz external crystal oscillator
with the on-chip PLL/DIV generating the 48 MHz system clock.
The following values also could be assigned to the system clock in the board DTS file
(``boards/riscv/tlsr9518adk80d/tlsr9518adk80d-common.dtsi``):

- 16000000
- 24000000
- 32000000
- 48000000
- 60000000
- 96000000

.. code-block::

   &cpu0 {
       clock-frequency = <48000000>;
   };

PINs Configuration
------------------

The TLSR9518A SoC has five GPIO controllers (PORT_A to PORT_E), but only two are
currently enabled (PORT_B for LEDs control and PORT_C for buttons) in the board DTS file:

- LED0 (blue): PB4, LED1 (green): PB5, LED2 (white): PB6, LED3 (red): PB7
- Key Matrix SW0: PC2_PC3, SW1: PC2_PC1, SW2: PC0_PC3, SW3: PC0_PC1

Peripheral's pins on the SoC are mapped to the following GPIO pins in the
``boards/riscv/tlsr9518adk80d/tlsr9518adk80d-common.dtsi`` file:

- UART0 TX: PB2, RX: PB3
- UART1 TX: PC6, RX: PC7
- PWM Channel 0: PB4
- PSPI CS0: PC4, CLK: PC5, MISO: PC6, MOSI: PC7
- HSPI CS0: PA1, CLK: PA2, MISO: PA3, MOSI: PA4
- I2C SCL: PE1, SDA: PE3

Serial Port
-----------

The TLSR9518A SoC has 2 UARTs. The Zephyr console output is assigned to UART0.
The default settings are 115200 8N1.

Programming and debugging
*************************

Building
========

.. important::

   These instructions assume you've set up a development environment as
   described in the `Zephyr Getting Started Guide`_.

To build applications using the default RISC-V toolchain from Zephyr SDK, just run the west build command.
Here is an example for the "hello_world" application.

.. code-block:: console

   # From the root of the zephyr repository
   west build -b tlsr9518adk80d samples/hello_world

Open a serial terminal with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Flash the board, reset and observe the following messages on the selected
serial port:

.. code-block:: console

   *** Booting Zephyr OS version 2.5.0  ***
   Hello World! tlsr9518adk80d


Flashing
========

To flash the TLSR9528A board see the sources below:

- `Burning and Debugging Tools for all Series`_

It is also possible to use the west flash command. Download BDT tool for Linux `Burning and Debugging Tools for Linux`_ or
`Burning and Debugging Tools for Windows`_ and extract archive into some directory you wish TELINK_BDT_BASE_DIR

- Now you should be able to run the west flash command with the BDT path specified (TELINK_BDT_BASE_DIR).

.. code-block:: console

   west flash --bdt-path=$TELINK_BDT_BASE_DIR --erase

- You can also run the west flash command without BDT path specification if TELINK_BDT_BASE_DIR is in your environment (.bashrc).

.. code-block:: console

   export TELINK_BDT_BASE_DIR="/opt/telink_bdt/"

References
**********

.. target-notes::

.. _Telink TLSR9 series chipset: https://wiki.telink-semi.cn/wiki/chip-series/TLSR951x-Series/
.. _Telink B91 Generic Starter Kit Hardware Guide: https://wiki.telink-semi.cn/wiki/Hardware/B91_Generic_Starter_Kit_Hardware_Guide/
.. _Burning and Debugging Tools for all Series: https://wiki.telink-semi.cn/wiki/IDE-and-Tools/Burning-and-Debugging-Tools-for-all-Series/
.. _Burning and Debugging Tools for Linux: https://wiki.telink-semi.cn/tools_and_sdk/Tools/BDT/Telink_libusb_BDT-Linux-X64-V1.6.0.zip
.. _Burning and Debugging Tools for Windows: https://wiki.telink-semi.cn/tools_and_sdk/Tools/BDT/BDT.zip
.. _Zephyr Getting Started Guide: https://docs.zephyrproject.org/latest/getting_started/index.html
