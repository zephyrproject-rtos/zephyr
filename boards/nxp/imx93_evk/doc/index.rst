.. _imx93_evk:

NXP i.MX93 EVK (Cortex-A55)
############################

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
| TPM       | on-chip    | TPM Counter                         |
+-----------+------------+-------------------------------------+
| ENET      | on-chip    | ethernet port                       |
+-----------+------------+-------------------------------------+

Devices
========
System Clock
------------

This board configuration uses a system clock frequency of 24 MHz.
Cortex-A55 Core runs up to 1.7 GHz.

Serial Port
-----------

This board configuration uses a single serial communication channel with the
CPU's UART4.

Programming and Debugging
*************************

Copy the compiled ``zephyr.bin`` to the first FAT partition of the SD card and
plug the SD card into the board. Power it up and stop the u-boot execution at
prompt.

Use U-Boot to load and kick zephyr.bin to Cortex-A55 Core1:

.. code-block:: console

    fatload mmc 1:1 0xd0000000 zephyr.bin; dcache flush; icache flush; dcache off; icache off; cpu 1 release 0xd0000000


Or use the following command to kick zephyr.bin to Cortex-A55 Core0:

.. code-block:: console

    fatload mmc 1:1 0xd0000000 zephyr.bin; dcache flush; icache flush; dcache off; icache off; go 0xd0000000


Use this configuration to run basic Zephyr applications and kernel tests,
for example, with the :zephyr:code-sample:`synchronization` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: mimx93_evk/mimx9352/a55
   :goals: run

This will build an image with the synchronization sample app, boot it and
display the following ram console output:

.. code-block:: console

    *** Booting Zephyr OS build zephyr-v3.2.0-8-g1613870534a0  ***
    thread_a: Hello World from cpu 0 on mimx93_evk_a55!
    thread_b: Hello World from cpu 0 on mimx93_evk_a55!
    thread_a: Hello World from cpu 0 on mimx93_evk_a55!
    thread_b: Hello World from cpu 0 on mimx93_evk_a55!

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
