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

The two types are supported: a plain shared memory (ivshmem-plain) or a shared
memory with the ability for a VM to generate an interruption on another, and
thus to be interrupted as well itself (ivshmem-doorbell).

Please refer to the official `Qemu ivshmem documentation
<https://www.qemu.org/docs/master/system/devices/ivshmem.html>`_ for more information.

Support
*******

Zephyr supports both versions: plain and doorbell. Ivshmem driver can be built
by enabling :kconfig:option:`CONFIG_IVSHMEM`. By default, this will expose the plain
version. :kconfig:option:`CONFIG_IVSHMEM_DOORBELL` needs to be enabled to get the
doorbell version.

Because the doorbell version uses MSI-X vectors to support notification vectors,
the :kconfig:option:`CONFIG_IVSHMEM_MSI_X_VECTORS` has to be tweaked to the number of
vectors that will be needed.

Note that a tiny shell module can be exposed to test the ivshmem feature by
enabling :kconfig:option:`CONFIG_IVSHMEM_SHELL`.

ivshmem-flat
************

QEMU ivshmem-flat provides shared memory and doorbell notifications for machines without PCI. 
Enable :kconfig:option:`CONFIG_IVSHMEM_FLAT` and provide a
:dtcompatible:`qemu,ivshmem-flat` node describing the register region, shared memory region,
and optional receive IRQ. These resources must match the local QEMU machine wiring, and
the shared memory size must match the ivshmem server configuration.

Doorbell notifications require :kconfig:option:`CONFIG_IVSHMEM_DOORBELL`. Only vector 0 is supported; configure the server with ``-n 1``. 
Without a receive IRQ, shared memory access
and sending doorbells remain available, while ``ivshmem_register_handler()`` returns
``-ENOSYS``. With an IRQ, the driver raises the registered ``k_poll_signal`` from its ISR.
The flat driver does not support :kconfig:option:`CONFIG_IVSHMEM_V2`.

See :zephyr:code-sample:`ivshmem-flat` for the local QEMU wiring patch and a two-VM ping-pong example.

ivshmem-v2
**********

Zephyr also supports ivshmem-v2:

https://github.com/siemens/jailhouse/blob/master/Documentation/ivshmem-v2-specification.md

This is primarily used for IPC in the Jailhouse hypervisor
(e.g. :zephyr:code-sample:`eth-ivshmem`). It is also possible to use ivshmem-v2 without
Jailhouse by building the Siemens fork of QEMU, and modifying the QEMU launch flags:

https://github.com/siemens/qemu/tree/wip/ivshmem2

API Reference
*************

.. doxygengroup:: ivshmem
