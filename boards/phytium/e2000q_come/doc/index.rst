.. zephyr:board:: e2000q_come

Overview
********

The Phytium E2000Q COM Express board is a COM Express module built around the
Phytium E2000Q (PE2204) SoC. The SoC integrates two FTC664 cores and two
FTC310 cores, a GICv3 interrupt controller, an ARM architected timer and
PL011 UARTs.

Zephyr runs as a Non-secure EL1 payload and is started by U-Boot, which
configures the secure side and provides the PSCI services used to bring up
the secondary cores.

Hardware
********

The E2000Q (PE2204) provides:

* 2x FTC664 + 2x FTC310 ARMv8-A cores
* ARM GICv3 interrupt controller with ITS
* ARM architected timer (50 MHz)
* PL011 UARTs
* PCIe host controller

Supported Features
==================

.. zephyr:board-supported-hw::

The ``e2000q_come`` board target boots a single core, the
``e2000q_come/pe2204/smp`` target starts all four cores through PSCI.

Serial Port
===========

The console is UART1 at ``0x2800d000``, driven at 115200 8N1. It is used both
as the Zephyr console and as the shell UART.

PCI Express
===========

The PCIe host uses the generic ECAM driver. The boot firmware must configure
the root ports, PHYs and address translation windows before starting Zephyr.
Zephyr enumerates the bus hierarchy and assigns BARs and bridge windows.
Both the single-core and SMP board targets support PCIe.

Enable :kconfig:option:`CONFIG_PCIE` in the application. To inspect devices
with the shell, also enable :kconfig:option:`CONFIG_SHELL` and
:kconfig:option:`CONFIG_PCIE_SHELL`, then run ``pcie ls``.

The host provides an I/O window at CPU address ``0x50000000``, a 32-bit
memory window at ``0x58000000``, and a 64-bit prefetchable memory window at
``0x1000000000``. I/O bus addresses are translated through the I/O window;
memory bus addresses match CPU physical addresses.

Enable :kconfig:option:`CONFIG_PCIE_MSI` and
:kconfig:option:`CONFIG_PCIE_MSI_MULTI_VECTOR` for the MSI vector allocation
APIs. MSI support also enables the GICv3 ITS and reserves interrupt table
entries for LPIs. Devices using MSI-X additionally need
:kconfig:option:`CONFIG_PCIE_MSI_X`.
The devicetree interrupt map routes INTx pins A through D to GIC SPIs 4
through 7 for statically described PCIe devices.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The image is loaded into DRAM by U-Boot. Build the image as usual:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :host-os: unix
   :board: e2000q_come
   :goals: build

Wrap ``zephyr.bin`` in a legacy U-Boot image and boot it with ``bootm``,
which follows the same path as a Linux kernel:

.. code-block:: console

   mkimage -C none -A arm64 -O linux -a 0x80100000 -e 0x801010b4 \
     -d build/zephyr/zephyr.bin build/zephyr/zephyr.uImage

``-a`` is the address Zephyr is linked at (``0x80100000``) and ``-e`` is the
entry point, which ``aarch64-zephyr-elf-readelf -h build/zephyr/zephyr.elf``
reports.

``bootm`` requires a devicetree even though Zephyr uses the one compiled into
the image, so load any devicetree blob next to it and pass its address:

.. code-block:: console

   E2000# fatload mmc 0:1 0x90000000 pe2204-come-board.dtb
   E2000# tftpboot 0x90100000 zephyr.uImage
   E2000# bootm 0x90100000 - 0x90000000

To start all four cores, build the SMP variant and boot it the same way:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :host-os: unix
   :board: e2000q_come/pe2204/smp
   :goals: build

Known Problems or Limitations
=============================

* Resetting through PSCI does not restart the module, the same symptom as in
  Linux. Probably a firmware bug.
