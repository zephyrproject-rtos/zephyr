.. zephyr:board:: canbardo

Overview
********

CANbardo is an open hardware Universal Serial Bus (USB) to Controller Area Network (CAN) adapter
board. It is designed to be compatible with the open source :ref:`external_module_cannectivity`.

Hardware
********

The CANbardo board is equipped with an Atmel SAME70N20B microcontroller and features an USB-C
connector (high-speed USB 2.0), two DB-9M connectors for CAN FD (up to 8 Mbit/s), a number of status
LEDs, and a push button. Schematics and component placement drawings are available in the `CANbardo
GitHub repository`_.

Supported Features
==================

The ``canbardo`` board supports the following hardware features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| PINMUX    | on-chip    | pinmux                              |
+-----------+------------+-------------------------------------+
| FLASH     | on-chip    | flash memory                        |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | gpio                                |
+-----------+------------+-------------------------------------+
| USB       | on-chip    | USB                                 |
+-----------+------------+-------------------------------------+
| UART1     | on-chip    | serial console                      |
+-----------+------------+-------------------------------------+
| CAN0      | on-chip    | CAN controller                      |
+-----------+------------+-------------------------------------+
| CAN1      | on-chip    | CAN controller                      |
+-----------+------------+-------------------------------------+

The default configuration can be found in the defconfig file:
:zephyr_file:`boards/others/canbardo/canbardo_defconfig`.

Other hardware features are not currently supported by the port.

System Clock
============

The SAME70N20B is driven by a 12 MHz crystal and configured to provide a system clock of 300
MHz. The two CAN FD controllers have a core clock frequency of 80 MHz.

Programming and Debugging
*************************

Build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: canbardo
   :Goals: flash

.. _CANbardo GitHub repository:
   https://github.com/CANbardo/canbardo
