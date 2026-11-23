.. zephyr:board:: beaglebone_ai64

Overview
********

BeagleBone AI-64 is a computational platform powered by TI J721E SoC, which is
targeted for automotive applications.

Hardware
********

BeagleBone AI-64 is powered by TI J721E SoC, which has three domains (MAIN,
MCU, WKUP). This document covers Zephyr on the MAIN and MCU domain Cortex-R5F
cores.

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

.. zephyr:board-supported-hw::

Running Zephyr
**************

The J721E does not have a separate flash for the R5 cores. Because of this
the A72 core has to load the program for the R5 cores to the right memory
address, set the PC and start the processor.
This can be done from Linux on the A72 core via remoteproc.

By default the R5's Memory Protection Unit (MPU) only allows for execution of
instructions in the ATCM/BTCM. There is also a couple regions of DRAM memory
carved out for each R5 by Linux. These can be used for IPC (DDR0) and for
data (DDR1). DDR1 can also be used for executable regions after programming
the MPU.

This is the memory mapping from A72 to the memory usable by the R5. Note that
the R5 cores always see their local ATCM at address 0x00000000 and their BTCM
at address 0x41010000 (TRM Table 2-5). The ATCM/BTCM slave ports used by
remoteproc are in the table below (Linux ``k3-j721e-main.dtsi``). The DDR
regions follow ``k3-j721e-ti-ipc-firmware.dtsi`` when that file is included in
the **running** Linux device tree; verify with
``ls /proc/device-tree/reserved-memory/`` on the board.

MAIN domain (Linux ``k3-j721e-main.dtsi``):

+------------+--------------+--------------+--------------+--------------+--------+
| Region     | R5FSS0 Core0 | R5FSS0 Core1 | R5FSS1 Core0 | R5FSS1 Core1 | Size   |
+============+==============+==============+==============+==============+========+
| ATCM       | 0x05c00000   | 0x05d00000   | 0x05e00000   | 0x05f00000   | 32KB   |
+------------+--------------+--------------+--------------+--------------+--------+
| BTCM       | 0x05c10000   | 0x05d10000   | 0x05e10000   | 0x05f10000   | 32KB   |
+------------+--------------+--------------+--------------+--------------+--------+
| DDR0 (IPC) | 0xA2000000   | 0xA3000000   | 0xA4000000   | 0xA5000000   | 1MB    |
+------------+--------------+--------------+--------------+--------------+--------+
| RSC table  | 0xA2100000   | 0xA3100000   | 0xA4100000   | 0xA5100000   | 1MB    |
+------------+--------------+--------------+--------------+--------------+--------+
| DDR1 (DRAM)| 0xA2200000   | 0xA3200000   | 0xA4200000   | 0xA5200000   | 14MB   |
+------------+--------------+--------------+--------------+--------------+--------+

MCU domain (Linux ``k3-j721e-mcu-wakeup.dtsi``):

+------------+------------------+------------------+--------+
| Region     | MCU R5FSS0 Core0 | MCU R5FSS0 Core1 | Size   |
+============+==================+==================+========+
| ATCM       | 0x41000000       | 0x41400000       | 32KB   |
+------------+------------------+------------------+--------+
| BTCM       | 0x41010000       | 0x41410000       | 32KB   |
+------------+------------------+------------------+--------+
| DDR0 (IPC) | 0xA0000000       | 0xA1000000       | 1MB    |
+------------+------------------+------------------+--------+
| RSC table  | 0xA0100000       | 0xA1100000       | 1MB    |
+------------+------------------+------------------+--------+
| DDR1 (DRAM)| 0xA0200000       | 0xA1200000       | 14MB   |
+------------+------------------+------------------+--------+

U-Boot typically starts MCU R5FSS0 Core0 before Linux, so that core shows up
as remoteproc **IPC-only / attached**. Replacing its firmware from Linux
requires the core to be in remoteproc mode (not late-attached), or loading
Zephyr as ``j7-mcu-r5f0_0-fw`` from the bootloader. Both MCU cores are in
lockstep on the default BeagleBone AI-64 Linux DT (``ti,cluster-mode = 1``),
so Core1 is not a separate remoteproc device until the cluster is switched
to split mode.

