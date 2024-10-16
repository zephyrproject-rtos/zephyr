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

 - :zephyr:code-sample:`ipc-icmsg`

Detailed Protocol Specification
===============================

The ICMsg uses two shared memory regions and two MBOX channels.
The region and channel pair are used to transfer messages in one direction.
The other pair is symmetric and transfers messages in the opposite direction.
For this reason, the specification below focuses on one such pair.
The other pair is identical.

The ICMsg provides just one endpoint per instance.

Shared Memory Region Organization
---------------------------------

If data caching is enabled, the shared memory region provided to ICMsg must be aligned according to the cache requirement.
If cache is not enabled, the required alignment is 4 bytes.

The shared memory region is entirely used by a single FIFO.
It contains read and write indexes followed by the data buffer.
The detailed structure is contained in the following table:

.. list-table::
   :header-rows: 1

   * - Field name
     - Size (bytes)
     - Byte order
     - Description
   * - ``rd_idx``
     - 4
     - little‑endian
     - Index of the first incoming byte in the ``data`` field.
   * - ``padding``
     - depends on cache alignment
     - n/a
     - Padding added to align ``wr_idx`` to the cache alignment.
   * - ``wr_idx``
     - 4
     - little‑endian
     - Index of the byte after the last incoming byte in the ``data`` field.
   * - ``data``
     - everything to the end of the region
     - n/a
     - Circular buffer containing actual bytes to transfer.

This is usual FIFO with a circular buffer:

* The Indexes (``rd_idx`` and ``wr_idx``) are wrapped around when they reach the end of the ``data`` buffer.
* The FIFO is empty if ``rd_idx == wr_idx``.
* The FIFO has one byte less capacity than the ``data`` buffer length.

Packets
-------

Packets are sent over the FIFO described in the above section.
One packet can be wrapped around if it occurs at the end of the FIFO buffer.

The following is the packet structure:

.. list-table::
   :header-rows: 1

   * - Field name
     - Size (bytes)
     - Byte order
     - Description
   * - ``len``
     - 2
     - big‑endian
     - Length of the ``data`` field.
   * - ``reserved``
     - 2
     - n/a
     - Reserved for the future use.
       It must be 0 for the current protocol version.
   * - ``data``
     - ``len``
     - n/a
     - Packet data.
   * - ``padding``
     - 0‑3
     - n/a
     - Padding is added to align the total packet size to 4 bytes.

The packet send procedure is the following:

#. Check if the packet fits into the buffer.
#. Write the packet to ``data`` FIFO buffer starting at ``wr_idx``.
   Wrap it if needed.
#. Write a new value of the ``wr_idx``.
#. Notify the receiver over the MBOX channel.

Initialization
--------------

The initialization sequence is the following:

#. Set the ``wr_idx`` and ``rd_idx`` to zero.
#. Push a single packet to FIFO containing magic data: ``45 6d 31 6c 31 4b 30 72 6e 33 6c 69 34``.
   The MBOX is not used yet.
#. Initialize the MBOX.
#. Repeat the notification over the MBOX channel using some interval, for example, 1 ms.
#. Wait for an incoming packet containing the magic data.
   It will arrive over the other pair (shared memory region and MBOX).
#. Stop repeating the MBOX notification.

After this, the ICMsg is bound, and it is ready to transfer packets.
