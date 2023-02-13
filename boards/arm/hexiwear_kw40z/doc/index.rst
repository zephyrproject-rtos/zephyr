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
| ADC       | on-chip    | adc                                 |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port-polling;                |
|           |            | serial port-interrupt               |
+-----------+------------+-------------------------------------+
| RTT       | on-chip    | console                             |
+-----------+------------+-------------------------------------+
| FLASH     | on-chip    | soc flash                           |
+-----------+------------+-------------------------------------+
| TRNG      | on-chip    | entropy                             |
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
| PTB1  | ADC             | ADC0 channel 1            |
+-------+-----------------+---------------------------+
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

The KW40Z SoC has one UART, which is used for BT HCI. There is no UART
available for a console.

Programming and Debugging
*************************

Build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Configuring a Debug Probe
=========================

A debug probe is used for both flashing and debugging the board. This board is
configured by default to use the :ref:`opensda-daplink-onboard-debug-probe`,
but because Segger RTT is required for a console, you must reconfigure the
board for one of the following debug probes instead.

:ref:`opensda-jlink-onboard-debug-probe`
----------------------------------------

Install the :ref:`jlink-debug-host-tools` and make sure they are in your search
path.

Follow the instructions in :ref:`opensda-jlink-onboard-debug-probe` to program
the `OpenSDA J-Link Generic Firmware for V2.1 Bootloader`_. Check that switches
SW1 and SW2 are **off**, and SW3 and SW4 are **on**  to ensure KW40Z SWD signals
are connected to the OpenSDA microcontroller.

Configuring a Console
=====================

The console is available using `Segger RTT`_.

Connect a USB cable from your PC to CN1.

Once you have started a debug session, run telnet:

.. code-block:: console

    $ telnet localhost 19021
    Trying 127.0.0.1...
    Connected to localhost.
    Escape character is '^]'.
    SEGGER J-Link V6.44 - Real time terminal output
    J-Link OpenSDA 2 compiled Feb 28 2017 19:27:57 V1.0, SN=621000000
    Process: JLinkGDBServerCLExe

Flashing
========

Here is an example for the :ref:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: hexiwear_kw40z
   :goals: flash

The Segger RTT console is only available during a debug session. Use ``attach``
to start one:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: hexiwear_kw40z
   :goals: attach

Run telnet as shown earlier, and you should see the following message in the
terminal:

.. code-block:: console

   ***** Booting Zephyr OS v1.14.0-rc1 *****
   Hello World! hexiwear_kw40z

Debugging
=========

Here is an example for the :ref:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: hexiwear_kw40z
   :goals: debug

Run telnet as shown earlier, step through the application in your debugger, and
you should see the following message in the terminal:

.. code-block:: console

   ***** Booting Zephyr OS v1.14.0-rc1 *****
   Hello World! hexiwear_kw40z

.. _KW40Z Website:
   https://www.nxp.com/products/processors-and-microcontrollers/arm-based-processors-and-mcus/kinetis-cortex-m-mcus/w-serieswireless-conn.m0-plus-m4/kinetis-kw40z-2.4-ghz-dual-mode-ble-and-802.15.4-wireless-radio-microcontroller-mcu-based-on-arm-cortex-m0-plus-core:KW40Z

.. _KW40Z Datasheet:
   https://www.nxp.com/docs/en/data-sheet/MKW40Z160.pdf

.. _KW40Z Reference Manual:
   https://www.nxp.com/webapp/Download?colCode=MKW40Z160RM

.. _Segger RTT:
   https://www.segger.com/products/debug-probes/j-link/technology/about-real-time-transfer/

.. _OpenSDA J-Link Generic Firmware for V2.1 Bootloader:
   https://www.segger.com/downloads/jlink/OpenSDA_V2_1
