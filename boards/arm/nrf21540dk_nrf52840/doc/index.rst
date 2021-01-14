.. _nrf21540dk_nrf52840:

nRF21540 DK
###########

Overview
********
The nRF21540 DK (PCA10112) shows possibility of the Nordic Semiconductor
nRF21540 Front End Module connected with nRF52840 ARM Cortex-M4F CPU.
The CPU provides support for the following devices:

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

.. figure:: img/nrf21540dk_nrf52840.jpg
     :width: 442px
     :align: center
     :alt: nRF21540 DK

     nRF21540 DK (Credit: Nordic Semiconductor)

More information about the board can be found at the `nRF21540 website`_.
The `Nordic Semiconductor Infocenter`_ contains the processor's and front end
module's information and the datasheet.

Hardware
********

The nRF52840 on the nRF21540 DK has two external oscillators. The frequency
of the slow clock is 32.768 kHz. The frequency of the main clock is 32 MHz.

Supported Features
==================

The nrf21540dk_nrf52840 board configuration supports the following
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
| RADIO     | on-chip    | Bluetooth,           |
|           |            | ieee802154           |
+-----------+------------+----------------------+
| RTC       | on-chip    | system clock         |
+-----------+------------+----------------------+
| RTT       | Segger     | console              |
+-----------+------------+----------------------+
| SPI(M/S)  | on-chip    | spi                  |
+-----------+------------+----------------------+
| UART      | on-chip    | serial               |
+-----------+------------+----------------------+
| USB       | on-chip    | usb                  |
+-----------+------------+----------------------+
| WDT       | on-chip    | watchdog             |
+-----------+------------+----------------------+

Other hardware features are not supported by the Zephyr kernel.
See `nRF52840 Product Specification`_ and `Nordic Semiconductor Infocenter`_
for a complete list of nRF21540 Development Kit board hardware features.

Connections and IOs
===================

LED
---

* LED1 (green) = P0.13
* LED2 (green) = P0.14
* LED3 (green) = P0.15
* LED4 (green) = P0.16

Push buttons
------------

* BUTTON1 = SW1 = P0.11
* BUTTON2 = SW2 = P0.12
* BUTTON3 = SW3 = P0.24
* BUTTON4 = SW4 = P0.25
* BOOT = SW5 = boot/reset

Front End Module
----------------

* MOSI        = P1.13
* MISO        = P1.14
* CLOCK       = P1.15
* CHIP SELECT = P0.21
* PDN         = P0.23
* MODE        = P0.17
* RXEN        = P0.19
* ANTSEL      = P0.20
* TXEN        = P0.22

Programming and Debugging
*************************

Applications for the ``nrf21540dk_nrf52840`` board configuration can be
built and flashed in the usual way (see :ref:`build_an_application`
and :ref:`application_run` for more details); however, the standard
debugging targets are not currently available.

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

Replace :code:`<tty_device>` with the port where the board nRF21540 DK
can be found. For example, under Linux, :code:`/dev/ttyACM0`.

Then build and flash the application in the usual way.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: nrf21540dk_nrf52840
   :goals: build flash

Debugging
=========

Refer to the :ref:`nordic_segger` page to learn about debugging Nordic boards with a
Segger IC.


Testing the LEDs and buttons in the nRF21540 DK
***********************************************

There are 2 samples that allow you to test that the buttons (switches) and LEDs on
the board are working properly with Zephyr:

.. code-block:: console

   samples/basic/blinky
   samples/basic/button

You can build and flash the examples to make sure Zephyr is running correctly on
your board. The button and LED definitions can be found in
:zephyr_file:`boards/arm/nrf21540dk_nrf52840/nrf21540dk_nrf52840.dts`.

Using UART1
***********

The following approach can be used when an application needs to use
more than one UART for connecting peripheral devices:

1. Add devicetree overlay file to the main directory of your application:

   .. code-block:: console

      $ cat nrf21540dk_nrf52840.overlay
      &uart1 {
        compatible = "nordic,nrf-uarte";
        current-speed = <115200>;
        status = "okay";
        tx-pin = <14>;
        rx-pin = <16>;
      };

   In the overlay file above, pin P0.16 is used for RX and P0.14 is used for TX

2. Use the UART1 as ``device_get_binding(DT_LABEL(DT_NODELABEL(uart1)))``

See :ref:`set-devicetree-overlays` for further details.

Selecting the pins
==================
To select the pin numbers for tx-pin and rx-pin:

.. code-block:: console

   tx-pin = <pin_no>

Open the `nRF52840 Product Specification`_, chapter 7 'Hardware and Layout'.
In the table 7.1.1 'aQFN73 ball assignments' select the pins marked
'General purpose I/O'.  Note that pins marked as 'low frequency I/O only' can only be used
in under-10KHz applications. They are not suitable for 115200 speed of UART.

Translate the 'Pin' into number for devicetree by using the following formula::

   pin_no = b\*32 + a

where ``a`` and ``b`` are from the Pin value in the table (Pb.a).
For example, for P0.1, ``pin_no = 1`` and for P1.0, ``pin_no = 32``.

References
**********

.. target-notes::

.. _Nordic Semiconductor Infocenter: https://infocenter.nordicsemi.com
.. _J-Link Software and documentation pack: https://www.segger.com/jlink-software.html
.. _nRF21540 website: https://www.nordicsemi.com/Products/Low-power-short-range-wireless/nRF21540
.. _nRF52840 Product Specification: http://infocenter.nordicsemi.com/pdf/nRF52840_PS_v1.0.pdf
.. _nRF21540 Product Specification: http://infocenter.nordicsemi.com/pdf/nRF21540_PS_v1.0.pdf
