.. zephyr:board:: imx8mn_evk

Overview
********

i.MX8M Nano LPDDR4 EVK board is based on NXP i.MX8M Nano applications
processor, composed of a quad Cortex®-A53 cluster and a single Cortex®-M7 core.
Zephyr OS is ported to run on the Cortex®-A53 core and on the Cortex®-M7 core.

- Board features:

  - RAM: 2GB LPDDR4
  - Storage:

    - SanDisk 16GB eMMC5.1
    - Micron 32MB QSPI NOR
    - microSD Socket
  - Wireless:

    - WiFi: 2.4/5GHz IEEE 802.11b/g/n
    - Bluetooth: v4.1
  - USB:

    - OTG - 2x type C
  - Ethernet
  - PCI-E M.2
  - Connectors:

    - 40-Pin Dual Row Header
  - LEDs:

    - 1x Power status LED
    - 1x UART LED
  - Debug

    - JTAG 20-pin connector
    - MicroUSB for UART debug, two COM ports for A53 and M7

More information about the board can be found at the
`NXP website`_.

Supported Features
==================

.. zephyr:board-supported-hw::

Devices
========
System Clock
------------

This board configuration uses a system clock frequency of 8 MHz.

The M7 Core is configured to run at a 600 MHz clock speed.

Serial Port
-----------

This board configuration uses a single serial communication channel with the
CPU's UART4.

GPIO
----

The M7 board configuration enables GPIO3 and maps the status LED
(GPIO3_IO16) as ``led0``, so the :zephyr:code-sample:`blinky` sample runs on
the M7. Linux drives the same LED through its ``leds-gpio`` driver, so the two
sides can disagree on its state.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

There are multiple methods to program and debug Zephyr on the A53 core:

Option 1. Boot Zephyr by Using JLink Runner
===========================================

The default runner for the board is JLink, connect the EVK board's JTAG connector to
the host computer using a J-Link debugger, power up the board and stop the board at
U-Boot command line.

Then use "west flash" or "west debug" command to load the zephyr.bin
image from the host computer and start the Zephyr application on A53 core0.

Flash and Run
-------------

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :host-os: unix
   :board: imx8mn_evk/mimx8mn6/a53
   :goals: flash

Then the following log could be found on UART4 console:

.. code-block:: console

    *** Booting Zephyr OS build v4.1.0-3063-g38519ca2c028 ***
    Hello World! imx8mn_evk/mimx8mn6/a53

Debug
-----

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :host-os: unix
   :board: imx8mn_evk/mimx8mn6/a53
   :goals: debug

Option 2. Boot Zephyr by Using U-Boot Command
=============================================

U-Boot "cpu" command is used to load and kick Zephyr to Cortex-A secondary Core, Currently
it has been supported in latest U-Boot version by `patch serials`_.

.. _patch serials:
   https://patchwork.ozlabs.org/project/uboot/list/?series=417536&archive=both&state=*

Step 1: Download Zephyr Image into DDR Memory
---------------------------------------------

Firstly need to download Zephyr binary image into DDR memory, it can use tftp:

.. code-block:: console

    tftp 0x93c00000 zephyr.bin

Or copy the Zephyr image ``zephyr.bin`` SD card and plug the card into the board, for example
if copy to the FAT partition of the SD card, use the following U-Boot command to load the image
into DDR memory (assuming the SD card is dev 1, fat partition ID is 1, they could be changed
based on actual setup):

.. code-block:: console

    fatload mmc 1:1 0x93c00000 zephyr.bin;

Step 2: Boot Zephyr
-------------------

Then use the following command to boot Zephyr on the core0:

.. code-block:: console

    dcache off; icache flush; go 0x93c00000;

Or use "cpu" command to boot from secondary Core, for example Core1:

.. code-block:: console

    dcache flush; icache flush; cpu 1 release 0x93c00000

Use this configuration to run basic Zephyr applications and kernel tests,
for example, with the :zephyr:code-sample:`synchronization` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: imx8mn_evk/mimx8mn6/a53
   :goals: build

This will build an image with the synchronization sample app, boot it and
display the following ram console output:

.. code-block:: console

    *** Booting Zephyr OS build v4.1.0-3063-g38519ca2c028 ***
    thread_a: Hello World from cpu 0 on mimx8mn_evk!
    thread_b: Hello World from cpu 0 on mimx8mn_evk!
    thread_a: Hello World from cpu 0 on mimx8mn_evk!
    thread_b: Hello World from cpu 0 on mimx8mn_evk!
    thread_a: Hello World from cpu 0 on mimx8mn_evk!

