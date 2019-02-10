.. _bbc_microbit:

BBC MicroBit
##############

Overview
********

The Micro Bit (also referred to as BBC Micro Bit, stylized as micro:bit) is an
ARM-based embedded system designed by the BBC for use in computer education in
the UK.

The board is 4 cm × 5 cm and has an ARM Cortex-M0 processor, accelerometer and
magnetometer sensors, Bluetooth and USB connectivity, a display consisting of
25 LEDs, two programmable buttons, and can be powered by either USB or an
external battery pack. The device inputs and outputs are through five ring
connectors that are part of the 23-pin edge connector.

* :abbr:`NVIC (Nested Vectored Interrupt Controller)`
* :abbr:`RTC (nRF RTC System Clock)`
* UART
* GPIO
* FLASH
* RADIO (Bluetooth Low Energy)

.. figure:: img/bbc_microbit.png
     :width: 442px
     :align: center
     :alt: BBC Micro Bit

     BBC Micro Bit (Credit: http://microbit.org/)

More information about the board can be found at the `microbit website`_.

Hardware
********

The micro:bit has the following physical features:

* 25 individually-programmable LEDs
* 2 programmable buttons
* Physical connection pins
* Light and temperature sensors
* Motion sensors (accelerometer and compass)
* Wireless Communication, via Radio and Bluetooth
* USB interface


Supported Features
==================

The bbc_microbit board configuration supports the following nRF51
hardware features:

+-----------+------------+----------------------+
| Interface | Controller | Driver/Component     |
+===========+============+======================+
| NVIC      | on-chip    | nested vectored      |
|           |            | interrupt controller |
+-----------+------------+----------------------+
| RTC       | on-chip    | system clock         |
+-----------+------------+----------------------+
| UART      | on-chip    | serial port          |
+-----------+------------+----------------------+
| GPIO      | on-chip    | gpio                 |
+-----------+------------+----------------------+
| FLASH     | on-chip    | flash                |
+-----------+------------+----------------------+
| RADIO     | on-chip    | Bluetooth            |
+-----------+------------+----------------------+

Programming and Debugging
*************************

Flashing
========

Build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Here is an example for the :ref:`hello_world` application.

First, run your favorite terminal program to listen for output.

.. code-block:: console

   $ minicom -D <tty_device> -b 115200

Replace :code:`<tty_device>` with the port where the board nRF51 DK
can be found. For example, under Linux, :code:`/dev/ttyACM0`.

Then build and flash the application in the usual way.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: bbc_microbit
   :goals: build flash


References
**********

.. target-notes::

.. _microbit website: http://www.microbit.org/

