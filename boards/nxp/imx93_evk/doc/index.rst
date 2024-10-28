.. zephyr:board:: imx93_evk

Overview
********

The i.MX93 Evaluation Kit (MCIMX93-EVK board) is a platform designed to show
the most commonly used features of the i.MX 93 Applications Processor in a
small and low cost package. The MCIMX93-EVK board is an entry-level development
board, which helps developers to get familiar with the processor before
investing a large amount of resources in more specific designs.

i.MX93 MPU is composed of one cluster of 2x Cortex-A55 cores and a single
Cortex®-M33 core. Zephyr OS is ported to run on one of the Cortex®-A55 core.

- Board features:

  - RAM: 2GB LPDDR4
  - Storage:

    - SanDisk 16GB eMMC5.1
    - microSD Socket
  - Wireless:

    - Murata Type-2EL (SDIO+UART+SPI) module. It is based on NXP IW612 SoC,
      which supports dual-band (2.4 GHz /5 GHz) 1x1 Wi-Fi 6, Bluetooth 5.2,
      and 802.15.4
  - USB:

    - Two USB 2.0 Type C connectors
  - Ethernet
  - PCI-E M.2
  - Connectors:

    - 40-Pin Dual Row Header
  - LEDs:

    - 1x Power status LED
    - 2x UART LED
  - Debug

    - JTAG 20-pin connector
    - MicroUSB for UART debug, two COM ports for A55 and M33


Supported Features
==================

The Zephyr mimx93_evk board Cortex-A Core configuration supports the following
hardware features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| GIC-v4    | on-chip    | interrupt controller                |
+-----------+------------+-------------------------------------+
| ARM TIMER | on-chip    | system clock                        |
+-----------+------------+-------------------------------------+
| CLOCK     | on-chip    | clock_control                       |
+-----------+------------+-------------------------------------+
| PINMUX    | on-chip    | pinmux                              |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port                         |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | GPIO                                |
+-----------+------------+-------------------------------------+
| I2C       | on-chip    | i2c                                 |
+-----------+------------+-------------------------------------+
| SPI       | on-chip    | spi                                 |
+-----------+------------+-------------------------------------+
| CAN       | on-chip    | can                                 |
+-----------+------------+-------------------------------------+
| TPM       | on-chip    | TPM Counter                         |
+-----------+------------+-------------------------------------+
| ENET      | on-chip    | ethernet port                       |
+-----------+------------+-------------------------------------+

The Zephyr imx93_evk board Cortex-M33 configuration supports the following
hardware features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| NVIC      | on-chip    | interrupt controller                |
+-----------+------------+-------------------------------------+
| SYSTICK   | on-chip    | systick                             |
+-----------+------------+-------------------------------------+
| CLOCK     | on-chip    | clock_control                       |
+-----------+------------+-------------------------------------+
| PINMUX    | on-chip    | pinmux                              |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port                         |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | GPIO                                |
+-----------+------------+-------------------------------------+

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

Board MUX Control
-----------------

This board configuration uses a series of digital multiplexers to switch between
different board functions. The multiplexers are controlled by a GPIO signal called
``EXP_SEL`` from onboard GPIO expander ADP5585. It can be configured to select
function set "A" or "B" by dts configuration if board control module is enabled.
The following dts node is defined:

.. code-block:: dts

    board_exp_sel: board-exp-sel {
        compatible = "imx93evk-exp-sel";
        mux-gpios = <&gpio_exp0 4 GPIO_ACTIVE_HIGH>;
        mux = "A";
    };

Following steps are required to configure the ``EXP_SEL`` signal:

1. Enable Kconfig option ``CONFIG_BOARD_MIMX93_EVK_EXP_SEL_INIT``.
2. Select ``mux="A";`` or ``mux="B";`` in ``&board_exp_sel`` devicetree node.

Kconfig option ``CONFIG_BOARD_MIMX93_EVK_EXP_SEL_INIT`` is enabled if a board
function that requires configuring the mux is enabled. The MUX option is
automatically selected if certain board function is enabled, and takes precedence
over dts config. For instance, if ``CONFIG_CAN`` is enabled, MUX A is selected
even if ``mux="B";`` is configured in dts, and an warning would be reported in
the log.

User Button GPIO Option
--------------------------

The user buttons RFU_BTN1 and RFU_BTN2 is connected to i.MX 93 GPIO by default,
but can be changed to connect to onboard GPIO expander PCAL6524 with on-board DIP
switches. To do this, switch SW1006 to 0000, then switch SW1005 to 0101. An devicetree
overlay is included to support this.

Run following command to test user buttons on PCAL6524:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/button
   :host-os: unix
   :board: imx93_evk/mimx9352/a55
   :goals: build
   :gen-args: -DEXTRA_DTC_OVERLAY_FILE=imx93_evk_mimx9352_exp_btn.overlay

Run the app, press RFU_BTN1 and the red LED turns on accordingly.

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

    fatload mmc 1:1 0xd0000000 zephyr.bin; dcache flush; icache flush; go 0xd0000000


Use this configuration to run basic Zephyr applications and kernel tests,
for example, with the :zephyr:code-sample:`synchronization` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: imx93_evk/mimx9352/a55
   :goals: build

This will build an image with the synchronization sample app, boot it and
display the following console output:

.. code-block:: console

    *** Booting Zephyr OS build Booting Zephyr OS build v3.7.0-2055-g630f27a5a867  ***
    thread_a: Hello World from cpu 0 on imx93_evk!
    thread_b: Hello World from cpu 0 on imx93_evk!
    thread_a: Hello World from cpu 0 on imx93_evk!
    thread_b: Hello World from cpu 0 on imx93_evk!

System Reboot (A55)
===================

Currently i.MX93 only support cold reboot and doesn't support warm reboot.
Use this configuratiuon to verify cold reboot with :zephyr:code-sample:`shell-module`
sample:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/shell/shell_module
   :host-os: unix
   :board: imx93_evk/mimx9352/a55
   :goals: build

This will build an image with the shell sample app, boot it and execute
kernel reboot command in shell command line:

.. code-block:: console

    uart:~$ kernel reboot cold

Programming and Debugging (M33)
*******************************

Copy the compiled ``zephyr.bin`` to the first FAT partition of the SD card and
plug the SD card into the board. Power it up and stop the u-boot execution at
prompt.

Use U-Boot to load and kick zephyr.bin to Cortex-M33 Core:

.. code-block:: console

    load mmc 1:1 0x80000000 zephyr.bin;cp.b 0x80000000 0x201e0000 0x30000;bootaux 0x1ffe0000 0

Use this configuration to run basic Zephyr applications and kernel tests,
for example, with the :zephyr:code-sample:`synchronization` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: imx93_evk/mimx9352/m33
   :goals: run

This will build an image with the synchronization sample app, boot it and
display the following console output:

.. code-block:: console

    *** Booting Zephyr OS build v3.7.0-684-g71a7d05ba60a ***
    thread_a: Hello World from cpu 0 on imx93_evk!
    thread_b: Hello World from cpu 0 on imx93_evk!
    thread_a: Hello World from cpu 0 on imx93_evk!
    thread_b: Hello World from cpu 0 on imx93_evk!

To make a container image flash.bin with ``zephyr.bin`` for SD/eMMC programming and booting
from BootROM. Refer to user manual of i.MX93 `MCUX SDK release`_.

.. _MCUX SDK release:
   https://mcuxpresso.nxp.com/

References
==========

More information can refer to NXP official website:
`NXP website`_.

.. _NXP website:
   https://www.nxp.com/products/processors-and-microcontrollers/arm-processors/i-mx-applications-processors/i-mx-9-processors/i-mx-93-applications-processor-family-arm-cortex-a55-ml-acceleration-power-efficient-mpu:i.MX93


Using the SOF-specific variant
******************************

Purpose
=======

Since this board doesn't have a DSP, an alternative for people who might be interested
in running SOF on this board had to be found. The alternative consists of running SOF
on an A55 core using Jailhouse as a way to "take away" one A55 core from Linux and
assign it to Zephyr with `SOF`_.

.. _SOF:
        https://github.com/thesofproject/sof

What is Jailhouse?
==================

Jailhouse is a light-weight hypervisor that allows the partitioning of hardware resources.
For more details on how this is done and, generally, about Jailhouse, please see: `1`_,
`2`_ and `3`_. The GitHub repo can be found `here`_.

.. _1:
        https://lwn.net/Articles/578295/

.. _2:
        https://lwn.net/Articles/578852/

.. _3:
        http://events17.linuxfoundation.org/sites/events/files/slides/ELCE2016-Jailhouse-Tutorial.pdf

.. _here:
        https://github.com/siemens/jailhouse


How does it work?
=================
Firstly, we need to explain a few Jailhouse concepts that will be referred to later on:

* **Cell**: refers to a set of hardware resources that the OS assigned to this
  cell can utilize.

* **Root cell**: refers to the cell in which Linux is running. This is the main cell which
  will contain all the hardware resources that Linux will utilize and will be used to assign
  resources to the inmates. The inmates CANNOT use resources such as the CPU that haven't been
  assigned to the root cell.

* **Inmate**: refers to any other OS that runs alongside Linux. The resources an inmate will
  use are taken from the root cell (the cell Linux is running in).

SOF+Zephyr will run as an inmate, alongside Linux, on core 1 of the board. This means that
said core will be taken away from Linux and will only be utilized by Zephyr.

The hypervisor restricts inmate's/root's access to certain hardware resources using
the second-stage translation table which is based on the memory regions described in the
configuration files. Please consider the following scenario:

        Root cell wants to use the **UART** which let's say has its registers mapped in
        the **[0x0 - 0x42000000]** region. If the inmate wants to use the same **UART** for
        some reason then we'd need to also add this region to inmate's configuration
        file and add the **JAILHOUSE_MEM_ROOTSHARED** flag. This flag means that the inmate
        is allowed to share this region with the root. If this region is not set in
        the inmate's configuration file and Zephyr (running as an inmate here) tries
        to access this region this will result in a second stage translation fault.

Notes:

* Linux and Zephyr are not aware that they are running alongside each other.
  They will only be aware of the cores they have been assigned through the config
  files (there's a config file for the root and one for each inmate).

Architecture overview
=====================

The architecture overview can be found at this `location`_. (latest status update as of now
and the only one containing diagrams).

.. _location:
        https://github.com/thesofproject/sof/issues/7192


How to use this board?
======================

This board has been designed for SOF so it's only intended to be used with SOF.

TODO: document the SOF build process for this board. For now, the support for
i.MX93 is still in review and has yet to merged on SOF side.
