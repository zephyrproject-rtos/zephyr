.. zephyr:board:: imx8mp_evk

Overview
********

i.MX8M Plus LPDDR4 EVK board is based on NXP i.MX8M Plus applications
processor, composed of a quad Cortex®-A53 cluster and a single Cortex®-M7 core.
Zephyr OS is ported to run on the Cortex®-A53 core.

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
    - MicroUSB for UART debug, two COM ports for A53 and M4

More information about the board can be found at the
`NXP website`_.

Supported Features
==================

The Zephyr mimx8mp_evk_a53 board configuration supports the following hardware
features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| GIC-v3    | on-chip    | interrupt controller                |
+-----------+------------+-------------------------------------+
| ARM TIMER | on-chip    | system clock                        |
+-----------+------------+-------------------------------------+
| CLOCK     | on-chip    | clock_control                       |
+-----------+------------+-------------------------------------+
| PINMUX    | on-chip    | pinmux                              |
+-----------+------------+-------------------------------------+
| RDC       | on-chip    | Resource Domain Controller          |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port                         |
+-----------+------------+-------------------------------------+
| ENET      | on-chip    | ethernet port                       |
+-----------+------------+-------------------------------------+

The Zephyr mimx8mp_evk_m7 board configuration supports the following hardware
features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| SYSTICK   | on-chip    | systick                             |
+-----------+------------+-------------------------------------+
| CLOCK     | on-chip    | clock_control                       |
+-----------+------------+-------------------------------------+
| PINMUX    | on-chip    | pinmux                              |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port-polling;                |
|           |            | serial port-interrupt               |
+-----------+------------+-------------------------------------+

Devices
========
System Clock
------------

This board configuration uses a system clock frequency of 8 MHz.

The M7 Core is configured to run at a 800 MHz clock speed.

Serial Port
-----------

This board configuration uses a single serial communication channel with the
CPU's UART4.

Programming and Debugging (A53)
*******************************

U-Boot "cpu" command is used to load and kick Zephyr to Cortex-A secondary Core, Currently
it is supported in : `Real-Time Edge U-Boot`_ (use the branch "uboot_vxxxx.xx-y.y.y,
xxxx.xx is uboot version and y.y.y is Real-Time Edge Software version, for example
"uboot_v2023.04-2.9.0" branch is U-Boot v2023.04 used in Real-Time Edge Software release
v2.9.0), and pre-build images and user guide can be found at `Real-Time Edge Software`_.

.. _Real-Time Edge U-Boot:
   https://github.com/nxp-real-time-edge-sw/real-time-edge-uboot
.. _Real-Time Edge Software:
   https://www.nxp.com/rtedge

Copy the compiled ``zephyr.bin`` to the first FAT partition of the SD card and
plug the SD card into the board. Power it up and stop the u-boot execution at
prompt.

Use U-Boot to load and kick zephyr.bin to Cortex-A53 Core0:

.. code-block:: console

    fatload mmc 1:1 0xc0000000 zephyr.bin; dcache flush; icache flush; go 0xc0000000

Or kick zephyr.bin to the other Cortex-A53 Core, for example Core2:

.. code-block:: console

    fatload mmc 1:1 0xc0000000 zephyr.bin; dcache flush; icache flush; cpu 2 release 0xc0000000

Use this configuration to run basic Zephyr applications and kernel tests,
for example, with the :zephyr:code-sample:`synchronization` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: imx8mp_evk/mimx8ml8/a53
   :goals: run

This will build an image with the synchronization sample app, boot it and
display the following console output:

.. code-block:: console

    *** Booting Zephyr OS build zephyr-v3.1.0-3575-g44dd713bd883  ***
    thread_a: Hello World from cpu 0 on mimx8mp_evk_a53!
    thread_b: Hello World from cpu 0 on mimx8mp_evk_a53!
    thread_a: Hello World from cpu 0 on mimx8mp_evk_a53!
    thread_b: Hello World from cpu 0 on mimx8mp_evk_a53!
    thread_a: Hello World from cpu 0 on mimx8mp_evk_a53!

