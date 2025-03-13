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

* If at least one of the cores uses data cache on shared memory, set the ``dcache-alignment`` value.
  This must be the largest value of the invalidation or the write-back size for both sides of the communication.
  You can skip it if none of the communication sides is using data cache on shared memory.
* Define two memory regions and assign them to ``tx-region`` and ``rx-region``
  of an instance. Ensure that the memory regions used for data exchange are
  unique (not overlapping any other region) and accessible by both domains
  (or CPUs).
* Define MBOX devices which are used to send the signal that informs the other
  domain (or CPU) that data has been written. Ensure that the other domain
  (or CPU) is able to receive the signal.

.. caution::

    Make sure that you set correct value of the ``dcache-alignment``.
    At first, wrong value may not show any signs, which may give a false impression that everything works.
    Unstable behavior will appear sooner or later.

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
            dcache-alignment = <32>;
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

#. The domain (or CPU) writes a magic number to its ``tx-region`` of the shared
   memory.
#. It then sends a signal to the other domain or CPU, informing that the data
   has been written. Sending the signal to the other domain or CPU is repeated
   with timeout.
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
Each region and channel pair is used to transfer messages in one direction, while the other pair, which is symmetric, transfers messages in the opposite direction.
Therefore, the specification below focuses on one such pair, as the other is identical.

The ICMsg provides a single endpoint per instance.

The ICMsg protocol has been updated to support disconnect and reset detection, as well as to improve the binding process.
The updated version is referred to as version ``1.1`` in this document, while the initial version is referred to as version ``1.0``.
Version ``1.1`` is designed to be optionally backward-compatible with version ``1.0``.

Packets
-------

The structure and behavior of packets are the same for both protocol versions.

Packets are transmitted through the FIFO, as described in the version-specific sections below.
A packet may wrap around if it reaches the end of the FIFO buffer.

The following table illustrates the packet structure:

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
     - Reserved for future use.
       It must be 0 for the current protocol versions.
   * - ``data``
     - ``len``
     - n/a
     - Packet data.
   * - ``padding``
     - 0‑3
     - n/a
     - Padding is added to align the total packet size to 4 bytes.

The packet sending procedure is as follows:

#. Check if the packet fits into the buffer.
#. Write the packet to the ``data`` FIFO buffer starting at ``wr_idx``, wrapping it if needed.
#. Write a new value to the ``wr_idx``.
#. Notify the receiver via the MBOX channel.

Protocol Version 1.1
--------------------

Shared Memory Region Organization
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

If data caching is enabled, the shared memory region provided to ICMsg must be aligned according to the cache requirements.
If cache is not enabled, the required alignment is 4 bytes.

To support disconnect and reset detection, the protocol uses a session ID that changes every time ICMsg connects.
The remote and local sides have their own session IDs, which are updated independently and may differ.
In this document, they are referred to as remote SID and local SID, respectively.
The session ID is 16 bits long.
The session ID value ``0`` is reserved and indicates a disconnected state.

The shared memory region contains a FIFO and handshake data.
The detailed structure is shown in the following table:

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
   * - ``handshake``
     - 4
     - little‑endian
     - Handshake word described below.
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

The handshake word is defined differently for the TX and RX regions.

For the TX region, the handshake word is defined as follows:

.. list-table::
   :header-rows: 1

   * - Field name
     - Location
     - Size (bits)
     - Access
     - Description
   * - ``remote_sid_req``
     - Lower bits
     - 16
     - read-only
     - Remote SID requested by the remote side.
   * - ``local_sid_ack``
     - Upper bits
     - 16
     - read-only
     - Local SID acknowledged by the remote side.

For the RX region, the handshake word is defined as follows:

.. list-table::
   :header-rows: 1

   * - Field name
     - Location
     - Size (bits)
     - Access
     - Description
   * - ``local_sid_req``
     - Lower bits
     - 16
     - read-write
     - Local SID requested by the local side.
   * - ``remote_sid_ack``
     - Upper bits
     - 16
     - write-only
     - Remote SID acknowledged by the local side.

