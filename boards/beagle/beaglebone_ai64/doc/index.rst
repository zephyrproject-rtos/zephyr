.. _beaglebone_ai64:

BeagleBone AI-64
################

Overview
********

BeagleBone AI-64 is a computational platform powered by TI J721E SoC, which is
targeted for automotive applications.

.. figure:: assets/bbai_64.webp
   :align: center
   :width: 600px
   :alt: BeagleBoard.org BeagleBone AI-64

Hardware
********

The BeagleBone AI-64 is powered by TI J721E SoC, which has three domains (Main,
MCU, WKUP). This document gives overview of Zephyr running on Cortex R5 in the
Main domain.

L1 Memory System
----------------

* 16 KB instruction cache.
* 16 KB data cache.
* 64 KB TCM.

Region Address Translation
--------------------------

The RAT module performs a region based address translation. It translates a
32-bit input address into a 48-bit output address. Any input transaction that
starts inside of a programmed region will have its address translated, if the
region is enabled.

VIM Interrupt Controller
------------------------

The VIM aggregates device interrupts and sends them to the R5F CPU(s). The VIM
module supports 512 interrupt inputs per R5F core. Each interrupt can be either
a level or a pulse (both active-high). The VIM has two interrupt outputs per core
IRQ and FIQ.

Supported Features
******************

The board configuration supports,

+-----------+------------+-----------------------+
| Interface | Controller | Driver/Component      |
+===========+============+=======================+
| UART      | on-chip    | serial port-polling   |
|           |            | serial port-interrupt |
+-----------+------------+-----------------------+

Other hardwares features are currently not supported.

Running Zephyr
**************

The J721E does not have a separate flash for the R5 cores. Because of this
the A72 core has to load the program for the R5 cores to the right memory
address, set the PC and start the processor.
This can be done from Linux on the A72 core via remoteproc.

This is the memory mapping from A72 to the memory usable by the R5. Note that
the R5 cores always see their local ATCM at address 0x00000000 and their BTCM
at address 0x41010000.

+------------+--------------+--------------+--------------+--------------+--------+
| Region     | R5FSS0 Core0 | R5FSS0 Core1 | R5FSS1 Core0 | R5FSS1 Core1 | Size   |
+============+==============+==============+==============+==============+========+
| ATCM       | 0x05c00000   | 0x05d00000   | 0x05e00000   | 0x05f00000   | 32KB   |
+------------+--------------+--------------+--------------+--------------+--------+
| BTCM       | 0x05c10000   | 0x05d10000   | 0x05e10000   | 0x05f00000   | 32KB   |
+------------+--------------+--------------+--------------+--------------+--------+
| DDR0       | 0xA2000000   | 0xA3000000   | 0xA4000000   | 0xA5000000   | 1MB    |
+------------+--------------+--------------+--------------+--------------+--------+
| DDR1       | 0xA2100000   | 0xA3000000   | 0xA4100000   | 0xA5000000   | 15MB   |
+------------+--------------+--------------+--------------+--------------+--------+

Steps to run the image
----------------------

The example shows how to load an image on Cortex R5FSS0_CORE0 on J721e.

| Copy Zephyr image to the /lib/firmware/ directory.
| ``cp build/zephyr/zephyr.elf /lib/firmware/``
|
| Ensure the Core is not running.
| ``echo stop > /sys/class/remoteproc/remoteproc18/state``
|
| Configuring the image name to the remoteproc module.
| ``echo zephyr.elf > /sys/class/remoteproc/remoteproc18/firmware``
|
| Once the image name is configured, send the start command.
| ``echo start > /sys/class/remoteproc/remoteproc18/state``

Console
-------

The Zephyr on BeagleBone AI-64 J721E Cortex R5 uses UART 2 (Rx p8.22, Tx p8.34)
as console.

References
**********

* `BeagleBone AI-64 Homepage <https://beagleboard.org/ai-64>`_
* `J721E TRM <https://www.ti.com/lit/zip/spruil1>`_
