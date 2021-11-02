.. _gdbstub:

GDB stub
########

Overview
********

The gdbstub feature provides an implementation of the GDB Remote
Serial Protocol (RSP) that allows you to remotely debug Zephyr
using GDB.

The protocol supports different connection types: serial, UDP/IP and
TCP/IP. Zephyr currently supports only serial device communication.

The GDB program acts as the client while Zephyr acts as the
server. When this feature is enabled, Zephyr stops its execution after
:c:func:`gdb_init` starts gdbstub service and waits for a GDB
connection. Once a connection is established it is possible to
synchronously interact with Zephyr. Note that currently it is not
possible to asynchronously send commands to the target.

Features
********

The following features are supported:

* Add and remove breakpoints
* Continue and step the target
* Print backtrace
* Read or write general registers
* Read or write the memory

Enabling GDB Stub
*****************

GDB stub can be enabled with the :kconfig:`CONFIG_GDBSTUB` option.

Using Serial Backend
====================

The serial backend for GDB stub can be enabled with
the :kconfig:`CONFIG_GDBSTUB_SERIAL_BACKEND` option.

Since serial backend utilizes UART devices to send and receive GDB commands,

* If there are spare UART devices on the board, set
  :kconfig:`CONFIG_GDBSTUB_SERIAL_BACKEND_NAME` to the spare UART device
  so that :c:func:`printk` and log messages are not being printed to
  the same UART device used for GDB.

* For boards with only one UART device, :c:func:`printk` and logging
  must be disabled if they are also using the same UART device for output.
  GDB related messages may interleave with log messages which may have
  unintended consequences. Usually this can be done by disabling
  :kconfig:`CONFIG_PRINTK` and :kconfig:`CONFIG_LOG`.

Debugging
*********

Using Serial Backend
====================

#. Build with GDB stub and serial backend enabled.

#. Flash built image onto board and reset the board.

   * Execution should now be paused at :c:func:`gdb_init`.

#. Execute GDB on development machine and connect to the GDB stub.

   .. code-block:: bash

      target remote <serial device>

   For example,

   .. code-block:: bash

      target remote /dev/ttyUSB1

#. GDB commands can be used to start debugging.
