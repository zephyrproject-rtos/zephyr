:orphan:

.. _stlink-v21:

ST-LINK/V2-1
############

Overview
********

ST-LINK/V2-1 is a serial and debug adapter built into all Nucleo and Discovery
boards. It provides a bridge between your computer (or other USB host) and the
embedded target processor, which can be used for debugging, flash programming,
and serial communication, all over a simple USB cable.

SEGGER offers a firmware upgrading the ST-LINK/V2-1 on board on the Nucleo and
Discovery boards. This firmware makes the ST-LINK/V2-1 compatible with J-Link
OB, allowing users to take advantage of most J-Link features like the ultra fast
flash download and debugging speed or the free-to-use GDBServer.

More informations about upgrading ST-LINK/V2-1 to JLink or restore ST-Link/V2-1
firmware please visit: `SEGGER`_

Zephyr supports the following debug tools through ST-LINK/V2-1:

* :ref:`st_ST-LINK/V2-1_stlink`
* :ref:`st_ST-LINK/V2-1_jlink`

.. _st_ST-LINK/V2-1_stlink:

ST-LINK/V2-1
************

ST-Link tool is available by default, flash and debug could be done as follows.

Flashing
========

  .. zephyr-app-commands::
     :zephyr-app: samples/hello_world
     :goals: flash

Debugging
=========

  .. zephyr-app-commands::
     :zephyr-app: samples/hello_world
     :goals: debug


.. _st_ST-LINK/V2-1_jlink:

Segger J-Link
*************

Host Tools and Firmware
=======================

Download and install the `Segger J-Link Software and Documentation Pack`_ to
get the J-Link GDB server for your host computer.

Flashing
========

Use the CMake ``flash`` target with the argument ``STLINK_FW=jlink`` to
build your Zephyr application.

  .. zephyr-app-commands::
     :zephyr-app: samples/hello_world
     :gen-args: -DSTLINK_FW=jlink
     :goals: flash

Debugging
=========

Use the CMake ``debug`` target with the argument ``STLINK_FW=jlink`` to build
your Zephyr application, invoke the J-Link GDB server, attach a GDB client, and
program your Zephyr application to flash. It will leave you at a GDB prompt.

  .. zephyr-app-commands::
     :zephyr-app: samples/hello_world
     :gen-args: -DSTLINK_FW=jlink
     :goals: debug

Console
=======

If you configured your Zephyr application to use a UART console (most boards
enable this by default), open a serial terminal (minicom, putty, etc.) with the
following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

If you configured your Zephyr application to use `Segger RTT`_ console instead,
open telnet:

  .. code-block:: console

     $ telnet localhost 19021
     Trying ::1...
     Trying 127.0.0.1...
     Connected to localhost.
     Escape character is '^]'.
     SEGGER J-Link V6.30f - Real time terminal output
     J-Link STLink V21 compiled Jun 26 2017 10:35:16 V1.0, SN=773895351
     Process: JLinkGDBServerCLExe
     Zephyr Shell, Zephyr version: 1.12.99
     Type 'help' for a list of available commands
     shell>

If you get no RTT output you might need to disable other consoles which conflict
with the RTT one if they are enabled by default in the particular sample or
application you are running, such as disable UART_CONSOLE in menuconfig.


.. _SEGGER:
   https://www.segger.com/products/debug-probes/j-link/models/other-j-links/st-link-on-board/

.. _Segger J-Link Software and Documentation Pack:
   https://www.segger.com/downloads/jlink

.. _Segger RTT:
    https://www.segger.com/jlink-rtt.html
