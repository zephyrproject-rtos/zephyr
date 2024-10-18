.. zephyr:board:: samd20_xpro

Overview
********

The SAM D20 Xplained Pro evaluation kit is ideal for evaluation and
prototyping with the SAM D20 Cortex®-M0+ processor-based
microcontrollers. The kit includes Atmel’s Embedded Debugger (EDBG),
which provides a full debug interface without the need for additional
hardware.

Hardware
********

- SAMD20J18 ARM Cortex-M0+ processor at 48 MHz
- 32.768 kHz crystal oscillator
- 256 KiB flash memory and 32 KiB of RAM
- One yellow user LED
- One mechanical user push button
- One reset button
- On-board USB based EDBG unit with serial console

Supported Features
==================

The samd20_xpro board configuration supports the following hardware
features:

.. list-table::
    :header-rows: 1

    * - Interface
      - Controller
      - Driver / Component
    * - NVIC
      - on-chip
      - nested vector interrupt controller
    * - Flash
      - on-chip
      - Can be used with LittleFS to store files
    * - SYSTICK
      - on-chip
      - systick
    * - WDT
      - on-chip
      - Watchdog
    * - ADC
      - on-chip
      - Analog to Digital Converter
    * - GPIO
      - on-chip
      - I/O ports
    * - USART
      - on-chip
      - Serial ports
    * - I2C
      - on-chip
      - I2C ports
    * - SPI
      - on-chip
      - Serial Peripheral Interface ports

Other hardware features are not currently supported by Zephyr.

The default configuration can be found in the Kconfig
:zephyr_file:`boards/atmel/sam0/samd20_xpro/samd20_xpro_defconfig`.

Connections and IOs
===================

The `Microchip website`_ has detailed information about board
connections. Download the `SAM D20 Xplained Pro Schematic`_ for more detail.

System Clock
============

The SAMD20 MCU is configured to use the 32.768 kHz external oscillator
with the on-chip PLL generating the 48 MHz system clock.

Serial Port
===========

The SAMD20 MCU has 6 SERCOM based USARTs. One of the USARTs
(SERCOM3) is connected to the onboard Atmel Embedded Debugger (EDBG).
SERCOM4 is available on the EXT1 connector.

SPI Port
========

The SAMD20 MCU has 6 SERCOM based SPIs. On the SAM D20 Xplained Pro,
SERCOM0 is available on the EXT1 connector.

Programming and Debugging
*************************

The SAM D20 Xplained Pro comes with a Atmel Embedded Debugger (EDBG).  This
provides a debug interface to the SAMD20 chip and is supported by
OpenOCD.

Flashing
========

#. Build the Zephyr kernel and the :zephyr:code-sample:`hello_world` sample application:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: samd20_xpro
      :goals: build
      :compact:

#. Connect the SAM D20 Xplained Pro to your host computer using the USB debug
   port.

#. Run your favorite terminal program to listen for output. Under Linux the
   terminal should be :code:`/dev/ttyACM0`. For example:

   .. code-block:: console

      $ minicom -D /dev/ttyACM0 -o

   The -o option tells minicom not to send the modem initialization
   string. Connection should be configured as follows:

   - Speed: 115200
   - Data: 8 bits
   - Parity: None
   - Stop bits: 1

#. To flash an image:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: samd20_xpro
      :goals: flash
      :compact:

   You should see "Hello World! samd20_xpro" in your terminal.

References
**********

.. target-notes::

.. _Microchip Technology:
    https://www.microchip.com/DevelopmentTools/ProductDetails.aspx?PartNO=ATSAMD20-XPRO

.. _Microchip website:
    https://www.microchip.com/DevelopmentTools/ProductDetails.aspx?PartNO=ATSAMD20-XPRO

.. _SAM D20 Xplained Pro Schematic:
    http://ww1.microchip.com/downloads/en/DeviceDoc/SAMD20-Xplained-Pro_Design-Documentation.zip
