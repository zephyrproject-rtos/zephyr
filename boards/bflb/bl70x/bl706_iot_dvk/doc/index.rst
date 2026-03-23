.. zephyr:board:: bl706_iot_dvk

Overview
********

The BL706-IoT-DVK is a development board from Bouffalo Lab based on the BL706
(QFN48 package, BL706C00Q2I) RISC-V microcontroller. It features USB-C
connectivity, four indicator LEDs, and exposes most GPIO pins for peripheral
evaluation. A USB-to-serial backplane provides JTAG debugging and UART console
access.

Hardware
********

The BL706C00Q2I SoC has the following features:

- 32-bit RISC-V core (SiFive E24) running at 144 MHz
- 132 KB SRAM (64 KB instruction TCM + 64 KB data TCM/cache + 4 KB retention)
- 4 MB external flash (XIP) on module
- USB 2.0 FS device
- 2x UART, 1x SPI, 1x I2C, 1x I2S
- 5-channel PWM
- 12-bit ADC / 10-bit DAC
- Hardware security engine (AES/SHA/TRNG)
- Bluetooth Low Energy 5.0 / Zigbee
- 32 GPIO pins (QFN48 package)

The board provides:

- USB-C connector (for power, USB device interface, and CDC-ACM console)
- USB-to-serial backplane (for UART console and JTAG debugging)
- 4 indicator LEDs (TX0, RX0, TX1, RX1)
- UART0 on GPIO14 (TX) / GPIO15 (RX)
- UART1 on GPIO18 (TX) / GPIO19 (RX)
- SPI on GPIO3 (SCLK) / GPIO4 (MOSI) / GPIO5 (MISO) / GPIO6 (SS)
- I2C on GPIO5 (SDA) / GPIO6 (SCL) (shared with SPI, disabled by default)
- Exposed GPIO headers for peripheral connections

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
- UART0_TX : GPIO14
- UART0_RX : GPIO15
- UART1_TX : GPIO18
- UART1_RX : GPIO19
- SPI0_SCLK : GPIO3
- SPI0_MOSI : GPIO4
- SPI0_MISO : GPIO5
- SPI0_SS : GPIO6
- I2C0_SDA : GPIO5 (shared with SPI)
- I2C0_SCL : GPIO6 (shared with SPI)
- TX0_LED : GPIO22
- RX0_LED : GPIO29
- TX1_LED : GPIO30
- RX1_LED : GPIO31

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Building
========

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: bl706_iot_dvk
   :goals: build

Flashing
========

The board can be flashed over UART using ``bflb-mcu-tool``. Hold the **Boot**
button while pressing **Reset** to enter the bootloader, then run:

.. code-block:: console

   west flash

Console
=======

By default the board uses a USB CDC-ACM UART as the Zephyr console. After
flashing and resetting the board, the console is available on the USB serial
port at 115200 baud.

Alternatively, UART0 is available via the USB-to-serial backplane for console
access without USB CDC-ACM.

References
**********

.. target-notes::

.. _`Bouffalo Lab BL702/BL706 Product Page`: https://en.bouffalolab.com/product/?type=detail&id=4
