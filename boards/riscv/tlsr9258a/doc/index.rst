.. _tlsr9258a:

Telink TLSR9258A
#####################

Overview
********

The TLSR9258A Generic Starter Kit is a hardware platform which
can be used to verify the `Telink TLSR9 series chipset`_ and develop applications
for several 2.4 GHz air interface standards including Bluetooth low energy,
Zigbee 3.0, Homekit, 6LoWPAN, Thread and 2.4 Ghz proprietary.

.. figure:: img/tlsr9258a.jpg
     :align: center
     :alt: TLSR9258A

More information about the board can be found at the `Telink B95 Generic Starter Kit Hardware Guide`_ website.

Hardware
********

TLSR9258A is a single chip SoC for Bluetooth low energy and 802.15.4. The embedded 2.4GHz transceiver
supports Bluetooth low energy, 802.15.4 as well as 2.4GHz proprietary operation. The TLSR9258 integrates a
powerful 32-bit RISC-V MCU, 512KB SRAM including at least 256KB retention SRAM, up to 4MB embedded
and up to 16MB external flash, 12-bit ADC, PWM, flexible IO interfaces, and other peripheral blocks. The
TLSR9258 supports standards and industrial alliance specifications including Bluetooth LE, Zigbee, Thread,
Matter, 2.4GHz proprietary standard.

.. figure:: img/tlsr9258a_block_diagram.jpg
     :align: center
     :alt: TLSR9258A_SOC

The TLSR9258A default board configuration provides the following hardware components:

- RF conducted antenna
- 1 MB External SPI Flash memory with reset button. (Possible to mount 1/2/4 MB)
- Chip reset button
- USB type-C interface
- 4-wire JTAG
- Key matrix up to 4 keys
- 4 LEDs
- 1 infra-red LED
- 1 analogue microphone with line-in function (switching by a jumper in microphone path)
- Dual Digital microphone

Supported Features
==================

The Zephyr TLSR9258A board configuration supports the following hardware features:

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

Board includes power management module: Embedded LDO and DCDC, Battery monitor for low battery voltage detection,
Brownout detection/shutdown and Power-On-Reset, Deep sleep with external wakeup (without SRAM retention),
Deep sleep with RTC and SRAM retention (32 KB SRAM retention).

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

The TLSR9258A board is configured to use the 24 MHz external crystal oscillator
with the on-chip PLL/DIV generating the 48 MHz system clock.
The following values also could be assigned to the system clock in the board DTS file
(``boards/riscv/tlsr9258a/tlsr9258a-common.dtsi``):

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

The TLSR9258A SoC has five GPIO controllers (PORT_A to PORT_F), and the next are
currently enabled:

- LED0 (blue): PC0, LED1 (green): PC2, LED2 (white): PC3, LED3 (red): PC1
- Key Matrix SW3: PD4_PD5, SW4: PD4_PD7, SW5: PD6_PD5, SW6: PD6_PD7

Peripheral's pins on the SoC are mapped to the following GPIO pins in the
``boards/riscv/tlsr9258a/tlsr9258a-common.dtsi`` file:

- UART0 TX: PB2, RX: PB3
- PWM Channel 0: PD0
- LSPI CLK: PE1, MISO: PE3, MOSI: PE2
- GSPI CLK: PA2, MISO: PA3, MOSI: PA4
- I2C SCL: PC0, SDA: PC1

Serial Port
-----------

The Zephyr console output is assigned to UART0.
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
   west build -b tlsr9258a samples/hello_world

Open a serial terminal with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Flash the board, reset and observe the following messages on the selected
serial port:

.. code-block:: console

   *** Booting Zephyr OS version 2.5.0  ***
   Hello World! tlsr9258a


Flashing
========

To flash the TLSR9258A board see the sources below:

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

.. _Telink TLSR9 series chipset: [UNDER_DEVELOPMENT]
.. _Telink B95 Generic Starter Kit Hardware Guide: [UNDER_DEVELOPMENT]
.. _Burning and Debugging Tools for all Series: https://wiki.telink-semi.cn/wiki/IDE-and-Tools/Burning-and-Debugging-Tools-for-all-Series/
.. _Burning and Debugging Tools for Linux: https://wiki.telink-semi.cn/tools_and_sdk/Tools/BDT/Telink_libusb_BDT-Linux-X64-V1.6.0.zip
.. _Burning and Debugging Tools for Windows: https://wiki.telink-semi.cn/tools_and_sdk/Tools/BDT/BDT.zip
.. _Zephyr Getting Started Guide: https://docs.zephyrproject.org/latest/getting_started/index.html
