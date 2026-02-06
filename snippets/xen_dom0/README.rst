.. _xen_dom0:

Xen Dom0: universal snippet for XEN control domain
##################################################

Overview
********

This snippet allows user to build Zephyr as a Xen initial domain (Dom0). The feature
is implemented as configuration snippet to allow support for any compatible platform.

How to add support of a new board
*********************************

* add board dt overlay to this snippet which deletes UART nodes.
* in application dt overlay add correct memory and hypervisor nodes, based on regions
  Xen picked for Domain-0 on your setup.

Programming
***********

Correct snippet designation for Xen must
be entered when you invoke ``west build``.
For example:

.. code-block:: console

   west build -b qemu_cortex_a53 -S xen_dom0 samples/synchronization

QEMU example with Xen
***********************

Overlay for qemu_cortex_a53 board, that is present in :zephyr_file:`snippets/xen_dom0/boards/`
directory of this snippet is QEMU Xen control domain example.

To run such setup, you need to:

* fetch and build Xen (e.g. RELEASE-4.17.0) for arm64 platform
* take and compile sample device tree from :file:`example/` directory
* build your Zephyr sample/application with ``xen_dom0`` snippet and start it as Xen control domain

Important notice regarding device tree
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Xen allocates addresses used for Dom0 image and grant tables/extended regions dynamically so they
may differ on specific setups (they depend on memory banks location and size, application
image size, hypervisor load address used by bootloader etc.). Since Zephyr does not parse device
tree in runtime and is not a position independent executable, it needs to know exact values on
build time. To make it you need to build your final binary, try to boot it with Xen and analyze
the hypervisor output to get exact addresses for your setup. They will look as follows:

Grant table region (first reg in hypervisor node):

.. code-block:: console

    (XEN) Grant table range: 0x00000088080000-0x000000880c0000

Extended regions (2nd and further regs in hypervisor node):

.. code-block:: console

    (XEN) [    0.928173] d0: extended region 2: 0x40000000->0x47e00000

Zephyr Dom0 memory:

.. code-block:: console

    (XEN) Allocating 1:1 mappings for dom0:
    (XEN) BANK[0] 0x00000060000000-0x00000070000000 (256MB)

Device tree nodes, that should be added by application overlay for provided example
(based on R-Car H3ULCB with extended region):

.. code-block:: devicetree

    hypervisor: hypervisor@88080000 {
        compatible = "xen,xen";
        reg = <0x0 0x88080000 0x0 0x40000 0x0 0x40000000 0x0 0x7e00000>;
        interrupts = <GIC_PPI 0x0 IRQ_TYPE_EDGE IRQ_DEFAULT_PRIORITY>;
        interrupt-parent = <&gic>;
        status = "okay";
    };

    ram: memory@60000000 {
        device_type = "mmio-sram";
        reg = <0x00 0x60000000 0x00 DT_SIZE_M(256)>;
    };

Start the example
^^^^^^^^^^^^^^^^^

For starting you can use QEMU from Zephyr SDK by following command:

.. code-block:: console

    <path to Zephyr SDK>/sysroots/x86_64-pokysdk-linux/usr/bin/qemu-system-aarch64 -cpu cortex-a53 \
    -m 6G -nographic -machine virt,gic-version=3,virtualization=true -chardev stdio,id=con,mux=on \
    -serial chardev:con -mon chardev=con,mode=readline -pidfile qemu.pid \
    -device loader,file=<path to Zephyr app build>/zephyr.bin,addr=0x40600000 \
    -dtb <path to DTB>/xen.dtb -kernel <path to Xen build>/xen

This will start you a Xen hypervisor with your application as Xen control domain. To make it usable,
you can add `zephyr-xenlib`_ by Xen-troops library to your project. It'll provide basic domain
management functionalities - domain creation and configuration.

.. _zephyr-xenlib: https://github.com/xen-troops/zephyr-xenlib