Handshake Process
^^^^^^^^^^^^^^^^^

Below is a general overview of the handshake process.
The same process is done in the opposite direction.

#. The local side selects its own local SID and writes it to ``local_sid_req``
   which indicates that the local side requested a new session with a specific SID.
#. The remote side reads this field and writes it back to ``local_sid_ack``
   which indicates that the remote side acknowledged the session.
   Additionally, it reads current values of FIFO pointers assuming that FIFO is empty at this point.
#. The local side reads ``local_sid_ack`` and if it is equal to the value written in step 1, the connection is established.

A more detailed description of the handshake process is provided below.

The handshake process starts when the local side opens the ICMsg instance.
The process is as follows:

#. Establish the local SID:

   - Read current ``local_sid_req``.
   - Increment it by ``1``.
   - Repeat incrementing if the new value is equal to ``local_sid_ack`` or ``0``.

#. Write handshake word in RX region with the following values:

   - ``local_sid_req`` set to the value calculated in the previous step.
   - ``remote_sid_ack`` set to ``0``.

   Keep those values locally as ``local_sid`` and ``remote_sid`` respectively.

#. Send a notification to the remote side with MBOX and wait for an incoming notification from the remote side.

#. The remote side will do the same, so at some point, an MBOX notification will be received.
   The following steps will run in the notification handler.

#. Read ``remote_sid_req`` and ``local_sid_ack`` from TX region handshake word.

#. If ``remote_sid_req`` is different from ``remote_sid`` and not equal to ``0``, the remote side has requested a new session.

   - We can now initialize the TX FIFO, since we know that the remote side, during receiving, will first read the FIFO indexes.
     Later, it will check if the session has changed before using indexes to receive the message.
     Additionally, we know that the remote side, after a session request change, will not try to receive more data.
   - Keep ``remote_sid_req`` locally as ``remote_sid``.
   - Write handshake word in RX region with ``local_sid_req`` set to ``local_sid`` and ``remote_sid_ack`` set to ``remote_sid``.
   - The remote SID has changed at this point, so the remote side is able to handle MBOX notifications.
     Notify the remote side with MBOX.
     This final notification is needed in case the remote side started too late and missed the previous notification that was sent when the instance was opened.

#. If ``local_sid_ack`` is the same as ``local_sid`` and ``remote_sid`` is not equal to ``0``, the connection is now established.
   Both sides requested a new session and both acknowledged it.
   The ICMsg is ready to transfer packets.
   The remote side may have already started to send packets, so check for incoming packets before exiting the notification handler.

Packets Transmission and Unbound Detection
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Once the handshake process is complete, the packet sending procedure remains the same as in version ``1.0``, described above.

When packets are received, the handshake word is checked to detect if the remote has changed its session ID.
There may be various reasons for this, such as the remote being reset or the remote closing and optionally reopening the ICMsg instance.

The packet receive procedure starts when a new MBOX notification is received and proceeds as follows:

#. Read the packet from the RX FIFO and update ``rd_idx`` accordingly.
   If there is no packet or the packet is invalid, do not fail immediately, but keep that information for later.
#. Read ``remote_sid_req`` from the TX region handshake word.
   If it differs from the ``remote_sid`` stored locally, the remote has requested a new session or disconnected.
   The connection is now unbound, so take appropriate action, such as calling the ``unbound`` callback.
   Stop processing incoming packets, ignoring any failures from step 1.
#. If any failure was detected in step 1, handle it now.
#. Process the incoming packet, for example, by calling the appropriate callback.
#. If there are still packets in the FIFO, repeat the procedure.

When the instance is closed, set ``local_sid`` and ``local_sid_req`` to ``0`` and send an MBOX notification to the remote.
This will inform the remote that the connection is unbound.

Protocol Version 1.0
--------------------

Shared Memory Region Organization
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

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

Initialization
^^^^^^^^^^^^^^

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
