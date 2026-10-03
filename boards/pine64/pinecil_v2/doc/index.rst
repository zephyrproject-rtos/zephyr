.. zephyr:board:: pinecil_v2

Overview
********

The Pinecil V2 is a portable soldering iron from Pine64 that doubles as a
development board. It includes a Bouffalo Lab BL706 MCU (QFN48 package,
BL706C10Q2I) RISC-V microcontroller. The Pinecil can be flashed using the USB-C
port, but to access any of the pins (e.g. UART, I2C, JTAG, etc.) the breakout
board from Pine64 is required.

Hardware
********

The BL706C10Q2I SoC has the following features:

- 32-bit RISC-V core (SiFive E24) running at 144 MHz
- 132 KB SRAM (64 KB instruction TCM + 64 KB data TCM/cache + 4 KB retention)
- 1 MB of integrated flash
- USB 2.0 FS device
- 2x UART, 1x SPI, 1x I2C, 1x I2S
- 5-channel PWM
- 12-bit ADC / 10-bit DAC
- Hardware security engine (AES/SHA/TRNG)
- Bluetooth Low Energy 5.0 / Zigbee
- 32 GPIO pins (QFN48 package)

Without the breakout board, the Pinecil v2 provides:

- USB-C connector (for power, USB device interface, and CDC-ACM console)
- 2 user buttons: ``-`` (GPIO28) and ``+`` (GPIO25)
- UART0 on GPIO22 (TX) / GPIO23 (RX)
- I2C on GPIO11 (SDA) / GPIO10 (SCL)
- SSD1306 display driver on I2C bus (GPIO3 used for reset)

There are other peripherals that are not yet supported. With the breakout board,
I2C, JTAG, SPI, UART, and USB header pins are exposed.

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

Default Zephyr Peripheral Mapping:
-----------------------------------

.. rst-class:: rst-columns

- USB_DP : GPIO7
- USB_DM : GPIO8
- UART0_TX : GPIO22
- UART0_RX : GPIO23
- I2C0_SDA : GPIO11
- I2C0_SCL : GPIO10
- INPUT_KEY_0 : GPIO28
- INPUT_KEY_1 : GPIO25

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Building
========

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: pinecil_v2
   :goals: build

Flashing
========

The board can be flashed over USB using ``bflb-mcu-tool``. Hold the ``-`` button
while connecting the device to the host machine to power on in bootloader mode:

.. code-block:: console

   west flash

Console
=======

By default the board uses a USB CDC-ACM UART as the Zephyr console. After
flashing and resetting the board, the console is available on the USB serial
port at 115200 baud.

Alternatively, UART0 is available via the breakout board for console access
without USB CDC-ACM.

References
**********

.. target-notes::

.. _`Pinecil v2 Product Page`: https://pine64.com/product/pinecil-smart-mini-portable-soldering-iron/
.. _`Pinecil Breakout Board Product Page`: https://pine64.com/product/pinecil-break-out-board/
.. _`Pinecil Documentation`: https://pine64.org/documentation/Pinecil/
.. _`Pinecil Wiki Page`: https://wiki.pine64.org/wiki/Pinecil
.. _`Bouffalo Lab BL702/BL706 Product Page`: https://en.bouffalolab.com/product/?type=detail&id=4
