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

Detailed Protocol Specification
===============================

The ICBMsg protocol transfers messages using dynamically allocated blocks of shared memory.
Internally, it uses ICMsg for control messages.
Small status area in shared memory is used to share additional information that allows hot restart
of the communication after one side resets.
Status area limits the number of control messages used for a transfer increasing
the efficiency of the communication.

No memory clearing on initialization
------------------------------------

The ICBMsg does not clear the shared memory on initialization.
All the existing state is treated as initialized state.
This allows to saferly restart the communication from one side only.


Shared Memory Organization
--------------------------

The ICBMsg uses two shared memory regions, ``rx-region`` for message receiving, and ``tx-region`` for message transmission.
The regions do not need to be next to each other, placed in any specific order, or be of the same size.
Those regions are interchanged on each core.

Each shared memory region is divided into following painformation about blocks usage rts:

* **ICMsg area** - An area reserved by ICMsg instance and used to transfer the control messages.
* **Status area** - An area containing information about the current buffers usage.
* **Blocks area** - An area containing allocatable blocks carrying the content of the messages.
  This area is divided into even-sized blocks aligned to cache boundaries.

The location of each area is calculated to fulfill cache boundary requirements and allow optimal region usage.
It is calculated using the following algorithm:

Inputs:

* ``region_begin``, ``region_end`` - Boundaries of the region.
* ``local_blocks`` - Number of blocks in this region.
* ``remote_blocks`` - Number of blocks in the opposite region.
* ``alignment`` - Memory cache alignment.
* ``force_atomic_size`` - Forced atomic size for architectures where communicating CPUs uses different atomic_t variable sizes.

The algorithm:

#. Align region boundaries to cache:

   * ``region_begin_aligned = ROUND_UP(region_begin, alignment)``
   * ``region_end_aligned = ROUND_DOWN(region_end, alignment)``
   * ``region_size_aligned = region_end_aligned - region_begin_aligned``

#. Calculate the minimum size required for ICMsg area ``icmsg_min_size``, which is a sum of:

   * ICMsg header size (refer to the ICMsg specification)
   * ICMsg message size for 4 bytes of content (refer to the ICMsg specification) multiplied by ``local_blocks + remote_blocks + 2``

#. Calculate address of the status area. Note that the actual size may be higher becouse of block alignment:

   ``status_area_begin = ROUND_UP(icmsg_min_size, alignment)``

