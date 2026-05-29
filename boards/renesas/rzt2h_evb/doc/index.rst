.. zephyr:board:: rzt2h_evb

Overview
********

The Evaluation Board Kit for RZ/T2H is an evaluation and development kit for the RZ/T2H MPU. Since
it has an on-board emulator, you can start evaluation by simply connecting the bundled cable with
your PC. This board includes rich functional ICs such as Gigabit Ethernet PHY, non-volatile memory
and LPDDR4 memory, and you can evaluate various functions of RZ/T2H without an extension board.

In addition, by expanding the inverter board, it is also possible to realize multi-axis motor control.

* On-board RZ/T2H MPU 729-pin (R9A09G077M44GBG)
* Rich functional ICs such as Gigabit Ethernet PHY, non-volatile memory (QSPI flash, Octa flash, eMMC)
  and DDR are mounted, functions of target MPU can be fully evaluated
* Micro SD card slot, SD card slot and PCIe connector
* Generic interface such as Pmod™/Grove®/QWIIC®/mikroBUS™
* The pin header enables users to freely combine with the user's hardware system and evaluate RZ/T2H
* Emulator circuit is mounted, can start program debugging by simply connecting USB cables:
  three USB cables are bundled for power supply (Type C, 15V), on-board emulator connection (Micro B)
  and terminal debugging (Mini B).

  * Please prepare USB PD power supply (45W = 15V-3A or more) by yourself.

* Renesas IDE e² studio and FSP for RZ/T2 are available

Hardware
********

Detailed hardware features for the board can be found at `RZ/T2H-EVKIT Website`_

- `RZ/T2H Group Website`_
- `RZ/T2H-EVKIT Website`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

By default, the board is configured for use with:

* UART0 connected to the USB serial port (CN34),
* LEDs defined as ``led0``, ``led1``, ``led2`` and ``led3``,

The Zephyr console uses UART0.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``rzt2h_evb`` board can be
built, flashed, and debugged in the usual way. See :ref:`build_an_application`
and :ref:`application_run` for more details on building and running.

To use J-Link OB,

1. Open the jumper pin (CN62) for switching the debug connection.

2. Connect the micro-USB type-B to J-Link OB USB connector (CN14), and then the LED10 is lighted.

Console
=======

The UART port is accessed by USB-Serial port (CN34).

Debugging
=========

Here is an example for building and debugging with the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: rzt2h_evb/r9a09g077m44gbg/cr52_0
   :goals: build debug

Flashing
=========

Before using ``flash`` command, the board must be set to xSPI boot mode.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: rzt2h_evb/r9a09g077m44gbg/cr520
   :goals: build flash

References
**********

.. target-notes::

.. _RZ/T2H Group Website:
   https://www.renesas.com/en/products/rz-t2h

.. _RZ/T2H-EVKIT Website:
   https://www.renesas.com/en/design-resources/boards-kits/rz-t2h-evkit
