.. _xen_dom0:

Xen Dom0: universal snippet for XEN control domain
##################################################

Overview
********

This snippet allows user to build Zephyr as a Xen initial domain (Dom0). The feature
is implemented as configuration snippet to allow support for any compatible platform.

How to add support of a new board
*********************************

* add board dts overlay to this snippet which deletes/adds memory and deletes UART nodes;
* add correct memory and hypervisor nodes, based on regions Xen picked for Domain-0 on your setup.

Programming
***********

Correct snippet designation for Xen must
be entered when you invoke ``west build``.
For example:

.. code-block:: console

   west build -b qemu_cortex_a53 -S xen_dom0 samples/synchronization

QEMU example with Xen
***********************

Overlay for qemu_cortex_a53 board, that is present in `board/` directory of this snippet is QEMU
Xen control domain example. To run such setup, you need to:

* fetch and build Xen (e.g. RELEASE-4.17.0) for arm64 platform
* take and compile sample device tree from `example/` directory
* build your Zephyr sample/application with `xen_dom0` snippet and start it as Xen control domain

For starting you can use QEMU from Zephyr SDK by following command:

.. code-block:: console

    <path to Zephyr SDK>/sysroots/x86_64-pokysdk-linux/usr/bin/qemu-system-aarch64 -cpu cortex-a53 \
    -m 6G -nographic -machine virt,gic-version=3,virtualization=true -chardev stdio,id=con,mux=on \
    -serial chardev:con -mon chardev=con,mode=readline -pidfile qemu.pid \
    -device loader,file=<path to Zephyr app build>/zephyr.bin,addr=0x40600000 \
    -dtb <path to DTB>/xen.dtb -kernel <path to Xen build>/xen

This will start you a Xen hypervisor with your application as Xen control domain. To make it usable,
you can add `zephyr-xenlib` by Xen-troops library to your project. It'll provide basic domain
management functionalities - domain creation and configuration.