#. Calculate the required size of status area and its end address:

   ``status_area_size = CHANNEL_STATUSUS_ALIGN_SIZE(sizeof(waiting_cnt) + CHANNEL_STATUSUS_ALIGN_SIZE(sizeof(send_bm)) + CHANNEL_STATUSUS_ALIGN_SIZE(sizeof(processed_bm))``

   Status area contains following fields:

   * ``waiting_cnt`` - Number of threads that are waiting for free block on TX side
   * ``send_bm`` - Bitmask of blocks that are currently being sent
   * ``processed_bm`` - Bitmask of blocks that are already processed

   The size of the status area is calculated as:

   ``status_area_size = CHANNEL_STATUSUS_ALIGN_SIZE(sizeof(waiting_cnt) + CHANNEL_STATUSUS_ALIGN_SIZE(sizeof(send_bm)) + CHANNEL_STATUSUS_ALIGN_SIZE(sizeof(processed_bm))``

   ``status_area_end = status_area_begin + status_area_size``

   Where ``CHANNEL_STATUSUS_ALIGN_SIZE`` is a macro that calculates the size of the status area:

   ``CHANNEL_STATUSUS_ALIGN_SIZE(x) = ROUND_UP(x, MAX(sizeof(atomic_t), force_atomic_size))``

#. Calculate available size for block area. Note that the actual size may be smaller because of block alignment:

   ``blocks_area_available_size = region_size_aligned - status_area_end``

#. Calculate single block size:

   ``block_size = ROUND_DOWN(blocks_area_available_size / local_blocks, alignment)``

#. Calculate actual block area size:

   ``blocks_area_size = block_size * local_blocks``

#. Calculate block area start address:

   ``blocks_area_begin = region_end_aligned - blocks_area_size``

The result:

* ``region_begin_aligned`` - The start of ICMsg area.
* ``status_area_begin`` - The start of status area.
* ``blocks_area_begin`` - End of ICMsg area and the start of block area.
* ``block_size`` - Single block size.
* ``region_end_aligned`` - End of blocks area.

.. image:: icbmsg_memory.svg
   :align: center

|

Message Transfer
----------------

The ICBMsg uses following two types of messages:

* **Binding message** - Message exchanged during endpoint binding process (described below).
* **Data message** - Message carrying actual data from a user.

They serve different purposes, but their lifetime and flow are the same.
The following steps describe it:

#. The sender wants to send a message that contains ``K`` bytes.
#. The sender reserves blocks from his ``tx-region`` blocks area that can hold at least ``K + 4`` bytes.
   The additional ``+ 4`` bytes are reserved for the header, which contains the exact size of the message.
   The blocks must be continuous (one after another).
   See :ref:`Block allocation` for details.
   It is up to the implementation to decide what to do if no blocks are available.
#. The sender fills the header with a 32-bit integer value, ``K`` (little-endian).
#. The sender fills the remaining part of the blocks with his data.
   Unused space is ignored.
#. The sender sends an ``MSG_DATA`` or ``MSG_BOUND`` control message over ICMsg that contains starting block number (where the header is located).
   Details about the control message are in the next section.
#. The control message travels to the receiver.
#. The receiver reads message size and data from his ``rx-region`` starting from the block number received in the control message.
#. The receiver processes the message.
#. After the message is processed and no longer needed, the receiver marks the fact in the status area in processed_bm field.
#. The receiver sends ``MSG_RELEASE_BOUND`` always or ``MSG_RELEASE_DATA`` only if the field ``waiting_cnt`` in status area shows that any thread on transmitter side waits for buffer.
   The stating block number is important only for bound releasing. For data releasing it would be ignored by the sender.
#. The control message travels back to the sender.
#. If bound releasing message was received the sender, the endpoint bond is considered complete.
   If it was data releasing message and any thread is waiting for buffer, the sender unblocks it and then the free blocks internal variable is updated.
   see :ref:`Block allocation` for details.

.. image:: icbmsg_message.svg
   :align: center

|

.. _Block allocation:

Block allocation
----------------

Block allocation is done with two factors in mind:

#. Any communicating core can reboot at any time (for example by WDT reset).
   If a core reboots, the communication would be restarted as a new session, but the buffers that are currently in use (processed by receiver side) cannot be broken.
#. Buffer allocations should be as fast as possible.

Any block can be in one of 3 states:

#. ``free`` - The block is unused and can be allocated,
#. ``allocated`` - The block is currently being used by the sender,
#. ``sent`` - The block is currently being used by the receiver.

Receiver in not writing anything into the block, so it is not necessary to inform it about the fact that the block is in ``allocated`` state.
This information is keept on the sender side only.
The only information we need to share is that the block is in ``sent`` state.

The internal and shared variables used to manage the blocks are presented on the following diagram:

.. image:: icbmsg_status_allocation.svg
   :align: center

Allowing core reboot at any time
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

When it comes to the core rebooting it is important to share the information about buffer usage between cores.
To archive this in the moment when the communication channel is estabilished the state of the memory is considered valid.
No buffer is cleared on initialization.
This allows to safely restart the communication from one side only not breaking the data in the buffers that may be in a middle of processing on second core.

To do so two fields are placed in status area:
#. ``send_bm`` - Bitmask of blocks that are currently being sent.
#. ``processed_bm`` - Bitmask of blocks that are already processed.

To calculate the allocated blocks the fallowind formula is used:

``blocks_in_sent_state`` = ``send_bm`` ^ ``processed_bm``

As the core that is being initiated does not use any buffers, during the initialization it copies the ``send_bm`` to its ``processed_bm`` effectively marking all blocks free for the sender.


Fast buffer allocation
^^^^^^^^^^^^^^^^^^^^^^

When it comes to speed considerations, the following assumptions are made:

#. Access to shared memory is slower thant access to internal variables.
#. The control message turnaround is slower than accessing the shared memory.

Thous we have to limit number of control messages exchanged at first and then we should limit number of access to schared memory.

Receiver does not send ``MSG_RELEASE_DATA`` control message if there is no thread waiting for buffer.
The information about number of threads waiting for a buffer is kept in ``waiting_cnt`` field in status area.
Sender increases its value if it cannot find a free buffer.

To limit the number of schared memory access, sender keeps track of the allocated blocks in its own ``tx_usage_bm``.
It marks the allocated blocks and does not care if it was released by the receiver.
Only if there is no space for newly requested block the synchronization procedure is performed.
First the ``waiting_cnt`` is increased not to miss ``MSG_RELEASE_DATA`` from receiver.
Then the sender reads its own ``send_bm`` and receiver ``processed_bm`` calculating the used blocks by the formula:

``tx_usage_bm = (send_bm ^ processed_bm) | tx_allocated_bm``

If the block can be allocated now the sended decreases ``waiting_cnt`` and updates its ``tx_usage_bm``.
The allocation is also marked in ``tx_allocated_bm``.
If the block still cannot be allocated the thread is blocked, waiting for ``MSG_RELEASE_DATA``.
After the ``MSG_RELEASE_DATA`` is received the thread is unblocked and the ``tx_usage_bm`` is updated again.
The process repeats until the block is allocated or the timeout is reached.

Just before sending the block, after all the write is finished by the sender,
the bits in shared memory ``send_bm`` related for the block are inverted and the same bits in ``tx_allocated_bm`` are cleared.

The only receiver task for the buffer allocation management is to inverse the bits in ``processed_bm`` related to the block after the message is processed.
Note that the block can be in hold state to be processed later.
The holded blocks are marked in ``rx_hold_bm`` by the receiver internally and are marked as processed later when requested by the application.

Finally, when the receiver marks blocks as processed it always check if ``waiting_cnt`` is not zero to notify the sender if it waits for free block.

All this processes are carefully organized to be processed atomically, using read - modify - cas mechanism.
This makes sure for example that ``waiting_cnt`` is checked in the right moment not to miss its change during the ``processed_bm`` change.

Status block memory layout
^^^^^^^^^^^^^^^^^^^^^^^^^^

The most obvious memory layout for the status block would be like the one presented in following image:

.. image:: icbmsg_status_layout_1_1.svg
   :align: center

In such a configuration we keep every information inside a buffer which it relates to.
It means that on sender side we have to write to ``send_bm`` and read from ``processed_bm`` inside the same memory area.
Rx side would use then ``send_bm`` and writes processed block information to ``processed_bm`` in the same memory area.
It all looks good as long as we are not considering the cache cocherency management.
For such a configuration to be safe, the ``processed_bm`` should be cache page aligned and then the ``Blocks area`` should be also aligned to cache page size.

To mitigate this problem the optimised memory layout was used, where we keep all writes on sender side while reading only on receiver side:

.. image:: icbmsg_status_layout_optimised.svg
   :align: center

The ``processed_bm`` was moved between receiver and sender side.
Now the receiver writes to ``Tx`` memory area the information about processed blocks.
This means that we have all the writes to status block on the ``Tx`` memory and all the reads on ``Rx`` memory.
The direction of access is also the same as the direction of reads and writes in the blocks region.
It means that from the cache coherency perspective the ``blocks area`` start address does not need to be aligned to cache page size.

Control Messages
----------------

The control messages are transmitted over ICMsg.
Each control message contains three bytes.
The first byte tells what kind of message it is.

The allocated size for ICMsg ensures that the maximum possible number of control messages will fit into its ring buffer,
so sending over the ICMsg will never fail because of buffer overflow.

MSG_DATA
^^^^^^^^

.. list-table::
   :header-rows: 1

   * - byte 0
     - byte 1
     - byte 2
   * - MSG_DATA
     - endpoint address
     - block number
   * - 0x00
     - 0x00 ÷ 0xFD
     - 0x00 ÷ N-1

The ``MSG_DATA`` control message indicates that a new data message was sent.
The data message starts with a header inside ``block number``.
The data message was sent over the endpoint specified in ``endpoint address``.
The endpoint binding procedure must be finished before sending this control message.

MSG_RELEASE_DATA
^^^^^^^^^^^^^^^^

.. list-table::
   :header-rows: 1

   * - byte 0
     - byte 1
     - byte 2
   * - MSG_RELEASE_DATA
     - unused
     - block number, ignored on receiver side
   * - 0x01
     -
     - 0x00 ÷ N-1

The ``MSG_RELEASE_DATA`` control message is sent in response to ``MSG_DATA``.
It informs us that the data message starting with ``block number`` was received and is no longer needed.
When this control message is received, the blocks containing the message must be released.

MSG_BOUND
^^^^^^^^^

.. list-table::
   :header-rows: 1

   * - byte 0
     - byte 1
     - byte 2
   * - MSG_BOUND
     - endpoint address
     - block number
   * - 0x02
     - 0x00 ÷ 0xFD
     - 0x00 ÷ N-1

The ``MSG_BOUND`` control message is similar to the ``MSG_DATA`` except the blocks carry binding information.
See the next section for details on the binding procedure.

MSG_RELEASE_BOUND
^^^^^^^^^^^^^^^^^

.. list-table::
   :header-rows: 1

   * - byte 0
     - byte 1
     - byte 2
   * - MSG_RELEASE_BOUND
     - endpoint address
     - block number
   * - 0x03
     - 0x00 ÷ 0xFD
     - 0x00 ÷ N-1

The ``MSG_RELEASE_BOUND`` control message is sent in response to ``MSG_BOUND``.
It is similar to the ``MSG_RELEASE_DATA`` except the ``endpoint address`` is required.
See the next section for details on the binding procedure.

Initialization
--------------

The ICBMsg initialization calls ICMsg to initialize.
When it is done, no further initialization is required.
Blocks can be left uninitialized.

After ICBMsg initialization, you are ready for the endpoint binding procedure.

Endpoint Binding
-----------------

So far, the protocol is symmetrical.
Each side of the connection was the same.
The binding process is not symmetrical.
There are following two roles:

* **Initiator** - It assigns endpoint addresses and sends binding messages.
* **Follower** - It waits for a binding message.

The roles are determined based on the addresses of the ``rx-region`` and ``tx-region``.

* If ``address of rx-region < address of tx-region``, then it is initiator.
* If ``address of rx-region > address of tx-region``, then it is follower.

The binding process needs an endpoint name and is responsible for following two things:

* To establish a common endpoint address,
* To make sure that two sides are ready to exchange messages over that endpoint.

After ICMsg is initialized, both sides can start the endpoint binding.
There are no restrictions on the order in which the sides start the endpoint binding.

Initiator Binding Procedure
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The initiator sends a binding message.
It contains a single null-terminated string with an endpoint name.
As usual, it is preceded by a message header containing the message size (including null-terminator).

Example of the binding message for ``example`` endpoint name:

.. list-table::
   :header-rows: 1

   * - Header
     - Endpoint name
     - Null-terminator
   * - bytes 0-3
     - bytes 4-10
     - byte 11
   * - 0x00000008
     - ``example``
     - 0x00

The binding message is sent using the ``MSG_BOUND`` control message and released with the ``MSG_RELEASE_BOUND`` control message.

The endpoint binding procedure from the initiator's point of view is the following:

#. The initiator assigns an endpoint address to this endpoint.
#. The initiator sends a binding message containing the endpoint name and address.
#. The initiator waits for any message from the follower using this endpoint address.
   Usually, it will be ``MSG_RELEASE_BOUND``, but ``MSG_DATA`` is also allowed.
#. The initiator is bound to an endpoint, and it can send data messages using this endpoint.

Follower Binding Procedure
^^^^^^^^^^^^^^^^^^^^^^^^^^

If the follower receives a binding message before it starts the binding procedure on that endpoint, it should store the message for later.
It should not send the ``MSG_RELEASE_BOUND`` yet.

The endpoint binding procedure from the follower's point of view is the following:

#. The follower waits for a binding message containing its endpoint name.
   The message may be a newly received message or a message stored before the binding procedure started.
#. The follower stores the endpoint address assigned to this endpoint by the initiator.
#. The follower sends the ``MSG_RELEASE_BOUND`` control message.
#. The follower is bound to an endpoint, and it can send data messages using this endpoint.

Example sequence diagrams
-------------------------

The following diagram shows a few examples of how the messages flow between two ends.
There is a binding of two endpoints and one fully processed data message exchange.

.. image:: icbmsg_flows.svg
   :align: center

|

Protocol Versioning
-------------------

The protocol allows improvements in future versions.
The newer implementations should be able to work with older ones in backward compatible mode.
To allow it, the current protocol version has the following restrictions:

* If the receiver receives a longer control message, it should use only the first three bytes and ignore the remaining.
* If the receiver receives a control message starting with a byte that does not match any of the messages described here, it should ignore it.
* If the receiver receives a binding message with additional bytes at the end, it should ignore the additional bytes.
