.. _x86_jailhouse:

Jailhouse cell X86 Emulation (QEMU)
###################################

Overview
********

The X86 Jailhouse QEMU board configuration is used to emulate the X86
architecture. This board configuration provides support for x86 64-bit
(KVM) passthrough CPUs (-cpu kvm64) and the following devices:

* Advanced Programmable Interrupt Controller (APIC)
* NS16550 UART

This is to prove that Zephyr, with this Jailhouse port, would run on
any x86-64 CPUs baremetal as well. The port only reaches the UART via
port I/O, now, but it can be extended to do MMIO.

Hardware
********

Supported Features
==================

This configuration supports the following hardware features:

+--------------+------------+-----------------------+
| Interface    | Controller | Driver/Component      |
+==============+============+=======================+
| APIC         | on-chip    | interrupt controller  |
+--------------+------------+-----------------------+
| NS16550      | on-chip    | serial port           |
| UART         |            |                       |
+--------------+------------+-----------------------+

Devices
=======

Serial Port
-----------

The board configuration uses a single serial communication channel
that uses the NS16550 serial driver operating in polling mode (port
I/O is used).

Known Problems or Limitations
=============================

The following platform features are unsupported:

* Serial port in Direct Memory Access (DMA) mode
* Serial Peripheral Interface (SPI) flash
* General-Purpose Input/Output (GPIO)
* Inter-Integrated Circuit (I2C)
* Ethernet

Programming and Debugging
*************************

Use this configuration to run basic Zephyr applications and kernel
tests in a QEMU emulated environment (we assume the same QEMU
configuration used to accomodate Jailhouse's configs/qemu-x86.c root
cell configuration).

For example, with the :ref:`synchronization_sample`, one would issue:

.. code-block:: console

        $ make -C samples/synchronization BOARD=x86_jailhouse JAILHOUSE_QEMU_IMG_FILE=path_to_image.qcow2 run

This assumes the user has the binary qemu-system-x86_64 in their
system (not provided by Zephyr's toolchain). This is because the base
system here will be 64-bit and Zephyr will boot as a virtual guest in
it. The qcow2 image must contain the base image with Jailhouse
installed (not provided by Zephyr's SDK, again).

This will build an image with the synchronization sample app and mount
the current directory inside QEMU under /mnt.

From a console, in that QEMU instance, issue:

.. code-block:: shell

        $ sudo insmod <path to jailhouse.ko>
        $ sudo jailhouse enable <path to configs/qemu-x86.cell>
        $ sudo jailhouse cell create <path to configs/tiny-demo.cell>
        $ sudo mount -t 9p -o trans/virtio host /mnt
        $ sudo jailhouse cell load tiny-demo /mnt/zephyr.bin
        $ sudo jailhouse cell start tiny-demo
        $ sudo jailhouse cell destroy tiny-demo
        $ sudo jailhouse disable
        $ sudo rmmod jailhouse

This should display the following console output:

.. code-block:: console

        Created cell "tiny-demo"
        Page pool usage after cell creation: mem 275/1480, remap 65607/131072
        Cell "tiny-demo" can be loaded
        CPU 3 received SIPI, vector 100
        Started cell "tiny-demo"
        ***** BOOTING ZEPHYR OS v1.8.99 - BUILD: Jun 27 2017 13:09:26 *****
        threadA: Hello World from x86!
        threadB: Hello World from x86!
        threadA: Hello World from x86!
        threadB: Hello World from x86!
        threadA: Hello World from x86!
        threadB: Hello World from x86!
        threadA: Hello World from x86!
        threadB: Hello World from x86!
        threadA: Hello World from x86!
        threadB: Hello World from x86!

Debugging
=========

Refer to the detailed overview about :ref:`application_debugging`.
