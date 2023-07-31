.. zephyr:code-sample:: timeaware-gpio
   :name: Time-aware GPIO
   :relevant-api: tgpio_interface

   Synchronize clocks.

Overview
********

A sample application that can be used with any supported boards. eg. RPL.
prints configuration info to the console. Below functionalities are tested:

1. Periodic pulse generation (an Oscilloscope or loopback may be required)
2. Input pulse timestamping (a pulse generator or loopback may be required)

Building and Running
********************

This application can be built and executed as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/misc/timeaware_gpio
   :board: intel_rpl_crb
   :host-os: unix
   :goals: build


To build for another supported board, change "intel_rpl_crb" to that board's
name (if supported).

Sample Output
=============

.. code-block:: console

   *** Booting Zephyr OS build zephyr-v3.4.0-4166-g52b34a310c67 ***
   [TGPIO] Bind Success
   [TGPIO] Time now: 00000001477ed72f
   [TGPIO] Running rate: 19200000
   [TGPIO] Periodic pulses start at: 0000000148a3cf2f
   [TGPIO] timestamp: 0000000000000000, event count: 0000000000000000
   [TGPIO] timestamp: 0000000148a3cf31, event count: 0000000000000001
   [TGPIO] timestamp: 0000000149c8c731, event count: 0000000000000002
   [TGPIO] timestamp: 000000014aedbf31, event count: 0000000000000003
   [TGPIO] timestamp: 000000014c12b731, event count: 0000000000000004
   [TGPIO] timestamp: 000000014d37af31, event count: 0000000000000005
   [TGPIO] timestamp: 000000014e5ca731, event count: 0000000000000006
   [TGPIO] timestamp: 000000014f819f31, event count: 0000000000000007
   [TGPIO] timestamp: 0000000150a69731, event count: 0000000000000008
   [TGPIO] timestamp: 0000000151cb8f31, event count: 0000000000000009
   [TGPIO] timestamp: 0000000152f08731, event count: 000000000000000a
   [TGPIO] timestamp: 0000000154157f31, event count: 000000000000000b
