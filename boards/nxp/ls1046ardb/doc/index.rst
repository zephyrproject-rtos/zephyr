.. zephyr:board:: ls1046ardb

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

.. zephyr:board-supported-hw::

.. note::

   There are two serial ports on the board: uart1 and uart2. Zephyr is using
   uart2 as serial console.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Use the following configuration to run basic Zephyr applications and
kernel tests on LS1046A RDB board. For example, with the :zephyr:code-sample:`synchronization` sample:

1. Non-SMP mode

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: ls1046ardb
   :goals: build

This will build an image with the synchronization sample app.

Use u-boot to load and kick Zephyr.bin to CPU Core0:

.. code-block:: console

	tftp c0000000 zephyr.bin; dcache off; dcache flush; icache flush; icache off; go  0xc0000000;

Or kick Zephyr.bin to any other CPU Cores, for example run Zephyr on Core3:

.. code-block:: console

	tftp c0000000 zephyr.bin; dcache off; dcache flush; icache flush; icache off; cpu 3 release 0xc0000000;


It will display the following console output:

.. code-block:: console

	*** Booting Zephyr OS build zephyr-v2.5.0-1922-g3265b69d47e7  ***
	thread_a: Hello World from cpu 0 on nxp_ls1046ardb!
	thread_b: Hello World from cpu 0 on nxp_ls1046ardb!
	thread_a: Hello World from cpu 0 on nxp_ls1046ardb!

2. SMP mode running on 4 CPU Cores

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: ls1046ardb/ls1046a/smp/4cores
   :goals: build

This will build an image with the synchronization sample app.

Use u-boot to load and kick Zephyr.bin to CPU Core0:

.. code-block:: console

	tftp c0000000 zephyr.bin; dcache off; dcache flush; icache flush; icache off; go  0xc0000000;

It will display the following console output:

.. code-block:: console

	*** Booting Zephyr OS build zephyr-v2.5.0-1922-g3265b69d47e7  ***
	Secondary CPU core 1 (MPID:0x1) is up
	Secondary CPU core 2 (MPID:0x2) is up
	Secondary CPU core 3 (MPID:0x3) is up
	thread_a: Hello World from cpu 0 on nxp_ls1046ardb!
	thread_b: Hello World from cpu 1 on nxp_ls1046ardb!
	thread_a: Hello World from cpu 0 on nxp_ls1046ardb!

3. SMP mode running on 2 CPU Cores: Core2 and Core3

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: ls1046ardb/ls1046a/smp
   :goals: build

This will build an image with the synchronization sample app.

Use u-boot to load and kick Zephyr.bin to CPU Core2:

.. code-block:: console

	tftp c0000000 zephyr.bin; dcache off; dcache flush; icache flush; icache off; cpu 2 release 0xc0000000;

It will display the following console output:

.. code-block:: console

	*** Booting Zephyr OS build zephyr-v2.5.0-1922-g3265b69d47e7  ***
	Secondary CPU core 1 (MPID:0x3) is up
	thread_a: Hello World from cpu 0 on nxp_ls1046ardb!
	thread_b: Hello World from cpu 1 on nxp_ls1046ardb!
	thread_a: Hello World from cpu 0 on nxp_ls1046ardb!

4. Running Zephyr on Jailhouse inmate Cell

Use the following to run Zephyr in Jailhouse inmate, need to configure Jailhouse
inmate Cell to use a single Core for Zephyr non-SMP mode, or use Core2 and Core3
for Zephyr SMP 2cores image.

1) Use root Cell dts to boot root Cell Linux.

2) Install Jailhouse module:

.. code-block:: console

	modprobe jailhouse

3) Run Zephyr demo in inmate Cell:

.. code-block:: console

	jailhouse enable ls1046a-rdb.cell
	jailhouse cell create ls1046a-rdb-inmate-demo.cell
	jailhouse cell load 1 zephyr.bin --address 0xc0000000
	jailhouse cell start 1

Flashing
========

Zephyr image can be loaded in DDR memory at address 0xc0000000 from SD Card,
EMMC, QSPI Flash or downloaded from network in uboot.

Debugging
=========

LS1046A RDB board includes one JTAG connector on board, connect it to
CodeWarrior TAP for debugging.

.. include:: ../../common/board-footer.rst
   :start-after: nxp-board-footer

References
==========

`Layerscape LS1046A Reference Design Board <https://www.nxp.com/design/qoriq-developer-resources/layerscape-ls1046a-reference-design-board:LS1046A-RDB>`_

`LS1046A Reference Manual <https://www.nxp.com/webapp/Download?colCode=LS1046ARM>`_
