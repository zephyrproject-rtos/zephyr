.. _ubx_bmd330eval_nrf52810:

u-blox EVK-BMD-330: BMD-330-EVAL
################################

Overview
********

The BMD-330-EVAL hardware provides support for the
u-blox BMD-330 Bluetooth 5 module, based on The
Nordic Semiconductor nRF52810 ARM Cortex-M4 CPU and
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

.. figure:: img/BMD-30-33-35-36-EVAL.jpg
	:align: center
	:alt: BMD-300-EVAL

	BMD-300-EVAL (Credit: u-blox AG)

.. note::
	The BMD-330-EVAL shares the same pin headers and assignments as the
	BMD-300-EVAL. The BMD-300-EVAL is shown here.

More information about the BMD-330-EVAL and the BMD-330 module
can be found at the `u-blox website`_. All of the Nordic
Semiconductor examples for the nRF52 DK (nrf52dk_nrf52810)
may be used without modification.

Hardware
********

The BMD-330 on the BMD-330-EVAL contains an internal
high-frequency oscillator at 32MHz. There is also a low frequency
(slow) oscillator of 32.768kHz. The BMD-330 itself does not include
the slow crystal; however, the BMD-330-EVAL does.

.. note::
	When targeting a custom design without a slow crystal,
	be sure to modify code to utilize the internal RC
	oscillator for the slow clock.

Supported Features
==================

The BMD-330-EVAL configuration supports the following
hardware features:

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
| RADIO     | on-chip    | Bluetooth            |
+-----------+------------+----------------------+
| RTC       | on-chip    | system clock         |
+-----------+------------+----------------------+
| RTT       | Segger     | console              |
+-----------+------------+----------------------+
| SPI(M/S)  | on-chip    | spi                  |
+-----------+------------+----------------------+
| UART      | on-chip    | serial               |
+-----------+------------+----------------------+
| WDT       | on-chip    | watchdog             |
+-----------+------------+----------------------+

Other hardware features have not been enabled yet for this board.
See the `u-blox website`_ for a complete list of
BMD-330-EVAL hardware features.

Connections and IOs
===================

LED
---

* LED1 (red) = P0.17
* LED2 (red) = P0.18
* LED3 (green) = P0.19
* LED4 (green) = P0.20
* D5 (red) = OB LED 1
* D6 (green) = OB LED 2

Push buttons
------------

* BUTTON1 = SW1 = P0.13
* BUTTON2 = SW2 = P0.14
* BUTTON3 = SW3 = P0.15
* BUTTON4 = SW4 = P0.16
* BOOT = SW5 = boot/reset

External Connectors
-------------------

.. figure:: img/bmd-300-eval_pin_out.jpg
	:align: center
	:alt: BMD-300-EVAL pin-out

	BMD-300-EVAL pin-out (Credit: u-blox AG)

.. note::
	The BMD-330-EVAL shares the same pin headers and assignments
	as the BMD-300-EVAL. The BMD-300-EVAL is shown here.

.. note::
	The pin numbers noted below are referenced to
	the pin 1 markings on the BMD-330-EVAL
	for each header

J-Link Prog Connector (J2)

+-------+--------------+
| PIN # | Signal Name  |
+=======+==============+
| 1     | VDD          |
+-------+--------------+
| 2     | IMCU_TMSS    |
+-------+--------------+
| 3     | GND          |
+-------+--------------+
| 4     | IMCU_TCKS    |
+-------+--------------+
| 5     | V5V          |
+-------+--------------+
| 6     | IMCU_TDOS    |
+-------+--------------+
| 7     | Cut off      |
+-------+--------------+
| 8     | IMCU_TDIS    |
+-------+--------------+
| 9     | Cut off      |
+-------+--------------+
| 10    | IMCU_RESET   |
+-------+--------------+


Debug OUT (J3)

+-------+----------------+
| PIN # | Signal Name    |
+=======+================+
| 1     | EXT_VTG        |
+-------+----------------+
| 2     | EXT_SWDIO      |
+-------+----------------+
| 3     | GND            |
+-------+----------------+
| 4     | EXT_SWDCLK     |
+-------+----------------+
| 5     | GND            |
+-------+----------------+
| 6     | EXT_SWO        |
+-------+----------------+
| 7     | N/C            |
+-------+----------------+
| 8     | N/C            |
+-------+----------------+
| 9     | EXT_GND_DETECT |
+-------+----------------+
| 10    | EXT_RESET      |
+-------+----------------+


Auxiliary (J9)

+-------+----------------+
| PIN # | Signal Name    |
+=======+================+
| 1     | P0.10          |
+-------+----------------+
| 2     | P0.09          |
+-------+----------------+
| 3     | P0.08          |
+-------+----------------+
| 4     | P0.07          |
+-------+----------------+
| 5     | P0.06          |
+-------+----------------+
| 6     | P0.05 / AIN3   |
+-------+----------------+
| 7     | P0.21 / RESET  |
+-------+----------------+
| 8     | P0.01 / XL2    |
+-------+----------------+
| 9     | P0.00 / XL1    |
+-------+----------------+
| 10    | GND            |
+-------+----------------+


Arduino Headers
---------------


Power (J5)

