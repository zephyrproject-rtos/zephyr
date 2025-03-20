.. zephyr:board:: ebyte_e73_tbb

Overview
********

The EBYTE E73-TBB hardware provides
support for the Nordic Semiconductor nRF52832 ARM Cortex-M4F CPU and
the following devices:

* :abbr:`ADC (Analog to Digital Converter)`
* CLOCK
* FLASH
* :abbr:`GPIO (General Purpose Input Output)`
* :abbr:`I2C (Inter-Integrated Circuit)`
* :abbr:`MPU (Memory Protection Unit)`
* :abbr:`NVIC (Nested Vectored Interrupt Controller)`
* :abbr:`PWM (Pulse Width Modulation)`
* RADIO (Bluetooth Low Energy)
* :abbr:`RTC (nRF RTC System Clock)`
* Segger RTT (RTT Console)
* :abbr:`SPI (Serial Peripheral Interface)`
* :abbr:`UART (Universal asynchronous receiver-transmitter)`
* :abbr:`WDT (Watchdog Timer)`

More information about the board can be found at the
`E73-TBB website`_. The `Nordic Semiconductor Infocenter`_
contains the processor's information and the datasheet.


Hardware
********

E73-TBB has two external oscillators. The frequency of
the slow clock is 32.768 kHz. The frequency of the main clock
is 32 MHz. Additionally the board features CH340 USB-UART converter.
It is possible to connect external BT antenna using U.FL socket
and solder NFC antenna using NFC_ANT connector.

Supported Features
==================

.. zephyr:board-supported-hw::

See `E73-TBB website`_ and `Nordic Semiconductor Infocenter`_
for a complete list of nRF52832 hardware features.

Connections and IOs
===================

LED
---

* LED0 (red) = P0.17
* LED1 (red) = P0.18

Push buttons
------------

* BUTTON0 = SW1 = P0.14
* BUTTON1 = SW2 = P0.13

External Connectors
-------------------

P1 Header

+-------+--------------+
| PIN # | Signal Name  |
+=======+==============+
| 1     | GND          |
+-------+--------------+
| 2     | 3.3V         |
+-------+--------------+
| 3     | P0.04        |
+-------+--------------+
| 4     | P0.03        |
+-------+--------------+
| 5     | P0.02        |
+-------+--------------+
| 6     | P0.31        |
+-------+--------------+
| 7     | P0.30        |
+-------+--------------+
| 8     | P0.29        |
+-------+--------------+
| 9     | P0.28        |
+-------+--------------+
| 10    | P0.27        |
+-------+--------------+
| 11    | P0.26        |
+-------+--------------+
| 12    | P0.25        |
+-------+--------------+

P2 Header

+-------+--------------+
| PIN # | Signal Name  |
+=======+==============+
| 1     | P0.24        |
+-------+--------------+
| 2     | P0.23        |
+-------+--------------+
| 3     | P0.22        |
+-------+--------------+
| 4     | SWDIO        |
+-------+--------------+
| 5     | SWDCLK       |
+-------+--------------+
| 6     | P0.21/RST    |
+-------+--------------+
| 7     | P0.20        |
+-------+--------------+
| 8     | P0.19        |
+-------+--------------+
| 9     | P0.16        |
+-------+--------------+
| 10    | P0.15        |
+-------+--------------+
| 11    | P0.12        |
+-------+--------------+
| 12    | P0.11        |
+-------+--------------+

NFC_ANT

+-------+--------------+
| PIN # | Signal Name  |
+=======+==============+
| 1     | P0.10        |
+-------+--------------+
| 2     | P0.09        |
+-------+--------------+

Programming and Debugging
*************************

Flashing
========

Follow the instructions in the :ref:`nordic_segger` page to install
and configure all the necessary software.
To flash the board connect pins: SWDIO, SWDCLK, RST, GND from E73-TBB
to corresponding pins on your J-Link device, then build and flash the application in the usual way.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: ebyte_e73_tbb/nrf52832
   :goals: build flash

Debugging
=========

Refer to the :ref:`nordic_segger` page to learn about debugging Nordic chips with a
Segger IC.


Testing the LEDs and buttons in the E73-TBB
*******************************************

There are 2 samples that allow you to test that the buttons (switches) and LEDs on
the board are working properly with Zephyr:

.. code-block:: console

   :zephyr:code-sample:`blinky`
   :zephyr:code-sample:`button`

You can build and flash the examples to make sure Zephyr is running correctly on
your board. The button and LED definitions can be found in
:zephyr_file:`boards/ebyte/e73_tbb/ebyte_e73_tbb_nrf52832.dts`.

References
**********

.. target-notes::

.. _E73-TBB website: https://www.cdebyte.com/products/E73-TBB
.. _Nordic Semiconductor Infocenter: https://infocenter.nordicsemi.com
