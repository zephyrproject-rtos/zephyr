.. zephyr:board:: frdm_imx93

Overview
********

The FRDM-IMX93 board is a low-cost and compact platform designed to show
the most commonly used features of the i.MX 93 Applications Processor in a
small and low cost package. The FRDM-IMX93 board is an entry-level development
board, which helps developers to get familiar with the processor before
investing a large amount of resources in more specific designs.

i.MX93 MPU is composed of one cluster of 2x Cortex®-A55 cores and a single
Cortex®-M33 core. Zephyr RTOS is ported on Cortex®-A55 core.

Hardware
********

- i.MX 93 applications processor

  - The processor integrates up to two Arm Cortex-A55 cores, and supports
    built-in Arm Cortex-M33 core.

- RAM: 2GB LPDDR4
- Storage:

  - SanDisk 16GB eMMC5.1
  - microSD Socket
- Wireless:

  - Murata Type-2EL (SDIO+UART+SPI) module. It is based on NXP IW612 SoC,
    which supports dual-band (2.4 GHz /5 GHz) 1x1 Wi-Fi 6, Bluetooth 5.2,
    and 802.15.4
- USB:

  - One USB 2.0 Type C connector
  - One USB 2.0 Type A connector
- Ethernet
- PCI-E M.2
- Connectors:

  - 40-Pin Dual Row Header
- LEDs:

  - 1x Power status LED
  - 1x RGB LED
- Debug

  - JTAG 3-pin connector
  - USB-C port for UART debug, two COM ports for A55 and M33


Supported Features
==================

.. zephyr:board-supported-hw::

Devices
========
System Clock
------------

This board configuration uses a system clock frequency of 24 MHz.
Cortex-A55 Core runs up to 1.7 GHz.
Cortex-M33 Core runs up to 200MHz in which SYSTICK runs on same frequency.

Serial Port
-----------

This board configuration uses a single serial communication channel with the
CPU's UART2 for A55 core and M33 core.

User Button GPIO Option
--------------------------

The user buttons USER_BTN1 and USER_BTN2 are connected to onboard GPIO expander
PCAL6524 by default, but can be changed to connect to FRDM-IMX93 GPIO. A devicetree
overlay is included to support this.

Run following command to test user buttons connected to FRDM-IMX93 GPIO:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/button
   :host-os: unix
   :board: frdm_imx93/mimx9352/a55
   :goals: build
   :gen-args: -DEXTRA_DTC_OVERLAY_FILE=frdm_imx93_mimx9352_native_btn.overlay

Note: The overlay only supports ``mimx9352/a55``, but can be extended to support
``mimx9352/m33`` if I2C and PCAL6524 is enabled.

Programming and Debugging (A55)
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

Use U-Boot to load and kick zephyr.bin to Cortex-A55 Core1:

.. code-block:: console

    fatload mmc 1:1 0xd0000000 zephyr.bin; dcache flush; icache flush; cpu 1 release 0xd0000000


Or use the following command to kick zephyr.bin to Cortex-A55 Core0:

.. code-block:: console

    fatload mmc 1:1 0xd0000000 zephyr.bin; dcache off; icache flush; go 0xd0000000


Use this configuration to run basic Zephyr applications and kernel tests,
for example, with the :zephyr:code-sample:`synchronization` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: frdm_imx93/mimx9352/a55
   :goals: build

This will build an image with the synchronization sample app, boot it and
display the following console output:

.. code-block:: console

    *** Booting Zephyr OS build v4.1.0-41-g6395333e3d18 ***
    thread_a: Hello World from cpu 0 on frdm_imx93!
    thread_b: Hello World from cpu 0 on frdm_imx93!
    thread_a: Hello World from cpu 0 on frdm_imx93!
    thread_b: Hello World from cpu 0 on frdm_imx93!

System Reboot (A55)
===================

Currently i.MX93 only support cold reboot and doesn't support warm reboot.
Use this configuratiuon to verify cold reboot with :zephyr:code-sample:`shell-module`
sample:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/shell/shell_module
   :host-os: unix
   :board: frdm_imx93/mimx9352/a55
   :goals: build

This will build an image with the shell sample app, boot it and execute
kernel reboot command in shell command line:

.. code-block:: console

    uart:~$ kernel reboot cold

.. include:: ../../common/board-footer.rst
   :start-after: nxp-board-footer