+-------+--------------+-------------------------+
| PIN # | Signal Name  | BMD-330 Functions       |
+=======+==============+=========================+
| 1     | VSHLD        | N/A                     |
+-------+--------------+-------------------------+
| 2     | VSHLD        | N/A                     |
+-------+--------------+-------------------------+
| 3     | RESET        | P0.21 / RESET           |
+-------+--------------+-------------------------+
| 4     | VSHLD        | N/A                     |
+-------+--------------+-------------------------+
| 5     | V5V          | N/A                     |
+-------+--------------+-------------------------+
| 6     | GND          | N/A                     |
+-------+--------------+-------------------------+
| 7     | GND          | N/A                     |
+-------+--------------+-------------------------+
| 8     | N/C          | N/A                     |
+-------+--------------+-------------------------+

Analog in (J8)

+-------+--------------+-------------------------+
| PIN # | Signal Name  | BMD-330 Functions       |
+=======+==============+=========================+
| 1     | A0           | P0.03 / AIN1            |
+-------+--------------+-------------------------+
| 2     | A1           | P0.04 / AIN2            |
+-------+--------------+-------------------------+
| 3     | A2           | P0.28 / AIN4            |
+-------+--------------+-------------------------+
| 4     | A3           | P0.29 / AIN5            |
+-------+--------------+-------------------------+
| 5     | A4           | P0.30 / AIN6            |
+-------+--------------+-------------------------+
| 6     | A5           | P0.31 / AIN7            |
+-------+--------------+-------------------------+

Digital I/O (J7)

+-------+--------------+----------------------------+
| PIN # | Signal Name  | BMD-330 Functions          |
+=======+==============+============================+
| 1     | D7           | P0.18                      |
+-------+--------------+----------------------------+
| 2     | D6           | P0.17                      |
+-------+--------------+----------------------------+
| 3     | D5           | P0.16                      |
+-------+--------------+----------------------------+
| 4     | D4           | P0.15                      |
+-------+--------------+----------------------------+
| 5     | D3           | P0.14                      |
+-------+--------------+----------------------------+
| 6     | D2           | P0.13                      |
+-------+--------------+----------------------------+
| 7     | D1 (TX)      | P0.12                      |
+-------+--------------+----------------------------+
| 8     | D0 (RX)      | P0.11                      |
+-------+--------------+----------------------------+

Digital I/O (J6)

+-------+--------------+-------------------------+
| PIN # | Signal Name  | BMD-330 Functions       |
+=======+==============+=========================+
| 1     | SCL          | P0.27                   |
+-------+--------------+-------------------------+
| 2     | SDA          | P0.26                   |
+-------+--------------+-------------------------+
| 3     | AREF         | P0.02 / AIN0            |
+-------+--------------+-------------------------+
| 4     | GND          | N/A                     |
+-------+--------------+-------------------------+
| 5     | D13 (SCK)    | P0.25                   |
+-------+--------------+-------------------------+
| 6     | D12 (MISO)   | P0.24                   |
+-------+--------------+-------------------------+
| 7     | D11 (MOSI)   | P0.23                   |
+-------+--------------+-------------------------+
| 8     | D10 (SS)     | P0.22                   |
+-------+--------------+-------------------------+
| 9     | D9           | P0.20                   |
+-------+--------------+-------------------------+
| 10    | D8           | P0.19                   |
+-------+--------------+-------------------------+

J11

+-------+--------------+-------------------------+
| PIN # | Signal Name  | BMD-330 Functions       |
+=======+==============+=========================+
| 1     | D12 (MISO)   | P0.24                   |
+-------+--------------+-------------------------+
| 2     | V5V          | N/A                     |
+-------+--------------+-------------------------+
| 3     | D13 (SCK)    | P0.25                   |
+-------+--------------+-------------------------+
| 4     | D11 (MOSI)   | P0.23                   |
+-------+--------------+-------------------------+
| 5     | RESET        | N/A                     |
+-------+--------------+-------------------------+
| 6     | N/A          | N/A                     |
+-------+--------------+-------------------------+

Programming and Debugging
*************************

Flashing
========

Follow the instructions in the :ref:`nordic_segger` page to install
and configure all the necessary software. Further information can be
found in :ref:`nordic_segger_flashing`. Then build and flash
applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Here is an example for the :ref:`hello_world` application.

First, run your favorite terminal program to listen for output.

.. code-block:: console

	$ minicom -D <tty_device> -b 115200

Replace :code:`<tty_device>` with the port where the
BMD-330-EVAL can be found. For example, under Linux,
:code:`/dev/ttyACM0`.

Then build and flash the application in the usual way.

.. zephyr-app-commands::
	:zephyr-app: samples/hello_world
	:board: ubx_bmd330eval_nrf52810
	:goals: build flash

Debugging
=========

Refer to the :ref:`nordic_segger` page to learn about debugging
u-blox boards with a Segger J-LINK-OB IC.


Testing the LEDs and buttons in the BMD-330-EVAL
************************************************

There are 2 samples that allow you to test that the buttons
(switches) and LEDs on the board are working properly with Zephyr:

.. code-block:: console

	samples/basic/blinky
	samples/basic/button

You can build and flash the examples to make sure Zephyr is
running correctly on your board. The button and LED definitions
can be found in :zephyr_file:`boards/arm/ubx_bmd330eval_nrf52810/ubx_bmd330eval_nrf52810.dts`.

References
**********

.. target-notes::

.. _u-blox website: https://www.u-blox.com/en/product/bmd-330-open-cpu