Remoteproc nodes
----------------

Each MAIN R5 core is started from Linux via remoteproc. Typical device nodes and
firmware names (``k3-j721e-main.dtsi``):

+---------------------------------------+------------------------------+------------------------+
| Linux remoteproc node                 | ``firmware-name``            | Zephyr board target    |
+=======================================+==============================+========================+
| ``/dev/remoteproc/j7-main-r5f0_0``    | ``j7-main-r5f0_0-fw``        | ``main_r5f0_0``        |
+---------------------------------------+------------------------------+------------------------+
| ``/dev/remoteproc/j7-main-r5f0_1``    | ``j7-main-r5f0_1-fw``        | ``main_r5f0_1``        |
+---------------------------------------+------------------------------+------------------------+
| ``/dev/remoteproc/j7-main-r5f1_0``    | ``j7-main-r5f1_0-fw``        | ``main_r5f1_0``        |
+---------------------------------------+------------------------------+------------------------+
| ``/dev/remoteproc/j7-main-r5f1_1``    | ``j7-main-r5f1_1-fw``        | ``main_r5f1_1``        |
+---------------------------------------+------------------------------+------------------------+
| ``41000000.r5f`` (``remoteproc12``)   | ``j7-mcu-r5f0_0-fw``         | ``mcu_r5f0_0``         |
+---------------------------------------+------------------------------+------------------------+
| ``41400000.r5f`` (split mode)         | ``j7-mcu-r5f0_1-fw``         | ``mcu_r5f0_1``         |
+---------------------------------------+------------------------------+------------------------+

Steps to build and run an image
-------------------------------

Here is an example for the :zephyr:code-sample:`hello_world` application
targeting one of the Cortex R5F on BeagleBone AI-64:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: beaglebone_ai64/j721e/main_r5f0_0
   :goals: build

To load the image (example for R5FSS0 Core0):

| Copy Zephyr image to the /lib/firmware/ directory.
| ``cp build/zephyr/zephyr.elf /lib/firmware/``
|
| Ensure the core is not running.
| ``echo stop > /dev/remoteproc/j7-main-r5f0_0/state``
|
| Configuring the image name to the remoteproc module.
| ``echo zephyr.elf > /dev/remoteproc/j7-main-r5f0_0/firmware``
|
| Once the image name is configured, send the start command.
| ``echo start > /dev/remoteproc/j7-main-r5f0_0/state``

For ``main_r5f0_1``, ``main_r5f1_0``, ``main_r5f1_1``, and the MCU targets,
use the matching remoteproc path from the table above. MCU Core0 is often
already attached; ``echo start`` only works after the core is ``offline``.

Console
-------

``main_r5f0_0`` uses UART2 (Rx P8.22, Tx P8.34) as ``zephyr,console`` (TRM
Table 2-1 UART2 at ``0x02820000``, VIM 160 in TRM Table 9-47).

The other MAIN R5 targets and both MCU R5 targets can be configured with an
RPMsg UART console via overlays. On Linux, enable the RPMsg TTY driver
(``rpmsg_tty``) and look for ``/dev/ttyRPMSG*`` (bind order depends on which
cores are running).

References
**********

* `BeagleBone AI-64 Homepage <https://www.beagleboard.org/boards/beaglebone-ai-64>`_
* `J721E TRM (SPRUIL1D) <https://www.ti.com/lit/zip/spruil1>`_ — Table 2-1 (MAIN
  memory map), Table 2-5 (R5 TCM/VIM), Tables 9-47–9-50 (R5F VIM interrupt IDs),
  Table 7-2/7-4 (NAVSS mailbox)
* `PSDK RTOS J721E memory map <https://software-dl.ti.com/jacinto7/esd/processor-sdk-rtos-jacinto7/latest/exports/docs/psdk_rtos/docs/user_guide/developer_notes_memory_map.html>`_
* `PSDK Linux IPC J721E <https://software-dl.ti.com/jacinto7/esd/processor-sdk-linux-jacinto7/11_02_01_03/exports/docs/linux/Foundational_Components_IPC_J721E.html>`_
* Linux ``k3-j721e-main.dtsi`` and ``k3-j721e-ti-ipc-firmware.dtsi``
