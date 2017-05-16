.. _hexiwear_kw40z:

Hexiwear KW40Z
##############

Overview
********

See :ref:`hexiwear_k64` for a general overview of the Hexiwear board and the
main application SoC, the K64. The KW40Z is a secondary SoC on the board that
provides wireless connectivity with a multimode BLE and 802.15.4 radio.

For more information about the KW40Z SoC:

- `KW40Z Website`_
- `KW40Z Datasheet`_
- `KW40Z Reference Manual`_

Supported Features
==================

The hexiwear_kw40z board configuration supports the following hardware features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| SYSTICK   | on-chip    | systick                             |
+-----------+------------+-------------------------------------+
| PINMUX    | on-chip    | pinmux                              |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | gpio                                |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port-polling;                |
|           |            | serial port-interrupt               |
+-----------+------------+-------------------------------------+
| RTT       | on-chip    | console                             |
+-----------+------------+-------------------------------------+
| FLASH     | on-chip    | soc flash                           |
+-----------+------------+-------------------------------------+

The default configuration can be found in the defconfig file:

	``boards/arm/hexiwear_kw40z/hexiwear_kw40z_defconfig``

Other hardware features are not currently supported by the port.

Connections and IOs
===================

The KW40Z SoC has three pairs of pinmux/gpio controllers, but only one is
currently enabled (PORTC/GPIOC) for the hexiwear_kw40z board.

+-------+-----------------+---------------------------+
| Name  | Function        | Usage                     |
+=======+=================+===========================+
| PTC6  | UART0_RX        | UART BT HCI               |
+-------+-----------------+---------------------------+
| PTC7  | UART0_TX        | UART BT HCI               |
+-------+-----------------+---------------------------+

System Clock
============

The KW40Z SoC is configured to use the 32 MHz external oscillator on the board
with the on-chip FLL to generate a 40 MHz system clock.

Serial Port
===========

The KW40Z SoC has one UART, which is used for BT HCI. The console is available
using `Segger RTT`_.

Programming and Debugging
*************************

The Hexiwear docking station includes the :ref:`nxp_opensda` serial and debug
adapter built into the board to provide debugging, flash programming, and
serial communication over USB.

To use the pyOCD tools with OpenSDA, follow the instructions in the
:ref:`nxp_opensda_pyocd` page using the `DAPLink Hexiwear Firmware`_.

To use the Segger J-Link tools with OpenSDA, follow the instructions in the
:ref:`nxp_opensda_jlink` page using the `Segger J-Link OpenSDA V2.1 Firmware`_.

Because `Segger RTT`_ is required for a console to KW40Z, the J-Link tools are
recommended.

Flashing
========

The Segger J-Link firmware does not support command line flashing, therefore
the ``make flash`` build target is not supported.

Debugging
=========

This example uses the :ref:`hello_world` sample with the
:ref:`nxp_opensda_jlink` tools. Use the ``make debug`` build target to build
your Zephyr application, invoke the J-Link GDB server, attach a GDB client, and
program your Zephyr application to flash. It will leave you at a gdb prompt.

.. code-block:: console

   $ cd <zephyr_root_path>
   $ . zephyr-env.sh
   $ cd samples/hello_world/
   $ make BOARD=hexiwear_kw40z DEBUG_SCRIPT=jlink.sh debug


In a second terminal, open telnet:

  .. code-block:: console

     $ telnet localhost 19021
     Trying 127.0.0.1...
     Connected to localhost.
     Escape character is '^]'.
     SEGGER J-Link V6.14b - Real time terminal output
     J-Link OpenSDA 2 compiled Feb 28 2017 19:27:57 V1.0, SN=621000000
     Process: JLinkGDBServer

Continue program execution in GDB, then in the telnet terminal you should see:

  .. code-block:: console

     ***** BOOTING ZEPHYR OS v1.7.99 - BUILD: Apr  6 2017 21:09:52 *****
     Hello World! arm


.. _KW40Z Website:
   http://www.nxp.com/products/microcontrollers-and-processors/arm-processors/kinetis-cortex-m-mcus/w-series-wireless-m0-plus-m4/kinetis-kw40z-2.4-ghz-dual-mode-ble-and-802.15.4-wireless-radio-microcontroller-mcu-based-on-arm-cortex-m0-plus-core:KW40Z

.. _KW40Z Datasheet:
   http://www.nxp.com/assets/documents/data/en/data-sheets/MKW40Z160.pdf

.. _KW40Z Reference Manual:
   http://www.nxp.com/assets/documents/data/en/reference-manuals/MKW40Z160RM.pdf

.. _Segger RTT:
    https://www.segger.com/jlink-rtt.html

.. _DAPLink Hexiwear Firmware:
   https://github.com/MikroElektronika/HEXIWEAR/blob/master/HW/HEXIWEAR_DockingStation/HEXIWEAR_DockingStation_DAPLINK_FW.bin

.. _Segger J-Link OpenSDA V2.1 Firmware:
   https://www.segger.com/downloads/jlink/OpenSDA_V2_1.bin
