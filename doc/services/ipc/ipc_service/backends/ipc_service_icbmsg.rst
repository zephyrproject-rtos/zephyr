.. _ipc_service_backend_icbmsg:

ICMsg with dynamically allocated buffers backend
################################################

Data transferred over this backend travels in dynamically allocated buffers on shared memory.
Allocation is thread safe and can happen from any context.
The backend supports:

* Multiple endpoints.
* No-copy sending.
* Holding RX buffers.
* Sending from an interrupt context.
* Two levels of endpoint priorities.
* Statistics and optional shell command with utilization report
* Up to 32 blocks are supported.
* Data cache support.
* Low memory footprint (around 2 kB of code).

Overview
========

For each direction a shared memory region is reserved and each region is divided into two parts.
One part forms a pool of fixed size buffers and the allocator builds a variable size buffer from adjacent buffers in the pool.
The other part is used for control path consisting of two message queues (for each direction).
There is a producer queue which is written by the sender and read by the receiver and a consumer queue which is written by the receiver and read by the sender.
The producer queue has information about location (within the pool) of the next message.
The consumer queue has information about location (within the pool) of the consumed message.

The data sending process is following:

* The sender allocates one or more blocks from the pool.
  If there are not enough sequential blocks, a thread context waits using the timeout provided in the parameter that also includes K_FOREVER and K_NO_WAIT.
* The allocated blocks are filled with data.
  At the beginning of the first block there is a 32 bit message header with length, endpoint ID and own block index.
  For the zero-copy case, this is done by the caller, otherwise, it is copied automatically.
  During this time other threads are not blocked in any way as long as there are enough free blocks for them.
  They can allocate, send data and receive data.
* A block index with the beginning of the message is written to the producer queue.
  Information about the priority of the endpoint is appended to the block index.
  :kconfig:option:`CONFIG_IPC_SERVICE_BACKEND_ICBMSG_MAX_ACTIVE_COUNT` defines number of slots in the queue.
  Mailbox notification is send to the receiver.
* The receiver reads the producer queue. Higher prioriy messages are processed first.
  It can hold the data as long as desired.
  Again, other threads are not blocked as long as there are enough free blocks for them.
* When data is no longer needed, the receiver writes to the consumer queue the block index.
* The sender is performing a garbage collection by reading the consumer queue and freeing the buffers.
  Garbage collection is performed after sending any message or if there is no available buffers.

Configuration
=============

The backend is configured using Kconfig and devicetree.

There are following Kconfig options:

:kconfig:option:`CONFIG_IPC_SERVICE_BACKEND_ICBMSG_NUM_EP` - maximum number of registered endpoints.

:kconfig:option:`CONFIG_IPC_SERVICE_BACKEND_ICBMSG_MAX_ACTIVE_COUNT` - number of slots in the queues.

:kconfig:option:`CONFIG_IPC_SERVICE_BACKEND_ICBMSG_DEINIT` - support for deregistration and closing.

:kconfig:option:`CONFIG_IPC_SERVICE_BACKEND_ICBMSG_SHELL` - support for the shell command.

When configuring the backend, do the following:

* If at least one of the cores uses data cache on shared memory, set the ``dcache-alignment`` value.
  This must be the largest value of the invalidation or the write-back size for both sides of the communication.
  You can skip it if none of the communication sides is using data cache on shared memory.
* Define two memory regions and assign them to ``tx-region`` and ``rx-region`` of an instance.
  Ensure that the memory regions used for data exchange are unique (not overlapping any other region) and accessible by both domains (or CPUs).
* Define the number of allocable blocks for each region with ``tx-blocks`` and ``rx-blocks``.
* Define MBOX devices for sending a signal that informs the other domain (or CPU) of the written data.
  Ensure that the other domain (or CPU) can receive the signal.

.. caution::

    Make sure that you set correct value of the ``dcache-alignment``.
    At first, wrong value may not show any signs, which may give a false impression that everything works.
    Unstable behavior will appear sooner or later.

If ``dcache-alignment`` is used then configuration for the number of blocks should be selected carefully to avoid inefficient usage of the memory.
That is because the blocks are aligned to the cache alignment and if the number of blocks is not a multiple of the cache alignment then the last block will not be used efficiently.
That is because the blocks and control data are aligned to the cache alignment.
For example, if ``dcache-alignment`` is 32 and 1024 bytes of shared memory is used for one direction.
Control data will take 64 bytes and 960 bytes are left for the buffers.
Using 16 blocks will result in 32 bytes per block (due to the cache alignment).
Using 15 blocks will result in 64 bytes per block (due to the cache alignment) with much better memory utilization.


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
         dcache-alignment = <32>;
         tx-region = <&tx>;
         rx-region = <&rx>;
         tx-blocks = <16>;
         rx-blocks = <16>;
         mboxes = <&mbox 0>, <&mbox 1>;
         mbox-names = "tx", "rx";
         status = "okay";
      };
   };


You must provide a similar configuration for the other side of the communication (domain or CPU).
Swap the MBOX channels, memory regions (``tx-region`` and ``rx-region``), and block count (``tx-blocks`` and ``rx-blocks``).

Limitations
===========

* Expected same endianness on both sides of the communication.
* No support for detection of unexpected remote reset.

