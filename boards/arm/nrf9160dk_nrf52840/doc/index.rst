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

.. _nrf9160dk_board_controller_firmware:

Board controller firmware
*************************

The board controller firmware is a small snippet of code that takes care of
routing specific pins of the nRF9160 SiP to different components on the DK,
such as LEDs and buttons, UART interfaces (VCOMx) of the interface MCU, and
specific nRF52840 SoC pins.

.. note::
   In nRF9160 DK revisions earlier than v0.14.0, nRF9160 signals routed to
   other components on the DK are not simultaneously available on the DK
   connectors.

When compiling a project for nrf9160dk_nrf52840, the board controller firmware
will be compiled and run automatically after the Kernel has been initialized.

By default, the board controller firmware will route the following:

+--------------------------------+----------------------------------+
| nRF9160 pins                   | Routed to                        |
+================================+==================================+
| P0.26, P0.27, P0.28, and P0.29 | VCOM0                            |
+--------------------------------+----------------------------------+
| P0.01, P0.00, P0.15, and P0.14 | VCOM2                            |
+--------------------------------+----------------------------------+
| P0.02                          | LED1                             |
+--------------------------------+----------------------------------+
| P0.03                          | LED2                             |
+--------------------------------+----------------------------------+
| P0.04                          | LED3                             |
+--------------------------------+----------------------------------+
| P0.05                          | LED4                             |
+--------------------------------+----------------------------------+
| P0.08                          | Switch 1                         |
+--------------------------------+----------------------------------+
| P0.09                          | Switch 2                         |
+--------------------------------+----------------------------------+
| P0.06                          | Button 1                         |
+--------------------------------+----------------------------------+
| P0.07                          | Button 2                         |
+--------------------------------+----------------------------------+
| P0.17, P0.18, and P0.19        | Arduino pin headers              |
+--------------------------------+----------------------------------+
| P0.21, P0.22, and P0.23        | Trace interface                  |
+--------------------------------+----------------------------------+
| COEX0, COEX1, and COEX2        | COEX interface                   |
+--------------------------------+----------------------------------+

For a complete list of all the routing options available,
see the `nRF9160 DK board control section in the nRF9160 DK User Guide`_.

If you want to route some of the above pins differently or enable any of the
other available routing options, enable or disable the devicetree node that
represents the analog switch that provides the given routing.

The following devicetree nodes are defined for the analog switches present
on the nRF9160 DK:

+------------------------------------+------------------------------+
| Devicetree node label              | Analog switch name           |
+====================================+==============================+
| ``vcom0_pins_routing``             | nRF91_UART1 (nRF91_APP1)     |
+------------------------------------+------------------------------+
| ``vcom2_pins_routing``             | nRF91_UART2 (nRF91_APP2)     |
+------------------------------------+------------------------------+
| ``led1_pin_routing``               | nRF91_LED1                   |
+------------------------------------+------------------------------+
| ``led2_pin_routing``               | nRF91_LED2                   |
+------------------------------------+------------------------------+
| ``led3_pin_routing``               | nRF91_LED3                   |
+------------------------------------+------------------------------+
| ``led4_pin_routing``               | nRF91_LED4                   |
+------------------------------------+------------------------------+
| ``switch1_pin_routing``            | nRF91_SWITCH1                |
+------------------------------------+------------------------------+
| ``switch2_pin_routing``            | nRF91_SWITCH2                |
+------------------------------------+------------------------------+
| ``button1_pin_routing``            | nRF91_BUTTON1                |
+------------------------------------+------------------------------+
| ``button2_pin_routing``            | nRF91_BUTTON2                |
+------------------------------------+------------------------------+
| ``nrf_interface_pins_0_2_routing`` | nRF_IF0-2_CTRL (nRF91_GPIO)  |
+------------------------------------+------------------------------+
| ``nrf_interface_pins_3_5_routing`` | nRF_IF3-5_CTRL (nRF91_TRACE) |
+------------------------------------+------------------------------+
| ``nrf_interface_pins_6_8_routing`` | nRF_IF6-8_CTRL (nRF91_COEX)  |
+------------------------------------+------------------------------+

When building for the DK revision 0.14.0 or later, you can use the following
additional nodes (see :ref:`application_board_version` for information how to
build for specific revisions of the board):

+------------------------------------+------------------------------+
| Devicetree node label              | Analog switch name           |
+====================================+==============================+
| ``nrf_interface_pin_9_routing``    | nRF_IF9_CTRL                 |
+------------------------------------+------------------------------+
| ``io_expander_pins_routing``       | IO_EXP_EN                    |
+------------------------------------+------------------------------+
| ``external_flash_pins_routing``    | EXT_MEM_CTRL                 |
+------------------------------------+------------------------------+

For example, if you want to enable the optional routing for the nRF9160 pins
P0.17, P0.18, and P0.19 so that they are routed to nRF52840 pins P0.17, P0.20,
and P0.15, respectively, add the following in the devicetree overlay in your
application:

.. code-block:: none

   &nrf_interface_pins_0_2_routing {
           status = "okay";
   };

And if you want to, for example, disable routing for the VCOM2 pins, add the
following:

.. code-block:: none

   &vcom2_pins_routing {
           status = "disabled";
   };

A few helper .dtsi files are provided in the directories
:zephyr_file:`boards/arm/nrf9160dk_nrf52840/dts` and
:zephyr_file:`boards/arm/nrf9160dk_nrf9160/dts`. They can serve as examples of
how to configure and use the above routings. You can also include them from
respective devicetree overlay files in your applications to conveniently
configure the signal routing between nRF9160 and nRF52840 on the nRF9160 DK.
For example, to use ``uart1`` on both these chips for communication between
them, add the following line in the overlays for applications on both sides:

.. code-block:: none

   #include <nrf9160dk_uart1_on_if0_3.dtsi>

References
**********

.. target-notes::
.. _Nordic Low power cellular IoT: https://www.nordicsemi.com/Products/Low-power-cellular-IoT
.. _Nordic Semiconductor Infocenter: https://infocenter.nordicsemi.com
.. _J-Link Software and documentation pack: https://www.segger.com/jlink-software.html
.. _nRF9160 DK board control section in the nRF9160 DK User Guide: https://infocenter.nordicsemi.com/topic/ug_nrf91_dk/UG/nrf91_DK/board_controller.html
