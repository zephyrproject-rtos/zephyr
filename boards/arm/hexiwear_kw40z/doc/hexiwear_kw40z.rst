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

The Hexiwear docking station includes an `OpenSDA`_ serial and debug adaptor
built into the board. Different firmware options are available for the adaptor
including Segger J-Link and DAPLink. Because `Segger RTT`_ is required for a
console, the `Segger J-Link OpenSDA`_ firmware is recommended.

Segger J-Link
=============

Download and install the `Segger J-Link Software and Documentation Pack`_ to
get the JLinkGDBServer for your host computer.

Put the OpenSDA adapter into bootloader mode by holding the reset button while
you power on the board. A USB mass storage device called MAINTENANCE will
enumerate. Copy the `Segger J-Link OpenSDA V2.1 Bootloader`_ to the MAINTENANCE
drive. Power cycle the board, this time without holding the reset button.

Start the GDB Server:

  .. code-block:: console

     $ JLinkGDBServer -if swd -device MKW40Z160xxx4

     SEGGER J-Link GDB Server V6.14b Command Line Version

     JLinkARM.dll V6.14b (DLL compiled Mar  9 2017 08:48:20)

     -----GDB Server start settings-----
     GDBInit file:                  none
     GDB Server Listening port:     2331
     SWO raw output listening port: 2332
     Terminal I/O port:             2333
     Accept remote connection:      yes
     Generate logfile:              off
     Verify download:               off
     Init regs on start:            off
     Silent mode:                   off
     Single run mode:               off
     Target connection timeout:     0 ms
     ------J-Link related settings------
     J-Link Host interface:         USB
     J-Link script:                 none
     J-Link settings file:          none
     ------Target related settings------
     Target device:                 MKW40Z160xxx4
     Target interface:              SWD
     Target interface speed:        1000kHz
     Target endian:                 little

     Connecting to J-Link...
     J-Link is connected.
     Firmware: J-Link OpenSDA 2 compiled Feb 28 2017 19:27:57
     Hardware: V1.00
     S/N: 621000000
     Checking target voltage...
     Target voltage: 3.30 V
     Listening on TCP/IP port 2331
     Connecting to target...Connected to target
     Waiting for GDB connection...

In a second terminal, open telnet:

  .. code-block:: console

     $ telnet localhost 19021
     Trying 127.0.0.1...
     Connected to localhost.
     Escape character is '^]'.
     SEGGER J-Link V6.14b - Real time terminal output
     J-Link OpenSDA 2 compiled Feb 28 2017 19:27:57 V1.0, SN=621000000
     Process: JLinkGDBServer

In a third terminal, build the Zephyr kernel and application:

   .. code-block:: console

      $ cd $ZEPHYR_BASE
      $ . zephyr-env.sh
      $ cd $ZEPHYR_BASE/samples/hello_world/
      $ make BOARD=hexiwear_kw40z

Start the GDB client:

  .. code-block:: console

     $ arm-zephyr-eabi-gdb outdir/hexiwear_kw40z/zephyr.elf

Connect to the GDB server:

  .. code-block:: console

     (gdb) target remote localhost:2331
     (gdb) load
     (gdb) monitor reset
     (gdb) continue

Back in the second terminal where you opened telnet, you should see:

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

.. _OpenSDA:
   http://www.nxp.com/products/software-and-tools/hardware-development-tools/startertrak-development-boards/opensda-serial-and-debug-adapter:OPENSDA

.. _Segger J-Link OpenSDA:
   https://www.segger.com/opensda.html

.. _Segger J-Link OpenSDA V2.1 Bootloader:
   https://www.segger.com/downloads/jlink/OpenSDA_V2_1.bin

.. _Segger J-Link Software and Documentation Pack:
   https://www.segger.com/downloads/jlink
