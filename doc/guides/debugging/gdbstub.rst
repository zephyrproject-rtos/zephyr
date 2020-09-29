.. _gdbgstub:

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
``gdb_init()`` starts gdbstub service and waits for a GDB
connection. Once a connection is established it is possible to
synchronously interact with Zephyr. Note that currently it is not
possible to asynchronously send commands to the target.

Enable this feature with the :option:`CONFIG_GDBSTUB` option.

Features
********

The following features are supported:

* Add and remove breakpoints
* Continue and step the target
* Print backtrace
* Read or write general registers
* Read or write the memory
