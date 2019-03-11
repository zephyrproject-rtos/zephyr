.. _acrn:

ACRN UOS (Guest Operating System)
#################################

Overview
********

This baseline configuration is intended to be used as a starting point for
custom ACRN UOS configurations. It supports the following devices:

* I/O APIC
* local APIC timer
* NS16550 UARTs

Serial Ports
------------

The serial ports are assumed present at traditional COM1: and COM2: I/O-space
addresses (based at 0x3f8 and 0x2f8, respectively). Only polled operation is
supported in this baseline configuration, as IRQ assignments under ACRN are
configurable (and frequently non-standard). Interrupt-driven and MMIO operation
are both theoretically possible, however.

Running
*******

The output from the build process is a zephyr.bin file, which is a raw image
which is intended to be loaded at 0x100000 (1MB) by the acrn-dm loader.

Zephyr can then be launched from the ACRN SOS with a command line like:

.. code-block:: none

    acrn-dm -A -m $mem_size -c $1 -s 0:0,hostbridge -s 1:0,lpc -l com1,stdio \
        --zephyr ./zephyr.bin \
        zephyr_vm

.. note::

Currently support for the ACRN devicemanager --zephyr option is not upstream,
so the above command will not work on a standard ACRN installation.

