.. zephyr:board:: adafruit_feather_m4_express

Overview
********

The Adafruit Feather M4 Express is a compact, lightweight
ARM development board with an onboard mini NeoPixel, 2 MiB
of SPI flash, charging status indicator and user LEDs, USB
connector, 21 GPIO pins and a small prototyping area.

Hardware
********

- ATSAMD51J19A ARM Cortex-M4 processor at 120 MHz
- 512 KiB of flash memory and 192 KiB of RAM
- 2 MiB of SPI flash
- Internal trimmed 8 MHz oscillator
- A user LED
- An RGB NeoPixel LED
- Native USB port
- One reset button

Supported Features
==================

The ``adafruit_feather_m4_express`` board target supports the following
hardware features:

+-----------+------------+------------------------------------------+
| Interface | Controller | Driver/Component                         |
+===========+============+==========================================+
| SYSTICK   | on-chip    | systick                                  |
+-----------+------------+------------------------------------------+
| WDT       | on-chip    | Watchdog                                 |
+-----------+------------+------------------------------------------+
| GPIO      | on-chip    | I/O ports                                |
+-----------+------------+------------------------------------------+
| USART     | on-chip    | Serial ports                             |
+-----------+------------+------------------------------------------+
| SPI       | on-chip    | Serial Peripheral Interface ports        |
+-----------+------------+------------------------------------------+
| TRNG      | on-chip    | True Random Number Generator             |
+-----------+------------+------------------------------------------+
| HWINFO    | on-chip    | Unique 128 bit serial number             |
+-----------+------------+------------------------------------------+
| RTC       | on-chip    | Real-Time Counter                        |
+-----------+------------+------------------------------------------+
| USB       | on-chip    | USB device                               |
+-----------+------------+------------------------------------------+
| WDT       | on-chip    | Watchdog Timer                           |
+-----------+------------+------------------------------------------+
| PWM       | on-chip    | PWM                                      |
+-----------+------------+------------------------------------------+

Other hardware features are not currently supported by Zephyr.

The default configuration can be found in the Kconfig file
:zephyr_file:`boards/adafruit/feather_m4_express/adafruit_feather_m4_express_defconfig`.

Zephyr can use the default Cortex-M SYSTICK timer or the SAM0 specific RTC.
To use the RTC, set :code:`CONFIG_CORTEX_M_SYSTICK=n` and set
:code:`CONFIG_SYS_CLOCK_TICKS_PER_SEC` to no more than 32 kHZ divided by 7,
i.e. no more than 4500.

Connections and IOs
===================

The `Adafruit Learning System`_ has detailed information about
the board including `pinouts`_ and the `schematic`_.

System Clock
============

The SAMD51 MCU is configured to use the 32 kHz internal oscillator
with the on-chip PLL generating the 120 MHz system clock.

Serial Port
===========

The SAMD51 MCU has 6 SERCOM based USARTs.  On the Feather, SERCOM5 is
the Zephyr console and is available on pins 0 (RX) and 1 (TX).

SPI Port
========

The SAMD51 MCU has 6 SERCOM based SPIs. On the Feather, SERCOM1 can be put
into SPI mode and used to connect to devices over the SCK (SCLK), MO (MOSI), and
MI (MISO) pins.

PWM
===

The SAMD51 has three PWM generators with up to six channels each.  :code:`TCC_0`
has a resolution of 24 bits and all other generators are 16 bit.  :code:`TCC_1`
pin 2 is mapped to PA18 (D7) and pin 3 is mapped to PA19 (D9).

USB Device Port
===============

The SAMD51 MCU has a USB device port that can be used to communicate
with a host PC.  See the :ref:`usb` sample applications for
more, such as the :zephyr:code-sample:`usb-cdc-acm` sample which sets up a virtual
serial port that echos characters back to the host PC.

Programming and Debugging
*************************

The Feather ships with a the BOSSA compatible UF2 bootloader.  The
bootloader can be entered by quickly tapping the reset button twice.

Additionally, if :kconfig:option:`CONFIG_USB_CDC_ACM` is enabled then the
bootloader will be entered automatically when you run :code:`west flash`.

Flashing
========

#. Build the Zephyr kernel and the :zephyr:code-sample:`hello_world` sample application:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: adafruit_feather_m4_express
      :goals: build
      :compact:

#. Connect the feather to your host computer using USB

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

#. Tap the reset button twice quickly to enter bootloader mode

#. Flash the image:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: adafruit_feather_m4_express
      :goals: flash
      :compact:

   You should see "Hello World! adafruit_feather_m4_express" in your terminal.

Debugging
=========

In addition to the built-in bootloader, the Feather can be flashed and
debugged using a SWD probe such as the Segger J-Link.

#. Connect the board to the probe by connecting the :code:`SWCLK`,
   :code:`SWDIO`, :code:`RESET`, :code:`GND`, and :code:`3V3` pins on the
   Feather to the :code:`SWCLK`, :code:`SWDIO`, :code:`RESET`, :code:`GND`,
   and :code:`VTref` pins on the `J-Link`_.

#. Flash the image:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: adafruit_feather_m4_express
      :goals: flash
      :flash-args: -r openocd
      :compact:

#. Start debugging:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: adafruit_feather_m4_express
      :goals: debug
      :compact:

References
**********

.. target-notes::

.. _Adafruit Learning System:
    https://learn.adafruit.com/adafruit-feather-m4-express-atsamd51

.. _pinouts:
    https://learn.adafruit.com/adafruit-feather-m4-express-atsamd51/pinouts

.. _schematic:
    https://learn.adafruit.com/adafruit-feather-m4-express-atsamd51/downloads

.. _J-Link:
    https://www.segger.com/products/debug-probes/j-link/technology/interface-description/
