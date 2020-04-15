.. _nrf9160dk_nrf52840:

nRF9160 DK - nRF52840
#####################

Overview
********

The nRF52840 SoC on the nRF9160 DK (PCA10090) hardware provides support for the
Nordic Semiconductor nRF52840 ARM Cortex-M4F CPU and the following devices:

* CLOCK
* FLASH
* :abbr:`GPIO (General Purpose Input Output)`
* :abbr:`MPU (Memory Protection Unit)`
* :abbr:`NVIC (Nested Vectored Interrupt Controller)`
* :abbr:`PWM (Pulse Width Modulation)`
* RADIO (Bluetooth Low Energy and 802.15.4)
* :abbr:`RTC (nRF RTC System Clock)`
* Segger RTT (RTT Console)
* :abbr:`UART (Universal asynchronous receiver-transmitter)`
* :abbr:`WDT (Watchdog Timer)`

The nRF52840 SoC does not have any connection to the any of the LEDs,
buttons, switches, and Arduino pin headers on the nRF9160 DK board. It is,
however, possible to route some of the pins of the nRF52840 SoC to the nRF9160
SiP.

More information about the board can be found at
the `Nordic Low power cellular IoT`_ website.
The `Nordic Semiconductor Infocenter`_
contains the processor's information and the datasheet.

.. note::

   In previous Zephyr releases this board was named *nrf52840_pca10090*.

Hardware
********

The nRF9160 DK has two external oscillators. The frequency of
the slow clock is 32.768 kHz. The frequency of the main clock
is 32 MHz.

Supported Features
==================

The nrf9160dk_nrf52840 board configuration supports the following
hardware features:

+-----------+------------+----------------------+
| Interface | Controller | Driver/Component     |
+===========+============+======================+
| CLOCK     | on-chip    | clock_control        |
+-----------+------------+----------------------+
| FLASH     | on-chip    | flash                |
+-----------+------------+----------------------+
| GPIO      | on-chip    | gpio                 |
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
| RTT       | Segger     | console              |
+-----------+------------+----------------------+
| UART      | on-chip    | serial               |
+-----------+------------+----------------------+
| WDT       | on-chip    | watchdog             |
+-----------+------------+----------------------+

Programming and Debugging
*************************

Applications for the ``nrf9160dk_nrf52840`` board configuration can be
built and flashed in the usual way (see :ref:`build_an_application`
and :ref:`application_run` for more details).

Make sure that the PROG/DEBUG switch on the DK is set to nRF52.

Flashing
========

Follow the instructions in the :ref:`nordic_segger` page to install
and configure all the necessary software. Further information can be
found in :ref:`nordic_segger_flashing`. Then build and flash
applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Remember to set the PROG/DEBUG switch on the DK to nRF52.

See the following example for the :ref:`hello_world` application.

First, run your favorite terminal program to listen for output.

.. code-block:: console

   $ minicom -D <tty_device> -b 115200

Replace :code:`<tty_device>` with the port where the nRF52840 SoC is connected
to. Usually, under Linux it will be ``/dev/ttyACM1``. The ``/dev/ttyACM0``
port is connected to the nRF9160 SiP on the board.

Then build and flash the application in the usual way.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: nrf9160dk_nrf52840
   :goals: build flash

Debugging
=========

Refer to the :ref:`nordic_segger` page to learn about debugging Nordic boards
with a Segger IC.

Remember to set the PROG/DEBUG switch on the DK to nRF52.

Board controller firmware
*************************

The board controller firmware is a small snippet of code that takes care of
routing specific pins on nRF9160 SiP to different components on the board,
such as LEDs, switches, and specific nRF52840 SoC pins.

When compiling a project for nrf9160dk_nrf52840, the board controller firmware
will be compiled and run automatically after the Kernel has been initialized.

By default, the board controller firmware will route the following:

+-----------------+----------------------------------+
| Component       | Routed to                        |
+=================+==================================+
| nRF9160 UART0   | VCOM0                            |
+-----------------+----------------------------------+
| nRF9160 UART1   | VCOM2                            |
+-----------------+----------------------------------+
| LEDs 1-4        | physical LEDs                    |
+-----------------+----------------------------------+
| Buttons 1-2     | physical buttons                 |
+-----------------+----------------------------------+
| Switches 1-2    | physical switches                |
+-----------------+----------------------------------+
| MCU Interface 0 | Arduino pin headers              |
+-----------------+----------------------------------+
| MCU Interface 1 | Trace interface                  |
+-----------------+----------------------------------+
| MCU Interface 2 | COEX interface                   |
+-----------------+----------------------------------+

It is possible to configure the behavior of the board controller firmware by
using Kconfig and editing its options under "Board options".


References
**********

.. target-notes::
.. _Nordic Low power cellular IoT: https://www.nordicsemi.com/Products/Low-power-cellular-IoT
.. _Nordic Semiconductor Infocenter: https://infocenter.nordicsemi.com
.. _J-Link Software and documentation pack: https://www.segger.com/jlink-software.html
