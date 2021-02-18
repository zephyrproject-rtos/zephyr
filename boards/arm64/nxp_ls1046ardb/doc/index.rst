.. _nxp_ls1046ardb:

NXP LS1046A RDB
#################################

Overview
********

The LS1046A reference design board (RDB) is a high-performance computing,
evaluation, and development platform that supports the Layerscape LS1046A
architecture processor. The LS1046ARDB board supports the Layerscape LS1046A
processor and is optimized to support the DDR4 memory and a full complement
of high-speed SerDes ports.

The Layerscape LS1046A processor integrates four 64-bit Arm(R) Cortex(R) A72
cores with packet processing acceleration and high-speed peripherals. The
impressive performance of more than 32,000 CoreMarks, paired with 10 Gb
Ethernet, PCIe Gen. 3, SATA 3.0, USB 3.0 and QSPI interfaces provides an
excellent combination for a range of enterprise and service provider
networking, storage, security and industrial applications.

Hardware
********

LS1046A RDB boards supports the following features:


- Four 32/64-bit Arm(R) Cortex(R)V8 A72 CPUs, up to 1.6 GHz core speed
- Supports 8 GB DDR4 SDRAM memory
- SDHC port connects directly to an adapter card slot, featuring 4 GB eMMCi
  memory device
- One 512 MB SLC NAND flash with ECC support (1.8 V)
- CPLD connection: 8-bit registers in CPLD to configure mux/demux selections
- Support two 64 MB onboard QSPI NOR flash memories
- USB:
  - Two USB 3.0 controllers with integrated PHYs.
  - One USB1 3.0 port is connected to a Type A host connector.
  - One USB1 3.0 port is configured as On-The-Go (OTG) with a Micro-AB connector.
  - One USB2.0 is connected to miniPCIe connector .
- Ethernet:
  - Supports SGMII 1G PHYs at Lane 2 and Lane 3
  - Supports SFP+module with XFI retimers
  - Supports AQR106/107 10G PHY with XFI/2.5G SGMII
- PCIe and SATA:
  - Mini PCIe express x1 (Gen1/2/3)card
  - Standard PCIe x1 (Gen1/2/3) card
  - Standard PCIe x1 (Gen1/2/3) card
  - One SATA 3.0 connector

Supported Features
==================

NXP LS1046A RDB board default configuration supports the following
hardware features:

+-----------+------------+--------------------------------------+
| Interface | Controller | Driver/Component                     |
+===========+============+======================================+
| GIC-400   | on-chip    | GICv2 interrupt controller           |
+-----------+------------+--------------------------------------+
| ARM TIMER | on-chip    | System Clock                         |
+-----------+------------+--------------------------------------+
| UART      | on-chip    | NS16550 compatible serial port       |
+-----------+------------+--------------------------------------+

Other hardware features are not supported by the Zephyr kernel.

The default configuration can be found in the defconfig file for NON-SMP:

        ``boards/arm64/nxp_ls1046ardb/nxp_ls1046ardb_defconfig``

Or for SMP:

	``boards/arm64/nxp_ls1046ardb/nxp_ls1046ardb_smp_defconfig``

There are two serial port on the board: uart1 and uart2, Zephyr is using
uart2 as serial console.

Programming and Debugging
*************************

Use the following configuration to run basic Zephyr applications and
kernel tests on LS1046A RDB board. For example, with the :ref:`synchronization_sample`:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: nxp_ls1046ardb
   :goals: run

This will build an image with the synchronization sample app.

Use u-boot to load and kick Zephyr.bin:

.. code-block:: console

	tftp c0000000 zephyr.bin; dcache off; dcache flush; icache flush; icache off;go  0xc0000000;

It will isplay the following console output:

.. code-block:: console

	*** Booting Zephyr OS build zephyr-v2.5.0-1922-g3265b69d47e7  ***
	thread_a: Hello World from cpu 0 on nxp_ls1046ardb!
	thread_b: Hello World from cpu 0 on nxp_ls1046ardb!
	thread_a: Hello World from cpu 0 on nxp_ls1046ardb!
	thread_b: Hello World from cpu 0 on nxp_ls1046ardb!
	thread_a: Hello World from cpu 0 on nxp_ls1046ardb!
	thread_b: Hello World from cpu 0 on nxp_ls1046ardb!

Flashing
========

Zephyr image can be loaded in DDR memory at address 0xc0000000 from SD Card,
EMMC, QSPI Flash or downloaded from network in uboot.

Debugging
=========

LS1046A RDB board includes one JTAG connector on board, connect it to
CodeWarrior TAP for debugging.

References
==========

`Layerscape LS1046A Reference Design Board <https://www.nxp.com/design/qoriq-developer-resources/layerscape-ls1046a-reference-design-board:LS1046A-RDB>`_

`LS1046A Reference Manual <https://www.nxp.com/webapp/Download?colCode=LS1046ARM>`_
