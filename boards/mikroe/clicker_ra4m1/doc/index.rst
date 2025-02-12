.. _mikroe_clicker_ra4m1:

Mikroe Clicker RA4M1
####################

Overview
********

The Mikroe Clicker RA4M1 development board contains a Renesas Cortex-M4 based
R7FA4M1AB3CFM Microcontroller operating at up to 48 MHz with 256 KB of Flash
memory and 32 KB of SRAM.

.. figure:: img/mikroe_clicker_ra4m1.jpg
   :align: center
   :alt: Clicker RA4M1

   Clicker RA4M1 (Credit: MikroElektronika d.o.o.)

Hardware
********

The Clicker RA4M1 board contains a USB Type-C connector, two LEDs, two push
buttons, and a reset button. It has J-Link onboard and mikroBUS socket for
interfacing with external electronics. For more information about the
development board see the `Clicker RA4M1 website`_.

Supported Features
==================

The Zephyr Mikroe Clicker RA4M1 configuration supports the following hardware
features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port-polling                 |
+-----------+------------+-------------------------------------+
| PINMUX    | on-chip    | pinmux                              |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | GPIO output                         |
|           |            | GPIO input                          |
+-----------+------------+-------------------------------------+

Other hardware features have not been enabled yet for this board.

The default configuration can be found in the defconfig file:
:zephyr_file:`boards/mikroe/clicker_ra4m1/mikroe_clicker_ra4m1_defconfig`.

Programming and debugging
*************************

Building & Flashing
===================

You can build and flash an application in the usual way (See
:ref:`build_an_application` and
:ref:`application_run` for more details).

Here is an example for building and flashing the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: mikroe_clicker_ra4m1
   :goals: build flash

Debugging
=========

Debugging also can be done in the usual way.
The following command is debugging the :zephyr:code-sample:`blinky` application.
Also, see the instructions specific to the debug server that you use.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: mikroe_clicker_ra4m1
   :maybe-skip-config:
   :goals: debug

References
**********

.. target-notes::

.. _Clicker RA4M1 website:
	https://www.mikroe.com/ra4m1-clicker
