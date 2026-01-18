.. zephyr:board:: qnxhv_vm

Overview
********

This board enables running Zephyr as a guest inside a QNX Hypervisor virtual
machine.

This is an example configuration. VM layouts are typically unique per product,
so you will likely need to adjust the devicetree and Kconfig options to match
your VM configuration (memory map, interrupt routing, clocks, devices, etc.).

Hardware
********

Supported Features
==================

.. zephyr:board-supported-hw::

.. _qvmconf:

QNX Hypervisor Virtual Machine Configuration
--------------------------------------------

The virtual hardware exposed to the guest depends on the VM configuration file.
The above features are supported by following configuration.

.. code-block::

  # Guest name (optional)
  system zephyr

  # One vCPU
  cpu

  # Guest RAM base/size
  ram 0x80000000,128M

  # Virtual interrupt controller
  vdev gic version 2

  # Load Zephyr image (ELF is supported by qvm)
  load ./zephyr.elf

  # PL011 UART mapped into the guest at loc, routed to a host device/endpoint
  vdev pl011
          hostdev >-        # QVM console (stdout/stderr), keeps early output visible
          loc 0x1c090000
          intr gic:37


Building and Running
********************

Build an application
====================

Use this board configuration to run basic Zephyr applications as a guest.
For example, build the :zephyr:code-sample:`synchronization` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :board: qnxhv_vm
   :goals: build

This produces a guest image (e.g. ``zephyr/zephyr.elf``) under the build directory.

Create a VM Configuration file
==============================

You also need to create ``.qvmconf`` file to configure virtual machine.
:ref:`qvmconf` is a example QVM configuration which is support by default
``qnxhv_vm`` configuration.

Here, create a file named ``zephyr.qvmconf`` with the contents of this example.

Running an application
======================

Transfer both the Zephyr image and the QVM configuration file to the QNX
Hypervisor machine, then start the VM:

.. code-block:: console

   qvm @zephyr.qvmconf

You will see Zephyr output:

.. code-block:: console

   *** Booting Zephyr OS build v4.3.0-3524-g5c47f098ffc4 ***
   thread_a: Hello World from cpu 0 on qnxhv_vm!
   thread_b: Hello World from cpu 0 on qnxhv_vm!
   thread_a: Hello World from cpu 0 on qnxhv_vm!
   thread_b: Hello World from cpu 0 on qnxhv_vm!
   thread_a: Hello World from cpu 0 on qnxhv_vm!

Use :kbd:`CTRL+C` to stop the virtual machine.


References
**********

- `QNX Hypervisor User's Guide (VM configuration) <https://www.qnx.com/developers/docs/8.0/com.qnx.doc.hypervisor.user/topic/vm/vm.html>`_
