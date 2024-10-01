.. _ipc_service_backend_icbmsg:

ICMsg with dynamically allocated buffers backend
################################################

This backend is built on top of the :ref:`ipc_service_backend_icmsg`.
Data transferred over this backend travels in dynamically allocated buffers on shared memory.
The ICMsg just sends references to the buffers.
It also supports multiple endpoints.

This architecture allows for overcoming some common problems with other backends (mostly related to multithread access and zero-copy).
This backend provides an alternative with no significant limitations.

Overview
========

The shared memory is divided into two parts.
One is reserved for the ICMsg and the other contains equal-sized blocks.
The number of blocks is configured in the devicetree.

The data sending process is following:

* The sender allocates one or more blocks.
  If there are not enough sequential blocks, it waits using the timeout provided in the parameter that also includes K_FOREVER and K_NO_WAIT.
* The allocated blocks are filled with data.
  For the zero-copy case, this is done by the caller, otherwise, it is copied automatically.
  During this time other threads are not blocked in any way as long as there are enough free blocks for them.
  They can allocate, send data and receive data.
* A message containing the block index is sent over ICMsg to the receiver.
  The size of the ICMsg queue is large enough to hold messages for all blocks, so it will never overflow.
* The receiver can hold the data as long as desired.
  Again, other threads are not blocked as long as there are enough free blocks for them.
* When data is no longer needed, the backend sends a release message over ICMsg.
* When the backend receives this message, it deallocates all blocks.
  It is done internally by the backend and it is invisible to the caller.

Configuration
=============

The backend is configured using Kconfig and devicetree.
When configuring the backend, do the following:

* Define two memory regions and assign them to ``tx-region`` and ``rx-region`` of an instance.
  Ensure that the memory regions used for data exchange are unique (not overlapping any other region) and accessible by both domains (or CPUs).
* Define the number of allocable blocks for each region with ``tx-blocks`` and ``rx-blocks``.
* Define MBOX devices for sending a signal that informs the other domain (or CPU) of the written data.
  Ensure that the other domain (or CPU) can receive the signal.

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
         compatible = "zephyr,ipc-icbmsg";
         tx-region = <&tx>;
         rx-region = <&rx>;
         tx-blocks = <16>;
         rx-blocks = <32>;
         mboxes = <&mbox 0>, <&mbox 1>;
         mbox-names = "tx", "rx";
         status = "okay";
      };
   };


You must provide a similar configuration for the other side of the communication (domain or CPU).
Swap the MBOX channels, memory regions (``tx-region`` and ``rx-region``), and block count (``tx-blocks`` and ``rx-blocks``).

Samples
=======

* :zephyr:code-sample:`ipc_multi_endpoint`