Programming and Debugging (M7)
******************************

The MIMX8MN EVK board doesn't have QSPI flash for the M7, and it needs
to be started by the A53 core. The A53 core is responsible to load the M7 binary
application into the RAM, put the M7 in reset, set the M7 Program Counter and
Stack Pointer, and get the M7 out of reset. The A53 can perform these steps at
bootloader level or after the Linux system has booted.

The M7 can use up to 3 different RAMs (currently, only the ITCM configuration
is supported). These are the memory mapping for A53 and M7:

+------------+-------------------------+------------------------+-----------------------+----------------------+
| Region     | Cortex-A53              | Cortex-M7 (System Bus) | Cortex-M7 (Code Bus)  | Size                 |
+============+=========================+========================+=======================+======================+
| OCRAM      | 0x00900000-0x0097FFFF   | 0x20200000-0x2027FFFF  | 0x00900000-0x0097FFFF | 512KB                |
+------------+-------------------------+------------------------+-----------------------+----------------------+
| DTCM       | 0x00800000-0x0081FFFF   | 0x20000000-0x2001FFFF  |                       | 128KB                |
+------------+-------------------------+------------------------+-----------------------+----------------------+
| ITCM       | 0x007E0000-0x007FFFFF   |                        | 0x00000000-0x0001FFFF | 128KB                |
+------------+-------------------------+------------------------+-----------------------+----------------------+
| OCRAM_S    | 0x00180000-0x00187FFF   | 0x20180000-0x20187FFF  | 0x00180000-0x00187FFF | 32KB                 |
+------------+-------------------------+------------------------+-----------------------+----------------------+
| DDR        | 0x40000000-0xFFFFFFFF   | 0x40000000-0xBFFFFFFF  | 0x10000000-0x1FFDFFFF | 2GB                  |
+------------+-------------------------+------------------------+-----------------------+----------------------+

For more information about memory mapping see the
`i.MX 8M Applications Processor Reference Manual`_ (sections 2.2 and 2.3).

At compilation time you have to choose which RAM will be used. This
configuration is done based on board name (imx8mn_evk/mimx8mn6/m7 for ITCM).

Load and Run M7 Zephyr Image by using Linux remoteproc
======================================================

Prepare device tree:

The device tree must include CM7 dts node with compatible string
"fsl,imx8mn-cm7-mmio", with which the imx_rproc driver releases the core itself
through the SRC and IOMUXC_GPR registers. The M7 runs from SYSTEM PLL3;
parenting the M7 core clock to it keeps Linux from gating that PLL as unused.

.. code-block:: devicetree

   imx8mn-cm7 {
            compatible = "fsl,imx8mn-cm7-mmio";
            clocks = <&clk IMX8MN_CLK_M7_CORE>;
            assigned-clocks = <&clk IMX8MN_CLK_M7_CORE>;
            assigned-clock-parents = <&clk IMX8MN_SYS_PLL3_OUT>;
            syscon = <&src>;
            fsl,iomuxc-gpr = <&gpr>;
   };

Then build the :zephyr:code-sample:`hello_world` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :host-os: unix
   :board: imx8mn_evk/mimx8mn6/m7
   :goals: build

Copy ``zephyr.elf`` to ``/lib/firmware`` on the board and boot it by using
remoteproc:

.. code-block:: console

   root@imx8mn-evk:~# echo zephyr.elf > /sys/class/remoteproc/remoteproc0/firmware
   root@imx8mn-evk:~# echo start > /sys/class/remoteproc/remoteproc0/state
   [   39.195651] remoteproc remoteproc0: powering up imx-rproc
   [   39.203345] remoteproc remoteproc0: Booting fw image zephyr.elf, size 521688
   [   39.203388] remoteproc remoteproc0: No resource table in elf
   [   39.711380] remoteproc remoteproc0: remote processor imx-rproc is now up

The M7 console is UART4, the second COM port of the EVK's debug USB connector
(115200 8N1). You should see the following message in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build v4.5.0-1461-ga8f2b5764473 ***
   Hello World! imx8mn_evk/mimx8mn6/m7

.. include:: ../../common/board-footer.rst.inc

.. _NXP website:
   https://www.nxp.com/design/development-boards/i-mx-evaluation-and-development-boards/evaluation-kit-for-the-i-mx-8m-nano-applications-processor:8MNANOD4-EVK

.. _i.MX 8M Applications Processor Reference Manual:
   https://www.nxp.com/webapp/Download?colCode=IMX8MNRM
