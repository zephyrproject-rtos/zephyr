.. zephyr:board:: adafruit_trrs_trinkey

Overview
********

The Adafruit TRRS Trinkey is a small ARM development board with an onboard TRRS jack, RGB LED, USB
port, and STEMMA QT port.

Hardware
********

- ATSAMD21E18A ARM Cortex-M0+ processor at 48 MHz
- 256 KiB flash memory and 32 KiB of RAM
- Internal trimmed 8 MHz oscillator
- TRRS jack
- STEMMA QT I2C Port
- An RGB NeoPixel LED
- Native USB port
- One reset button

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The `Adafruit TRRS Trinkey Learn site`_ has detailed information about the board including
`pinouts`_ and the `schematic`_.

System Clock
============

The SAMD21 MCU is configured to use the 8 MHz internal oscillator with the on-chip PLL generating
the 48 MHz system clock.  The internal APB and GCLK unit are set up in the same way as the upstream
Arduino libraries.

TRRS Jack
=========

The TRRS jack is connected to the following pins of the SAMD21 MCU:

.. list-table::
   :header-rows: 1

   * - Pin
     - Function

   * - PA02 (AIN0)
     - Tip

   * - PA03 (AIN1)
     - Tip Switch

   * - PA04 (AIN4)
     - Ring 2

   * - PA05 (AIN5)
     - Sleeve

   * - PA06 (AIN6)
     - Ring 1

   * - PA07 (AIN7)
     - Ring Switch

USB Serial Port
===============

Since the TRRS Trinkey has a native USB port, and lacks dedicated UART pins, the USB CDC ACM serial
port is used for console I/O.

STEMMA QT I2C Port
==================

The SAMD21E MCU has 5 SERCOM-based I2C peripherals.  On the TRRS Trinkey, SERCOM2 is used to drive
the STEMMA QT I2C port.

RGB NeoPixel LED
==================

The board has one NeoPixel WS2812B RGB LED connected to pin PA01. Unfortunately no Zephyr driver
currently exists for this configuration of MCU and pin.

USB Device Port
===============

The SAMD21 MCU has a USB device port that can be used to communicate with a host PC.  See the
:zephyr:code-sample-category:`usb` sample applications for more. Since the TRRS Trinkey doesn't
have UART pins, the USB CDC ACM serial is enabled by default for console I/O.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The TRRS Trinkey ships the BOSSA compatible UF2 bootloader. The bootloader can be entered by
quickly tapping the reset button twice.

Flashing
========

#. Build the Zephyr kernel and the :zephyr:code-sample:`hello_world` sample application:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: adafruit_trrs_trinkey
      :goals: build
      :compact:

#. Connect the TRRS Trinkey to your host computer using USB

#. Run your favorite terminal program to listen for output. Under Linux the terminal should be
   :code:`/dev/ttyACM0`. For example:

   .. code-block:: console

      $ minicom -D /dev/ttyACM0 -o

   The -o option tells minicom not to send the modem initialization string. Connection should be
   configured as follows:

   - Speed: 115200
   - Data: 8 bits
   - Parity: None
   - Stop bits: 1

#. Tap the reset button twice quickly to enter bootloader mode

#. Flash the image:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: adafruit_trrs_trinkey
      :goals: flash
      :compact:

   You should see "Hello World! adafruit_trrs_trinkey/samd21e18a" in your terminal.

References
**********

.. target-notes::

.. _Adafruit TRRS Trinkey Learn site:
    https://learn.adafruit.com/adafruit-trrs-trinkey

.. _pinouts:
    https://github.com/adafruit/Adafruit-TRRS-Trinkey-PCB/blob/main/Adafruit%20TRRS%20Trinkey%20PrettyPins.pdf

.. _schematic:
    https://learn.adafruit.com/assets/130062
