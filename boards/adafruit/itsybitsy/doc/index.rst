.. zephyr:board:: adafruit_itsybitsy

Overview
********

The Adafruit ItsyBitsy nRF52840 Express is a small (36 mm x 18 mm) ARM
development board with an onboard RGB LED, USB port, 2 MB of QSPI flash,
and range of I/O broken out onto 21 GPIO pins.

This development kit has the following features:

* :abbr:`ADC (Analog to Digital Converter)`
* CLOCK
* FLASH
* :abbr:`GPIO (General Purpose Input Output)`
* :abbr:`I2C (Inter-Integrated Circuit)`
* :abbr:`I2S (Inter-Integrated Sound)`
* :abbr:`MPU (Memory Protection Unit)`
* :abbr:`NVIC (Nested Vectored Interrupt Controller)`
* :abbr:`PWM (Pulse Width Modulation)`
* :abbr:`QSPI (Quad Serial Peripheral Interface)`
* RADIO (Bluetooth Low Energy and 802.15.4)
* :abbr:`RTC (nRF RTC System Clock)`
* :abbr:`SPI (Serial Peripheral Interface)`
* :abbr:`UARTE (Universal asynchronous receiver-transmitter)`
* :abbr:`USB (Universal Serial Bus)`
* :abbr:`WDT (Watchdog Timer)`

Hardware
********
- nRF52840 ARM Cortex-M4F CPU at 64MHz
- 1 MB of flash memory and 256 KB of SRAM
- 2 MB of QSPI flash
- A user LED
- A user switch
- An RGB DotStar LED
- Native USB port
- One reset button

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The `Adafruit ItsyBitsy nRF52840 Express Learn site`_ has detailed
information about the board including `pinouts`_ and the `schematic`_.

LED
---

* LED0 (red) = P0.06

* LED1 (Adafruit DotStar)

    * DATA = P0.08

    * CLK = P1.09

Push buttons
------------

* SWITCH = P0.29

* RESET = P0.18

Logging
-------

Logging is done using the USB-CDC port. See the :zephyr:code-sample:`logging` sample
or the :zephyr:code-sample:`usb-cdc-acm-console` sample applications to see how this works.

Testing LEDs and buttons on the Adafruit ItsyBitsy nRF52840 Express
*******************************************************************
The :zephyr:code-sample:`button` sample lets you test the buttons (switches) and the red LED.
The :zephyr:code-sample:`blinky` sample lets you test the red LED.

The DotStar LED has been implemented as a SPI device and can be tested
with the :zephyr:code-sample:`led-strip` sample application.

You can build and flash the examples to make sure Zephyr is running correctly on
your board. The button and LED definitions can be found in
:zephyr_file:`boards/adafruit/itsybitsy/adafruit_itsybitsy_nrf52840.dts`.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The ItsyBitsy ships with the BOSSA compatible UF2 bootloader.  The
bootloader can be entered by quickly tapping the reset button twice.

First time setup
================
Some versions of this board were shipped with a buggy bootloader.
Ensure that the bootloader is up to date by following the
`Adafruit UF2 Bootloader update`_ tutorial. Note that this tutorial
was made for the Adafruit Feather nRF52840, but the steps to update
the bootloader are the same for the ItsyBitsy. The files for the
ItsyBitsy bootloader can be found in the `Adafruit nRF52 Bootloader repo`_.

The building and flashing of Zephyr applications have been tested with
release 0.7.0 of the UF2 bootloader.

Flashing
========
Flashing is done by dragging and dropping the built Zephyr UF2-file
into the :code:`ITSY840BOOT` drive.

#. Build the Zephyr kernel and the :zephyr:code-sample:`blinky`
   sample application:

   .. zephyr-app-commands::
      :zephyr-app: samples/basic/blinky
      :board: adafruit_itsybitsy/nrf52840
      :goals: build
      :compact:

#. Connect the ItsyBitsy to your host computer using USB

#. Tap the reset button twice quickly to enter bootloader mode

#. Flash the image:

   Drag and drop the file :code:`samples/basic/blinky/build/zephyr/zephyr.uf2`
   into :code:`ITSY840BOOT`

The device will disconnect and you should see the red LED blink.

References
**********

.. target-notes::

.. _Adafruit ItsyBitsy nRF52840 Express Learn site:
    https://learn.adafruit.com/adafruit-itsybitsy-nrf52840-express

.. _pinouts:
    https://learn.adafruit.com/adafruit-itsybitsy-nrf52840-express/pinouts

.. _schematic:
    https://learn.adafruit.com/adafruit-itsybitsy-nrf52840-express/downloads

.. _Adafruit UF2 Bootloader update:
    https://learn.adafruit.com/introducing-the-adafruit-nrf52840-feather/update-bootloader

.. _Adafruit nRF52 Bootloader repo:
    https://github.com/adafruit/Adafruit_nRF52_Bootloader/releases
