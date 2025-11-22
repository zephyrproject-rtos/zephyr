.. zephyr:code-sample:: vhost
   :name: Vhost sample application

Overview
********

This sample demonstrates the use of the vhost driver subsystem for implementing
VIRTIO backends in Zephyr. The application shows how to:

* Initialize and configure vhost devices
* Handle VIRTIO queue operations
* Process guest requests using the vringh utility
* Implement a basic VIRTIO backend for Xen virtualization

The sample sets up a vhost device that can communicate with VIRTIO frontend
drivers in guest virtual machines, specifically designed for Xen MMIO
virtualization environments.

Requirements
************

This sample requires:

* A Xen hypervisor environment
* Xen domain management tools
* A board that supports Xen virtualization (e.g., xenvm)

Building and Running
********************

This application can be built and executed on Xen as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/virtualization/vhost
   :host-os: unix
   :board: xenvm
   :goals: run
   :compact:

The application will initialize the vhost subsystem and wait for VIRTIO
frontend connections from guest domains. When a guest connects and sends
requests, the sample will process them and provide responses.

Expected Output
***************

When running successfully, you should see output similar to::

   *** Booting Zephyr OS build zephyr-v3.x.x ***
   [00:00:00.000,000] <inf> vhost: VHost device ready
   [00:00:00.000,000] <dbg> vhost: queue_ready_handler(dev=0x..., qid=0, data=0x...)
   [00:00:00.000,000] <dbg> vhost: vringh_kick_handler: queue_id=0
