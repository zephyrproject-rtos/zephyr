.. _ivshmem_driver:

Inter-VM Shared Memory
######################

.. contents::
   :local:
   :depth: 2

Overview
********

As Zephyr is enabled to run as a guest OS on Qemu and
`ACRN <https://projectacrn.github.io/latest/tutorials/using_zephyr_as_uos.html>`_
it might be necessary to make VMs aware of each other, or aware of the host.
This is made possible by exposing a shared memory among parties via a feature
called ivshmem, which stands for inter-VM Shared Memory.

The Two types are supported: a plain shared memory (ivshmem-plain) or a shared
memory with the ability for a VM to generate an interruption on another, and
thus to be interrupted as well itself (ivshmem-doorbell).

Please refer to the official `Qemu ivshmem documentation
<https://www.qemu.org/docs/master/system/ivshmem.html>`_ for more information.

Support
*******

Zephyr supports both version: plain and doorbell. Ivshmem driver can be build
by enabling :option:`CONFIG_IVSHMEM`. By default, this will expose the plain
version. :option:`CONFIG_IVSHMEM_DOORBELL` needs no be enabled to get the
doorbell version.

Because the doorbell version uses MSI-X vectors to support notification vectors,
the :option:`CONFIG_IVSHMEM_MSI_X_VECTORS` has to be tweaked to the amount of
vectors that will be needed.

Note that a tiny shell module can be exposed to test the ivshmem feature by
enabling :option:`CONFIG_IVSHMEM_SHELL`.

API Reference
*************

.. doxygengroup:: ivshmem
   :project: Zephyr
