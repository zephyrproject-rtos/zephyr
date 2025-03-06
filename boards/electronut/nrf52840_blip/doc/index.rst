.. zephyr:board:: nrf52840_blip

Overview
********

The Electronut Labs Blip hardware provides support for the Nordic Semiconductor
nRF52840 ARM Cortex-M4F CPU and the following devices:

* :abbr:`ADC (Analog to Digital Converter)`
* CLOCK
* FLASH
* :abbr:`GPIO (General Purpose Input Output)`
* :abbr:`I2C (Inter-Integrated Circuit)`
* :abbr:`MPU (Memory Protection Unit)`
* :abbr:`NVIC (Nested Vectored Interrupt Controller)`
* :abbr:`PWM (Pulse Width Modulation)`
* RADIO (Bluetooth Low Energy and 802.15.4)
* :abbr:`RTC (nRF RTC System Clock)`
* Segger RTT (RTT Console)
* :abbr:`SPI (Serial Peripheral Interface)`
* :abbr:`UART (Universal asynchronous receiver-transmitter)`
* :abbr:`USB (Universal Serial Bus)`
* :abbr:`WDT (Watchdog Timer)`

More information about the board is available at https://github.com/electronut/ElectronutLabs-blip.

Hardware
********

Blip has two external oscillators. The frequency of
the slow clock is 32.768 kHz. The frequency of the main clock
is 32 MHz.

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

LED
---

* LED1 (green) = P0.13
* LED2 (red)   = P0.14
* LED3 (blue)  = P0.15

Push buttons
------------

* BUTTON1 = SW1 = P1.07
* Reset = SW5 = P0.18 (can be used as GPIO also)

UART
----

BMP does not support hardware flow control, so only RX/TX pins are connected.

* TX = P0.6
* RX = P0.8

I2C
---

I2C pins connected to onboard sensors:

* SDA = P0.12
* SCL = P0.11

SPI
---

* SCK = P0.25
* MOSI = P1.02
* MISO = P0.24

MicroSD is connected to these pins, and CS pin is connected to P0.17.

Programming and Debugging
*************************

Applications for the ``nrf52840_blip`` board configuration can be
built and flashed in the usual way (see :ref:`build_an_application`
and :ref:`application_run` for more details); The onboard Black Magic
Probe debugger presents itself as two USB-serial ports. On Linux,
they may come up as ``/dev/ttyACM0`` and ``/dev/ttyACM1``. The first
one of these (``/dev/ttyACM0`` here) is the debugger port.
GDB can directly connect to this port without requiring a GDB server by specifying
``target external /dev/ttyACM0``. The second port acts as a
serial port, connected to the SoC.

Flashing
========

Applications are flashed and run as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Here is an example for the :zephyr:code-sample:`hello_world` application.

First, run your favorite terminal program to listen for output.

.. code-block:: console

   $ minicom -D <tty_device> -b 115200

Replace :code:`<tty_device>` with the serial port of Black Magic Probe.
For example, under Linux, :code:`/dev/ttyACM1`.

Then build and flash the application in the usual way.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: nrf52840_blip
   :goals: build flash

Debugging
=========

Debug and attach configurations are available using Black Magic Probe, and
``ninja debug``, or ``ninja attach`` (or with ``make``) are available.

NOTE: You may need to press the reset button once after using ``ninja flash``
to start executing the code. (not required with ``debug`` or ``attach``)


Testing the LEDs and buttons in the nRF52840 PDK
************************************************

There are 2 samples that allow you to test that the buttons (switches) and LEDs on
the board are working properly with Zephyr:

* :zephyr:code-sample:`blinky`
* :zephyr:code-sample:`button`

You can build and flash the examples to make sure Zephyr is running correctly on
your board. The button and LED definitions can be found in
:zephyr_file:`boards/electronut/nrf52840_blip/nrf52840_blip.dts`.


References
**********

.. target-notes::

.. _Electronut Labs website: https://electronut.in
.. _Store link: https://www.tindie.com/stores/ElectronutLabs/
.. _Blip website: https://github.com/electronut/ElectronutLabs-blip
.. _Schematic: https://github.com/electronut/ElectronutLabs-blip/blob/master/blip_v0.3_schematic.pdf
.. _Nordic Semiconductor Infocenter: http://infocenter.nordicsemi.com/
.. _Black Magic Probe website: https://github.com/blacksphere/blackmagic
