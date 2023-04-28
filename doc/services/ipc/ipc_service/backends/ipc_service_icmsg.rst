.. _ipc_service_backend_icmsg:

ICMsg backend
#############

The inter core messaging backend (ICMsg) is a lighter alternative to the
heavier RPMsg static vrings backend. It offers a minimal feature set in a small
memory footprint. The ICMsg backend is build on top of :ref:`spsc_pbuf`.

Overview
========

The ICMsg backend uses shared memory and MBOX devices for exchanging data.
Shared memory is used to store the data, MBOX devices are used to signal that
the data has been written.

The backend supports the registration of a single endpoint on a single
instance. If the application requires more than one communication channel, you
must define multiple instances, each having its own dedicated endpoint.

Configuration
=============

The  backend is configured via Kconfig and devicetree.
When configuring the backend, do the following:

* Define two memory regions and assign them to ``tx-region`` and ``rx-region``
  of an instance. Ensure that the memory regions used for data exchange are
  unique (not overlapping any other region) and accessible by both domains
  (or CPUs).
* Define MBOX devices which are used to send the signal that informs the other
  domain (or CPU) that data has been written. Ensure that the other domain
  (or CPU) is able to receive the signal.

See the following configuration example for one of the instances:

.. code-block:: devicetree

   reserved-memory {
      tx: memory@20070000 {
         reg = <0x20070000 0x0800>;
      };

      rx: memory@20078000 {
         reg = <0x20078000 0x0800>;
      };
   };

      ipc {
         ipc0: ipc0 {
            compatible = "zephyr,ipc-icmsg";
            tx-region = <&tx>;
            rx-region = <&rx>;
            mboxes = <&mbox 0>, <&mbox 1>;
            mbox-names = "tx", "rx";
            status = "okay";
         };
      };
   };


You must provide a similar configuration for the other side of the
communication (domain or CPU) but you must swap the MBOX channels and  memory
regions (``tx-region`` and ``rx-region``).

Bonding
=======

When the endpoint is registered, the following happens on each domain (or CPU)
connected through the IPC instance:

1. The domain (or CPU) writes a magic number to its ``tx-region`` of the shared
memory.
#. It then sends a signal to the other domain or CPU, informing that the data
has been written. Sending the signal to the other domain or CPU is repeated
with timeout specified by
:kconfig:option:`CONFIG_IPC_SERVICE_ICMSG_BOND_NOTIFY_REPEAT_TO_MS` option.
#. When the signal from the other domain or CPU is received, the magic number
is read from ``rx-region``. If it is correct, the bonding process is finished
and the backend informs the application by calling
:c:member:`ipc_service_cb.bound` callback.

Samples
=======

 - :ref:`ipc_icmsg_sample`
