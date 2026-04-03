.. zephyr:board:: arduino_portenta_c33

Overview
********

The Portenta C33 is a powerful System-on-Module based on the Renesas RA6M5
microcontroller group, which utilizes the high-performance Arm® Cortex®-M33
core. The Portenta C33 shares the same form factor as the Portenta H7 and is
backward compatible with it, making it fully compatible with all Portenta
family shields and carriers through its High-Density connectors.

Hardware
********

- Renesas RA6M5 ARM Cortex-M33 processor at 200 MHz
- 24 MHz crystal oscillator
- 32.768 kHz crystal oscillator for RTC
- 2 MB flash memory and 512 KiB of RAM
- 16 MB external QSPI flash
- One RGB user LED
- One reset button
- NXP SE050 secure element
- Onboard 10/100 Ethernet PHY
- WiFi + Bluetooth via ESP32-C3 running `esp-hosted`_ firmware
- Battery charger
- MKR header connector exposing standard peripherals (UART, SPI, I2C, ADC, PWM)
- 160 pins high density Portenta connectors exposing SD, CAN, I2S, SWD interfaces

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The `Arduino store`_ has detailed information about board connections. Download
the `Arduino Portenta C33 Schematic`_ for more details.

Serial Port
===========

The Portenta C33 exposes 4 serial ports with hardware flow control.

PWM
===

The Portenta C33 exposes 10 dedicated independent PWM pins.

USB Device Port
===============

The RA6M5 MCU has an high speed USB device port that can be used to communicate
with a host PC.  See the :zephyr:code-sample-category:`usb` sample applications for
more, such as the :zephyr:code-sample:`usb-cdc-acm` sample which sets up a virtual
serial port that echos characters back to the host PC.
A second full speed USB interface is exposed on the high density connectors.

DAC
===

The RA6M5 MCU has two DACs with 12 bits of resolution. On the
Arduino Portenta C33, the DACs are available on pins A5 and A6.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The Arduino Portenta C33 ships with a DFU compatible bootloader. The
bootloader can be entered by quickly tapping the reset button twice.

Flashing
========

#. Build the Zephyr kernel and the :zephyr:code-sample:`hello_world` sample application:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: arduino_portenta_c33
      :goals: build
      :compact:

#. Connect the Portenta C33 to your host computer using USB

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
      :board: arduino_portenta_c33
      :goals: flash
      :compact:

   You should see "Hello World! arduino_portenta_c33" in your terminal.

References
**********

.. target-notes::

.. _Arduino Store:
    https://store.arduino.cc/products/portenta-c33

.. _Arduino Portenta C33 Schematic:
    http://docs.arduino.cc/resources/schematics/ABX00074-schematics.pdf

.. _esp-hosted:
    https://github.com/espressif/esp-hosted
