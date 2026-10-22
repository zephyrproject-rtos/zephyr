.. zephyr:board:: t3_gem_o1

Overview
********

The T3 Gemstone O1 is a single-board computer powered by the TI AM67A (J722S) SoC.
T3 Gemstone O1 development board are built from open-source hardware and software components,
and they pair AI-accelerated hardware with a Debian-based GNU/Linux operating system optimized for real-time applications,
making them suitable for manned and unmanned systems, IoT, robotics, and many other fields.

Hardware
********
T3 Gemstone O1 is powered by the TI AM67A (J722S) SoC, which has two domains
(Main, MCU). This document gives an overview of Zephyr running on both Cortex
R5F cores.

The board also features integrated sensors, including the ICM-20948 9-axis motion sensor,
LPS22DF barometric pressure sensor, and HDC2010 temperature and humidity sensor.
Connectivity includes an onboard CAN FD transceiver, two 4-lane MIPI CSI camera interfaces,
and a 4-lane MIPI DSI display interface — one CSI port is multiplexed with DSI,
so either dual CSI or one CSI plus one DSI can be used.
An I2C real-time clock with a battery input preserves system time across power cycles,
while 32 GB of onboard eMMC storage allows the board to run without a microSD card.
Programmable red and green user LEDs are also available.

L1 Memory System
----------------
T3 Gemstone O1 defaults to single-core mode for the R5 subsystem. Changes in
that will impact the L1 memory system configuration.

* 32KB instruction cache
* 32KB data cache
* 64KB tightly-coupled memory (TCM)
  * 32KB TCMA
  * 32KB TCMB

Region Address Translation
--------------------------
The RAT module performs a region based address translation. It translates a
32-bit input address into a 36-bit output address. Any input transaction that
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
The board configuration supports a console UART via the 40-pin GPIO header.
Future versions will also support a console over RPmsg.

.. zephyr:board-supported-hw::

Running Zephyr
**************

The AM67A does not have a separate flash for the R5 core. Because of this
an A53 core has to load the program for the R5 core to the right memory
address, set the PC and start the processor.
This can be done from Linux on the A53 core via remoteproc.

This is the memory mapping from A53 to the memory usable by the R5. Note that
the R5 core always sees its local TCMA at address 0x00000000 and its TCMB0
at address 0x41010000.

The A53 Linux configuration allocates a region in DDR that is shared with
the R5. The amount of the allocation can be changed in the Linux device tree.
Note that T3 Gemstone O1 has 4GB of LPDDR4.

+-------------------+---------------+--------------+--------+
| Region            | Addr from A53 | MAIN R5F     | Size   |
+===================+===============+==============+========+
| ATCM              | 0x0078400000  | 0x0000000000 | 32KB   |
+-------------------+---------------+--------------+--------+
| BTCM              | 0x0078500000  | 0x0041010000 | 32KB   |
+-------------------+---------------+--------------+--------+
| DDR Shared Region | 0x00A2000000  | 0x00A2000000 | 16MB   |
+-------------------+---------------+--------------+--------+

+-------------------+---------------+--------------+--------+
| Region            | Addr from A53 | MCU R5F      | Size   |
+===================+===============+==============+========+
| ATCM              | 0x0079000000  | 0x0000000000 | 32KB   |
+-------------------+---------------+--------------+--------+
| BTCM              | 0x0079020000  | 0x0041010000 | 32KB   |
+-------------------+---------------+--------------+--------+
| DDR Shared Region | 0x00A1000000  | 0x00A1000000 | 16MB   |
+-------------------+---------------+--------------+--------+

Steps to run the image
----------------------
Here is an example for the :zephyr:code-sample:`hello_world` application
targeting the MAIN domain Cortex R5F on T3 Gemstone O1:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: t3_gem_o1/j722s/main_r5f0_0
   :goals: build

For the MCU domain Cortex R5F on T3 Gemstone O1:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: t3_gem_o1/j722s/mcu_r5f0_0
   :goals: build

To load the image:

| Copy Zephyr image to the /lib/firmware/ directory.
| ``cp build/zephyr/zephyr.elf /lib/firmware/``
|
| Ensure the Core is not running.
| ``echo stop > /dev/remoteproc/am67a-{main,mcu}-r5f0_0/state``
|
| Configuring the image name to the remoteproc module.
| ``echo zephyr.elf > /dev/remoteproc/am67a-{main,mcu}-r5f0_0/firmware``
|
| Once the image name is configured, send the start command.
| ``echo start > /dev/remoteproc/am67a-{main,mcu}-r5f0_0/state``

Console
-------
Zephyr on the T3 Gemstone O1 Cortex-R5F uses UART 1 (40-pin GPIO header
pins 8-TX, 10-RX) as console.

Debugging
---------
The board provides an ARM Cortex 10-pin JTAG connector which can be used to
debug the Cortex-R5F cores.

References
**********
* `T3 Gemstone Homepage <https://t3gemstone.org>`_
* `T3 Gemstone O1 documentation <https://docs.t3gemstone.org>`_
* `T3 Gemstone on GitHub <https://github.com/t3gemstone>`_
* `AM67A TRM <https://www.ti.com/lit/zip/sprujb3>`_
