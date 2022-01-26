.. _wio_terminal:

Wio Terminal
############

Overview
********

The Wio Terminal is a small (72 mm x 57 mm x 12 mm) and powerful ARM board
with wireless connectivity (bluetooth and 2.4GHz/5GHz Wi-Fi), LCD display,
USB C port, FPC connector, MicroSD card slot, Raspberry pi compatible 40pin
header and 2 Grove connectors.

.. image:: img/wio_terminal.png
     :width: 500px
     :align: center
     :alt: Wio Terminal

Hardware
********

Chips:

- primary chip: ATSAMD51P19 ARM Cortex-M4 processor at 120 MHzF
- 512 KB of flash memory and 192 KB of RAM
- 4 MB of external flash
- wireless connectivity chip: RTL8720DN dual core: ARM® Cortex®-M4F @ 200MHz & ARM® Cortex®-M0
- Internal trimmed 8 MHz oscillator

- 2.4inch LCD display
- A user LED

Built-in Modules:

- Accelerometer LIS3DHTR
- Microphone 1.0V-10V -42dB
- Speaker ≥78dB @10cm 4000Hz
- Light Sensor 400-1050nm
- Infrared Emitter 940nm

Interface:

- MicroSD Card Slot
- GPIO 40 pin (Raspberry Pi compatible)
- 2x Grove connectors
- USB C port (Power & USB-OTG)

Operation Interface:

- 3x user buttons
- 5-way user button
- power/reset/boot mode switch

Supported Features
==================

The wio_terminal board configuration supports the following
hardware features:

+-----------+------------+------------------------------------------+
| Interface | Controller | Driver/Component                         |
+===========+============+==========================================+
| NVIC      | on-chip    | Nested vector interrupt controller       |
+-----------+------------+------------------------------------------+
| SYSTICK   | on-chip    | systick                                  |
+-----------+------------+------------------------------------------+
| WDT       | on-chip    | Watchdog                                 |
+-----------+------------+------------------------------------------+
| GPIO      | on-chip    | I/O ports                                |
+-----------+------------+------------------------------------------+
| USART     | on-chip    | Serial ports                             |
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

Other hardware features are not currently supported by Zephyr.

The default configuration can be found in the Kconfig file
:zephyr_file:`boards/arm/wio_terminal/wio_terminal_defconfig`.

Zephyr can use the default Cortex-M SYSTICK timer or the SAM0 specific RTC.
To use the RTC, set :code:`CONFIG_CORTEX_M_SYSTICK=n` and set
:code:`CONFIG_SYS_CLOCK_TICKS_PER_SEC` to no more than 32 kHZ divided by 7,
i.e. no more than 4500.

Connections and IOs
===================

The `Wio Terminal Getting started guide`_ has detailed information about
the board including `pinouts`_ and the `schematic`_.

System Clock
============

The SAMD51 MCU is configured to use the 32 kHz internal oscillator
with the on-chip PLL generating the 120 MHz system clock.

Serial Port
===========

The SAMD51 MCU has 6 SERCOM based USARTs. On the Wio Terminal, SERCOM3 is
the Zephyr console and is available on pins 10 (RX) and 8 (TX).

USB Device Port
===============

The SAMD51 MCU has a USB device port that can be used to communicate
with a host PC.  See the :ref:`usb-samples` sample applications for
more, such as the :ref:`usb_cdc-acm` sample which sets up a virtual
serial port that echos characters back to the host PC.

Programming and Debugging
*************************

The Wio Terminal ships with a the BOSSA compatible UF2 bootloader.  The
bootloader can be entered by quickly tapping the reset button twice.

Additionally, if :code:`CONFIG_USB_CDC_ACM` is enabled then the bootloader
will be entered automatically when you run :code:`west flash`. (Note: The board may not enter bootloader after the first code:`west flash` call)

Flashing
========

#. Build the Zephyr kernel and the :code:`button` sample application:

   .. zephyr-app-commands::
      :zephyr-app: samples/basic/button
      :board: wio_terminal
      :goals: build
      :compact:

#. Swipe the reset/power button down twice quickly to enter bootloader mode

#. Flash the image:

   .. zephyr-app-commands::
      :zephyr-app: samples/basic/button
      :board: wio_terminal
      :goals: flash
      :compact:

   You should see the blue (user) LED flashing whenever you press the
   third (counting from the top left) user button at the top of the
   Wio Terminal.

Debugging
=========

In addition to the built-in bootloader, the Wio Terminal can be flashed and
debugged using a SWD probe such as the Segger J-Link.

#.  Solder cables to the code:`SWCLK`, :code:`SWDIO`, :code:`RESET`,
    :code:`GND`, and :code:`3V3` pins. See `Test with SWD`_ for more
    information.

#. Connect the board to the probe by connecting the :code:`SWCLK`,
   :code:`SWDIO`, :code:`RESET`, :code:`GND`, and :code:`3V3` pins on the
   Wio Terminal to the :code:`SWCLK`, :code:`SWDIO`, :code:`RESET`,
   :code:`GND`, and :code:`VTref` pins on the `J-Link`_.

#. Flash the image:

   .. zephyr-app-commands::
      :zephyr-app: samples/basic/button
      :board: wio_terminal
      :goals: flash -r openocd
      :compact:

#. Start debugging:

   .. zephyr-app-commands::
      :zephyr-app: samples/basic/button
      :board: wio_terminal
      :goals: debug
      :compact:

References
**********

.. target-notes::

.. _Wio Terminal Getting started guide:
   https://wiki.seeedstudio.com/Wio-Terminal-Getting-Started/

.. _pinouts:
    https://wiki.seeedstudio.com/Wio-Terminal-Getting-Started/#pinout-diagram

.. _schematic:
    https://wiki.seeedstudio.com/Wio-Terminal-Getting-Started/#resources
.. _Test with SWD:
    https://wiki.seeedstudio.com/Wio-Terminal-Getting-Started/#test-with-swd

.. _J-Link:
    https://www.segger.com/products/debug-probes/j-link/technology/interface-description/
