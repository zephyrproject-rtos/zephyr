.. _seeeduino_xiao:

Seeeduino XIAO
##############

Overview
********

The Seeeduino XIAO is a tiny (20 mm x 17.5 mm) ARM development
board with onboard LEDs, USB port, and range of I/O broken out
onto 14 pins.

.. image:: img/seeeduino_xiao.jpg
     :align: center
     :alt: Seeeduino XIAO

Hardware
********

- ATSAMD21G18A ARM Cortex-M0+ processor at 48 MHz
- 256 KiB flash memory and 32 KiB of RAM
- Three user LEDs
- Native USB port

Supported Features
==================

The seeeduino_xiao board configuration supports the following hardware
features:

+-----------+------------+------------------------------------------+
| Interface | Controller | Driver/Component                         |
+===========+============+==========================================+
| DMA       | on-chip    | Direct memory access                     |
+-----------+------------+------------------------------------------+
| DAC       | on-chip    | Digital to analogue converter            |
+-----------+------------+------------------------------------------+
| Flash     | on-chip    | Can be used with LittleFS to store files |
+-----------+------------+------------------------------------------+
| GPIO      | on-chip    | I/O ports                                |
+-----------+------------+------------------------------------------+
| HWINFO    | on-chip    | Hardware info                            |
+-----------+------------+------------------------------------------+
| I2C       | on-chip    | Inter-Integrated Circuit                 |
+-----------+------------+------------------------------------------+
| NVIC      | on-chip    | nested vector interrupt controller       |
+-----------+------------+------------------------------------------+
| SPI       | on-chip    | Serial Peripheral Interface ports        |
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

The default configuration can be found in the Kconfig file
:zephyr_file:`boards/seeed/seeeduino_xiao/seeeduino_xiao_defconfig`.

Connections and IOs
===================

The `Seeeduino XIAO wiki`_ has detailed information about
the board including `pinouts`_ and the `schematic`_.

System Clock
============

The SAMD21 MCU is configured to use the 32 kHz external crystal
with the on-chip PLL generating the 48 MHz system clock.  The internal
APB and GCLK unit are set up in the same way as the upstream Arduino
libraries.

SPI Port
========

The SAMD21 MCU has 6 SERCOM based SPIs.  On the XIAO, SERCOM0 can be put
into SPI mode and used to connect to devices over pin 9 (MISO), pin 10
(MOSI), and pin 8 (SCK).

I2C Port
========

The SAMD21 MCU has 6 SERCOM based USARTs. On the XIAO, SERCOM2 is available on
pin 4 (SDA) and pin 5 (SCL).

Serial Port
===========

The SAMD21 MCU has 6 SERCOM based USARTs.  On the XIAO, SERCOM4 is
the Zephyr console and is available on pins 7 (RX) and 6 (TX).

USB Device Port
===============

The SAMD21 MCU has a USB device port that can be used to communicate
with a host PC.  See the :ref:`usb-samples` sample applications for
more, such as the :zephyr:code-sample:`usb-cdc-acm` sample which sets up a virtual
serial port that echos characters back to the host PC.

DAC
===

The SAMD21 MCU has a single channel DAC with 10 bits of resolution. On
the XIAO, the DAC is available on pin 0.

Programming and Debugging
*************************

The XIAO ships the BOSSA compatible UF2 bootloader.  The bootloader can be
entered by shorting the RST and GND pads twice.

Additionally, if :code:`CONFIG_USB_CDC_ACM` is enabled then the bootloader
will be entered automatically when you run :code:`west flash`.

Flashing
========

#. Build the Zephyr kernel and the :ref:`hello_world` sample application:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: seeeduino_xiao
      :goals: build
      :compact:

#. Connect the XIAO to your host computer using USB

#. Connect a 3.3 V USB to serial adapter to the board and to the
   host.  See the `Serial Port`_ section above for the board's pin
   connections.

#. Run your favorite terminal program to listen for output. Under Linux the
   terminal should be :code:`/dev/ttyUSB0`. For example:

   .. code-block:: console

      $ minicom -D /dev/ttyUSB0 -o

   The -o option tells minicom not to send the modem initialization
   string. Connection should be configured as follows:

   - Speed: 115200
   - Data: 8 bits
   - Parity: None
   - Stop bits: 1

#. Short the RST and GND pads twice quickly to enter bootloader mode

#. Flash the image:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: seeeduino_xiao
      :goals: flash
      :compact:

   You should see "Hello World! seeeduino_xiao" in your terminal.

References
**********

.. target-notes::

.. _Seeeduino XIAO wiki:
    https://wiki.seeedstudio.com/Seeeduino-XIAO/

.. _pinouts:
    https://wiki.seeedstudio.com/Seeeduino-XIAO/#hardware-overview

.. _schematic:
    https://wiki.seeedstudio.com/Seeeduino-XIAO/#resourses