Samples
=======

* :zephyr:code-sample:`ipc_multi_endpoint`

Detailed Protocol Specification
===============================

The ICBMsg protocol transfers messages using dynamically allocated blocks of shared memory.

Shared Memory Organization
--------------------------

The ICBMsg uses two shared memory regions, ``rx-region`` for message receiving, and ``tx-region`` for message transmission.
The regions do not need to be next to each other, placed in any specific order, or be of the same size.
Those regions are interchanged on each core.

Each shared memory region is divided into following two parts:

* **Control area** - An area reserved by producer and consumer queues.
* **Blocks area** - An area containing allocatable blocks carrying the content of the messages.
  This area is divided into even-sized blocks aligned to cache boundaries.

The location of each area is calculated to fulfill cache boundary requirements and allow optimal region usage.
It is calculated using the following algorithm:

Inputs:

* ``region_begin``, ``region_end`` - Boundaries of the region.
* ``local_blocks`` - Number of blocks in this region.
* ``remote_blocks`` - Number of blocks in the opposite region.
* ``alignment`` - Memory cache alignment.

The algorithm:

#. Align region boundaries to cache:

   * ``region_begin_aligned = ROUND_UP(region_begin, alignment)``
   * ``region_end_aligned = ROUND_DOWN(region_end, alignment)``
   * ``region_size_aligned = region_end_aligned - region_begin_aligned``

#. Calculate the minimum size required for the control area:

   * for each queue there is :kconfig:option:`IPC_SERVICE_BACKEND_ICBMSG_MAX_ACTIVE_COUNT` bytes and 8 byte queue header.
   * Typically control data takes less than 64 bytes per direction.

#. Calculate available size for block area. Note that the actual size may be smaller because of block alignment:

   ``blocks_area_available_size = region_size_aligned - control_area``

#. Calculate single block size:

   ``block_size = ROUND_DOWN(blocks_area_available_size / local_blocks, alignment)``

#. Calculate actual block area size:

   ``blocks_area_size = block_size * local_blocks``

#. Calculate block area start address:

   ``blocks_area_begin = region_end_aligned - blocks_area_size``

The result:

* ``region_begin_aligned`` - The start of ICMsg area.
* ``blocks_area_begin`` - End of ICMsg area and the start of block area.
* ``block_size`` - Single block size.
* ``region_end_aligned`` - End of blocks area.

.. image:: icbmsg_memory.svg
   :align: center

|

Message Transfer
----------------

The ICBMsg uses following two types of messages:

* **Control message** - Messages like binding or unbinding.
* **Data message** - Message carrying actual user data.

They serve different purposes, but their lifetime and flow are the same.
The following steps describe it:

#. The sender wants to send a message that contains ``K`` bytes.
#. The sender reserves blocks from his ``tx-region`` blocks area that can hold at least ``K + 4`` bytes.
   The additional ``+ 4`` bytes are reserved for the header.
   The blocks must be continuous (one after another).
   The sender is responsible for block allocation management.
   If blocks are not available then a thread context may block and an interrupt context will return error.
#. The sender fills the header.
#. The sender fills the remaining part of the blocks with his data.
   Unused space is ignored.
#. The sender writes the message to the producer queue and sends the mailbox signal.
#. The receiver excutes mailbox callback in the mailbox interrupt context and reads the producer queue.
#. The receiver reads the block index and locates the message within his ``rx-region``.
#. The receiver reads the endpoint and message length and processes the message.
#. The receiver consumes the message by writing its block index to the consumer queue.
   The mailbox signal is not send.
#. The sender checks consume queue after each sending or if sending fails.
   Messages are read from the consumer queue and released to the pool.

.. image:: icbmsg_message.svg
   :align: center

|

Binding Instances
-----------------

When backend instance is opened it send bound message which contains a 64 bit magic number.
Mailbox callback is enabled and instance is waiting for bound message.
After receiving bound message, instance is bound to the remote instance and endpoints can be registered.

Binding Endpoint
----------------

Endpoint binding message contains SHA of the endpoint name and endpoint ID which is the index in the local array with endpoint's data.

There are two possible scenarios:

* The remote instance sent binding message for the endpoint before it was registered.
* The endpoint is registered before receiving the binding message from the remote instance.

When the binding message is received then SHA is compared to the stored SHA in the array of endpoint's data.
If match is found then it means that the endpoint is already registered by the local instance.
Endpoint ID is stored in the endpoint's data and bound callback is called.
If match is not found then empty slot is found and endpoint ID and SHA is stored in the available slot.

When endpoint is registered then SHA for the name is calculated and compared to the stored SHA in the array of endpoint's data.
If match is found then it means that remote binding message for that endpoint is already received.
In this case binding message is sent to the remote instance and bound callback is called.
If match is not found then empty slot is found and endpoint ID and SHA is stored in the available slot.
Binding message is sent to the remote instance but endpoints are not bound yet.

Later on, remote endpoint ID is used for data message to identify the endpoint.

Unbinding Endpoint
------------------

When endpoint is unregistered then unbinding control message is sent.
Endpoint is removed from the array of endpoint's data.
When unbinding message is received then endpoint slot is marked as empty and unbinding callback is called.
