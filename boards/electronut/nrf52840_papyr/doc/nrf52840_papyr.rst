.. zephyr:board:: nrf52840_papyr

Overview
********

Zephyr applications use the nrf52840_papyr board configuration
to run on Electronut Labs Papyr hardware. It provides
support for the Nordic Semiconductor nRF52840 ARM Cortex-M4F CPU and
the following devices:

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
* :abbr:`SPI (Serial Peripheral Interface)`
* :abbr:`UART (Universal asynchronous receiver-transmitter)`
* :abbr:`USB (Universal Serial Bus)`
* :abbr:`WDT (Watchdog Timer)`
* COUNTER

More information about the board is available at https://gitlab.com/electronutlabs-public/papyr.

Hardware
********

Papyr has two external oscillators. The frequency of
the slow clock is 32.768 kHz. The frequency of the main clock
is 32 MHz.

Supported Features
==================

The nrf52840_papyr board configuration supports the following
hardware features currently:

+-----------+------------+----------------------+
| Interface | Controller | Driver/Component     |
+===========+============+======================+
| ADC       | on-chip    | adc                  |
+-----------+------------+----------------------+
| CLOCK     | on-chip    | clock_control        |
+-----------+------------+----------------------+
| FLASH     | on-chip    | flash                |
+-----------+------------+----------------------+
| GPIO      | on-chip    | gpio                 |
+-----------+------------+----------------------+
| I2C(M)    | on-chip    | i2c                  |
+-----------+------------+----------------------+
| MPU       | on-chip    | arch/arm             |
+-----------+------------+----------------------+
| NVIC      | on-chip    | arch/arm             |
+-----------+------------+----------------------+
| PWM       | on-chip    | pwm                  |
+-----------+------------+----------------------+
| RADIO     | on-chip    | Bluetooth,           |
|           |            | ieee802154           |
+-----------+------------+----------------------+
| RTC       | on-chip    | system clock         |
+-----------+------------+----------------------+
| SPI(M/S)  | on-chip    | spi                  |
+-----------+------------+----------------------+
| UART      | on-chip    | serial               |
+-----------+------------+----------------------+
| USB       | on-chip    | usb                  |
+-----------+------------+----------------------+
| WDT       | on-chip    | watchdog             |
+-----------+------------+----------------------+

Connections and IOs
===================

LED
---

* LED1 (green) = P0.13
* LED2 (blue)  = P0.15
* LED3 (red)   = P0.14

Push buttons
------------

* Reset = SW0 = P0.18 (can be used as GPIO also)

UART
----

* TX = P0.8
* RX = P0.7

I2C
---

I2C pins connected to onboard sensors (I2C_0):

* SDA = P0.5
* SCL = P0.6

SPI
---

The e-paper display is connected to the chip via SPI on the following pins (SPI_1):

* SCK  = P0.31
* MOSI = P0.29
* MISO = P1.1 (not used by the display)

NOTE: P1.1 is pin 33 in absolute enumeration.

Other pins used by the e-paper display are:

* E-ink enable = P0.11 (cuts off power to the display with MOSFET)
* CS   = P0.30
* BUSY = P0.3
* D/C  = P0.28
* RES  = P0.2

Programming and Debugging
*************************

Applications for the ``nrf52840_papyr`` board configuration can be
built and flashed in the usual way (see :ref:`build_an_application`
and :ref:`application_run` for more details); Black Magic
Probe debugger presents itself as two USB-serial ports. On Linux,
they may come up as ``/dev/ttyACM0`` and ``/dev/ttyACM1``. The first
one of these (``/dev/ttyACM0`` here) is the debugger port.
GDB can directly connect to this port without requiring a GDB server by specifying
``target external /dev/ttyACM0``. The second port acts as a
serial port, connected to the SoC.

Flashing
========

By default, papyr is configured to be used with a blackmagicprobe compatible
debugger (see _Bumpy).

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
   :board: nrf52840_papyr
   :goals: build flash

Debugging
=========

Debug and attach configurations are available using Black Magic Probe, and
``ninja debug``, or ``ninja attach`` (or with ``make``) are available.

NOTE: You may need to press the reset button once after using ``ninja flash``
to start executing the code. (not required with ``debug`` or ``attach``)

References
**********

.. target-notes::

.. _Electronut Labs website: https://electronut.in
.. _Store link: https://www.tindie.com/stores/ElectronutLabs/
.. _Papyr website: https://docs.electronut.in/papyr/
.. _Schematic: https://gitlab.com/electronutlabs-public/papyr/raw/master/hardware/papyr_schematic_v_0_3.pdf?inline=false
.. _Datasheet: https://gitlab.com/electronutlabs-public/papyr/raw/master/papyr_v0.3_datasheet.pdf?inline=false
.. _Nordic Semiconductor Infocenter: http://infocenter.nordicsemi.com/
.. _Black Magic Probe website: https://github.com/blacksphere/blackmagic
.. _Bumpy website: https://docs.electronut.in/bumpy/
