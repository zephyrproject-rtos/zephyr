.. _arduino_mkrzero:

Arduino MKR Zero
####################

Overview
********

The Arduino MKR Zero built with smaller MKR form factor and powered by Atmel's SAMD21 MCU.
This board come with microSD card holder that allows you to play with music files with no extra hardware.

.. image:: img/arduino_mkrzero.jpg
     :align: center
     :alt: Arduino MKR Zero

Hardware
********

- ATSAMD21G18A ARM Cortex-M0+ processor at 48 MHz
- 32.768 kHz crystal oscillator
- 256 KiB flash memory and 32 KiB of RAM
- One user LEDs
- One reset button
- microSD card slot
- ATECC508A secure element

Supported Features
==================

The arduino_mkrzero board configuration supports the following hardware
features:

+-----------+------------+------------------------------------------+
| Interface | Controller | Driver/Component                         |
+===========+============+==========================================+
| ADC       | on-chip    | Analog to digital converter              |
+-----------+------------+------------------------------------------+
| COUNTER   | on-chip    | Pulse counter                            |
+-----------+------------+------------------------------------------+
| DMA       | on-chip    | Direct memory access unit                |
+-----------+------------+------------------------------------------+
| Flash     | on-chip    | Can be used with LittleFS to store files |
+-----------+------------+------------------------------------------+
| GPIO      | on-chip    | I/O ports                                |
+-----------+------------+------------------------------------------+
| HWINFO    | on-chip    | Hardware info and serial number          |
+-----------+------------+------------------------------------------+
| NVIC      | on-chip    | nested vector interrupt controller       |
+-----------+------------+------------------------------------------+
| PWM       | on-chip    | Pulse Width Modulation                   |
+-----------+------------+------------------------------------------+
| SPI       | on-chip    | Serial Peripheral Interface ports        |
+-----------+------------+------------------------------------------+
| I2C       | on-chip    | Inter-Integrated Circuit ports           |
+-----------+------------+------------------------------------------+
| SYSTICK   | on-chip    | systick                                  |
+-----------+------------+------------------------------------------+
| USART     | on-chip    | Serial ports                             |
+-----------+------------+------------------------------------------+
| USB       | on-chip    | USB device                               |
+-----------+------------+------------------------------------------+
| WDT       | on-chip    | Watchdog                                 |
+-----------+------------+------------------------------------------+


Other hardware features are not currently supported by Zephyr.

The default configuration can be found in the Kconfig
:zephyr_file:`boards/arm/arduino_mkrzero/arduino_mkrzero_defconfig`.

Connections and IOs
===================

The `Arduino store`_ has detailed information about board
connections. Download the `Arduino MKR Zero Schematic`_ for more detail.

System Clock
============

The SAMD21 MCU is configured to use the 32.768 kHz external oscillator
with the on-chip PLL generating the 48 MHz system clock.  The internal
APB and GCLK unit are set up in the same way as the upstream Arduino
libraries.

Serial Port
===========

The SAMD21 MCU has 6 SERCOM based USARTs. SERCOM5 is available on pins 13(PA23) and 14(PA22).

PWM
===

The SAMD21 MCU has 3 TCC based PWM units with up to 4 outputs each and a period
of 24 bits or 16 bits.

SPI Port
========

The SAMD21 MCU has 6 SERCOM based SPIs.  On the Arduino MKR Zero, SERCOM1
is available on pin 8, 9, and 10.
SERCOM2 connect to microSD card slot as SPI interface.

I2C Port
========

The SAMD21 MCU has 6 SERCOM based I2Cs. SERCOM0 is available on pin 11(PA08) and 12(PA09).
This I2C bus also available as ESLOV(JST SH 5pin) socket.
ATECC508A secure element is connect to this I2C bus.

USB Device Port
===============

The SAMD21 MCU has a USB device port that can be used to communicate
with a host PC.  See the :ref:`usb-samples` sample applications for
more, such as the :zephyr:code-sample:`usb-cdc-acm` sample which sets up a virtual
serial port that echos characters back to the host PC.

DAC
===

The SAMD21 MCU has a single channel DAC with 10 bits of resolution. On the
Arduino MKR Zero, the DAC is available on pin A0.

Programming and Debugging
*************************

The Arduino MKR Zero ships the BOSSA compatible bootloader.  The
bootloader can be entered by quickly tapping the reset button twice.

Flashing
========

#. Build the Zephyr kernel and the :ref:`hello_world` sample application:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: arduino_mkrzero
      :goals: build
      :compact:

#. Connect the MKR Zero to your host computer using USB

#. Connect a 3.3 V USB to serial adapter to the board and to the
   host.  See the `Serial Port`_ section above for the board's pin
   connections.

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

#. Tap the reset button twice quickly to enter bootloader mode

#. Flash the image:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: arduino_mkrzero
      :goals: flash
      :compact:

   You should see "Hello World! arduino_mkrzero" in your terminal.

References
**********

.. target-notes::

.. _Arduino Store:
    https://store.arduino.cc/collections/boards/products/arduino-mkr-zero-i2s-bus-sd-for-sound-music-digital-audio-data

.. _Arduino MKR Zero Schematic:
    https://www.arduino.cc/en/uploads/Main/ArduinoMKRZero-schematic.pdf