Use Jailhouse hypervisor, after root cell linux is up:

.. code-block:: console

    #jailhouse enable imx8mp.cell
    #jailhouse cell create imx8mp-zephyr.cell
    #jailhouse cell load 1 zephyr.bin -a 0xc0000000
    #jailhouse cell start 1

Programming and Debugging (M7)
******************************

The MIMX8MP EVK board doesn't have QSPI flash for the M7, and it needs
to be started by the A53 core. The A53 core is responsible to load the M7 binary
application into the RAM, put the M7 in reset, set the M7 Program Counter and
Stack Pointer, and get the M7 out of reset. The A53 can perform these steps at
bootloader level or after the Linux system has booted.

The M7 can use up to 3 different RAMs (currently, only two configurations are
supported: ITCM and DDR). These are the memory mapping for A53 and M7:

+------------+-------------------------+------------------------+-----------------------+----------------------+
| Region     | Cortex-A53              | Cortex-M7 (System Bus) | Cortex-M7 (Code Bus)  | Size                 |
+============+=========================+========================+=======================+======================+
| OCRAM      | 0x00900000-0x0098FFFF   | 0x20200000-0x2028FFFF  | 0x00900000-0x0098FFFF | 576KB                |
+------------+-------------------------+------------------------+-----------------------+----------------------+
| DTCM       | 0x00800000-0x0081FFFF   | 0x20000000-0x2001FFFF  |                       | 128KB                |
+------------+-------------------------+------------------------+-----------------------+----------------------+
| ITCM       | 0x007E0000-0x007FFFFF   |                        | 0x00000000-0x0001FFFF | 128KB                |
+------------+-------------------------+------------------------+-----------------------+----------------------+
| OCRAM_S    | 0x00180000-0x00188FFF   | 0x20180000-0x20188FFF  | 0x00180000-0x00188FFF | 36KB                 |
+------------+-------------------------+------------------------+-----------------------+----------------------+
| DDR        | 0x80000000-0x803FFFFF   | 0x80200000-0x803FFFFF  | 0x80000000-0x801FFFFF | 2MB                  |
+------------+-------------------------+------------------------+-----------------------+----------------------+

For more information about memory mapping see the
`i.MX 8M Applications Processor Reference Manual`_  (section 2.1 to 2.3)

At compilation time you have to choose which RAM will be used. This
configuration is done based on board name (imx8mp_evk/mimx8ml8/m7 for ITCM and
imx8mp_evk/mimx8ml8/m7/ddr for DDR).

There are two methods to load M7 Core images: U-Boot command and Linux remoteproc.

Load and Run M7 Zephyr Image from U-Boot
========================================

Load and run Zephyr on M7 from A53 using u-boot by copying the compiled
``zephyr.bin`` to the first FAT partition of the SD card and plug the SD
card into the board. Power it up and stop the u-boot execution at prompt.

Load the M7 binary onto the desired memory and start its execution using:

ITCM
====

.. code-block:: console

   fatload mmc 0:1 0x48000000 zephyr.bin
   cp.b 0x48000000 0x7e0000 20000
   bootaux 0x7e0000

DDR
===

.. code-block:: console

   fatload mmc 0:1 0x80000000 zephyr.bin
   dcache flush
   bootaux 0x80000000

Load and Run M7 Zephyr Image by using Linux remoteproc
======================================================

Prepare device tree:

The device tree must inlcude CM7 dts node with compatible string "fsl,imx8mn-cm7",
and also need to reserve M4 DDR memory if using DDR code and sys address, and also
need to put "m4_reserved" in the list of memory-region property of the cm7 node.

