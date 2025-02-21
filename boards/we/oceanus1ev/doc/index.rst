.. zephyr:board:: we_oceanus1ev

Overview
********

The we_oceanus1ev board is an evaluation board of the `Oceanus-I`_ radio module.
It provides support for the `STM32WLE5CC`_ ARM CPU and
the following devices:

* CLOCK
* FLASH
* :abbr:`GPIO (General Purpose Input Output)`
* :abbr:`I2C (Inter-Integrated Circuit)`
* :abbr:`NVIC (Nested Vectored Interrupt Controller)`
* RADIO (LoRa)
* :abbr:`RTC (STM32 RTC System Clock)`
* :abbr:`SPI (Serial Peripheral Interface)`
* :abbr:`UART (Universal asynchronous receiver-transmitter)`
* :abbr:`WDT (Watchdog Timer)`

Hardware
********

The board has below hardware features:

- `Oceanus-I`_, 256KB Flash, 64KB RAM with external antenna
- 1 FTDI chip (USB to UART) converter
- 1 I2C WE sensor EV-Boards connector
- 1 SPI WE sensor EV-Boards connector
- 2 application LEDs
- 1 application, and 1 reset push-button

Supported Features
==================

The ``we_oceanus1ev`` board supports the following
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
| I2C(M)    | on-chip    | i2c                  |
+-----------+------------+----------------------+
| MPU       | on-chip    | arch/arm             |
+-----------+------------+----------------------+
| NVIC      | on-chip    | arch/arm             |
+-----------+------------+----------------------+
| RADIO     | on-chip    | LoRa                 |
+-----------+------------+----------------------+
| RTC       | on-chip    | system clock         |
+-----------+------------+----------------------+
| SPI(M/S)  | on-chip    | spi                  |
+-----------+------------+----------------------+
| UART      | on-chip    | serial               |
+-----------+------------+----------------------+
| WDT       | on-chip    | watchdog             |
+-----------+------------+----------------------+

Programming and Debugging
*************************

Applications for the ``we_oceanus1ev`` board can be built the
usual way (see :ref:`build_an_application`).

The board debugged and flashed with an external debug probe connected
to the SWD pins, current native support is for the JLink debug probe.

Flashing
========

Connect the board to your host computer and build and flash an application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: we_oceanus1ev
   :goals: build flash

Run a serial terminal to connect with your board. By default, ``lpuart1`` is
accessible via the on-board FTDI USB to UART converter.

Debugging
=========

You can debug an application in the usual way.  Here is an example for the
:zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: we_oceanus1ev
   :maybe-skip-config:
   :goals: debug

References
**********

.. target-notes::

.. _`Oceanus-I`: https://www.we-online.com/katalog/de/OCEANUS-I
.. _`STM32WLE5CC`: https://www.st.com/en/microcontrollers-microprocessors/stm32wle5cc.html