.. code-block:: console

   reserved-memory {
            #address-cells = <2>;
            #size-cells = <2>;
            ranges;

            m7_reserved: m4@80000000 {
                  no-map;
                  reg = <0 0x80000000 0 0x1000000>;
            };
            ...
   }


   imx8mp-cm7 {
            compatible = "fsl,imx8mn-cm7";
            rsc-da = <0x55000000>;
            clocks = <&clk IMX8MP_CLK_M7_DIV>,
                     <&audio_blk_ctrl IMX8MP_CLK_AUDIO_BLK_CTRL_AUDPLL_ROOT>;
            clock-names = "core", "audio";
            mbox-names = "tx", "rx", "rxdb";
            mboxes = <&mu 0 1
                     &mu 1 1
                     &mu 3 1>;
            memory-region = <&vdevbuffer>, <&vdev0vring0>, <&vdev0vring1>, <&rsc_table>, <&m7_reserved>;
            status = "okay";
            fsl,startup-delay-ms = <500>;
   };

Extra Zephyr Kernel configure item for DDR Image:

If use remotepoc to boot DDR board (imx8mp_evk/mimx8ml8/m7/ddr), also need to enable
"CONFIG_ROMSTART_RELOCATION_ROM" in order to put romstart memory section into ITCM because
M7 Core will get the first instruction from zero address of ITCM, but romstart relocation
will make the storage size of zephyr.bin too large, so we don't enable it by default in
board defconfig.

.. code-block:: console

   diff --git a/boards/nxp/imx8mp_evk/imx8mp_evk_mimx8ml8_m7_ddr_defconfig b/boards/nxp/imx8mp_evk/imx8mp_evk_mimx8ml8_m7_ddr_defconfig
   index 17542cb4eec..8c30c5b6fa3 100644
   --- a/boards/nxp/imx8mp_evk/imx8mp_evk_mimx8ml8_m7_ddr_defconfig
   +++ b/boards/nxp/imx8mp_evk/imx8mp_evk_mimx8ml8_m7_ddr_defconfig
   @@ -12,3 +12,4 @@ CONFIG_CONSOLE=y
   CONFIG_XIP=y
   CONFIG_CODE_DDR=y
   +CONFIG_ROMSTART_RELOCATION_ROM=y

Then use the following steps to boot Zephyr kernel:

1. In U-Boot command line execute prepare script:

.. code-block:: console

   u-boot=> run prepare_mcore

2. Boot Linux kernel with specified dtb and then boot Zephyr by using remoteproc:

.. code-block:: console

   root@imx8mp-lpddr4-evk:~# echo zephyr.elf > /sys/devices/platform/imx8mp-cm7/remoteproc/remoteproc0/firmware
   root@imx8mp-lpddr4-evk:~# echo start  > /sys/devices/platform/imx8mp-cm7/remoteproc/remoteproc0/state
   [   39.195651] remoteproc remoteproc0: powering up imx-rproc
   [   39.203345] remoteproc remoteproc0: Booting fw image zephyr.elf, size 503992
   [   39.203388] remoteproc remoteproc0: No resource table in elf
   root@imx8mp-lpddr4-evk:~# [   39.711380] remoteproc remoteproc0: remote processor imx-rproc is now up

   root@imx8mp-lpddr4-evk:~#

Debugging
=========

MIMX8MP EVK board can be debugged by connecting an external JLink
JTAG debugger to the J24 debug connector and to the PC. Then
the application can be debugged using the usual way.

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: imx8mp_evk/mimx8ml8/m7
   :goals: debug

Open a serial terminal, step through the application in your debugger, and you
should see the following message in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build v2.7.99-1310-g2801bf644a91  ***
   Hello World! imx8mp_evk

References
==========

.. _NXP website:
   https://www.nxp.com/design/development-boards/i-mx-evaluation-and-development-boards/evaluation-kit-for-the-i-mx-8m-plus-applications-processor:8MPLUSLPD4-EVK

.. _i.MX 8M Applications Processor Reference Manual:
   https://www.nxp.com/webapp/Download?colCode=IMX8MPRM
