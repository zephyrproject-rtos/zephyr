.. _bluetooth_mesh_blob:

BLOB Transfer models
####################

The Binary Large Object (BLOB) Transfer models implement the Bluetooth Mesh Binary Large Object
Transfer Model specification version 1.0 and provide functionality for sending large binary objects
from a single source to many Target nodes over the Bluetooth Mesh network. It is the underlying
transport method for the :ref:`bluetooth_mesh_dfu`, but may be used for other object transfer
purposes. The implementation is in experimental state.

The BLOB Transfer models support transfers of continuous binary objects of up to 4 GB (2 \ :sup:`32`
bytes). The BLOB transfer protocol has built-in recovery procedures for packet losses, and sets up
checkpoints to ensure that all targets have received all the data before moving on. Data transfer
order is not guaranteed.

BLOB transfers are constrained by the transfer speed and reliability of the underlying mesh network.
Under ideal conditions, the BLOBs can be transferred at a rate of up to 1 kbps, allowing a 100 kB
BLOB to be transferred in 10-15 minutes. However, network conditions, transfer capabilities and
other limiting factors can easily degrade the data rate by several orders of magnitude. Tuning the
parameters of the transfer according to the application and network configuration, as well as
scheduling it to periods with low network traffic, will offer significant improvements on the speed
and reliability of the protocol. However, achieving transfer rates close to the ideal rate is
unlikely in actual deployments.

There are two BLOB Transfer models:

.. toctree::
   :maxdepth: 1

   blob_srv
   blob_cli

The BLOB Transfer Client is instantiated on the sender node, and the BLOB Transfer Server is
instantiated on the receiver nodes.

Concepts
********

The BLOB transfer protocol introduces several new concepts to implement the BLOB transfer.


BLOBs
=====

BLOBs are binary objects up to 4 GB in size, that can contain any data the application would like to
transfer through the mesh network. The BLOBs are continuous data objects, divided into blocks and
chunks to make the transfers reliable and easy to process. No limitations are put on the contents or
structure of the BLOB, and applications are free to define any encoding or compression they'd like
on the data itself.

The BLOB transfer protocol does not provide any built-in integrity checks, encryption or
authentication of the BLOB data. However, the underlying encryption of the Bluetooth Mesh protocol
provides data integrity checks and protects the contents of the BLOB from third parties using
network and application level encryption.

Blocks
------

The binary objects are divided into blocks, typically from a few hundred to several thousand bytes
in size. Each block is transmitted separately, and the BLOB Transfer Client ensures that all BLOB
Transfer Servers have received the full block before moving on to the next. The block size is
determined by the transfer's ``block_size_log`` parameter, and is the same for all blocks in the
transfer except the last, which may be smaller. For a BLOB stored in flash memory, the block size is
typically a multiple of the flash page size of the Target devices.

Chunks
------

Each block is divided into chunks. A chunk is the smallest data unit in the BLOB transfer, and must
fit inside a single Bluetooth Mesh access message excluding the opcode (379 bytes or less). The
mechanism for transferring chunks depends on the transfer mode.

When operating in Push BLOB Transfer Mode, the chunks are sent as unacknowledged packets from the
BLOB Transfer Client to all targeted BLOB Transfer Servers. Once all chunks in a block have been
sent, the BLOB Transfer Client asks each BLOB Transfer Server if they're missing any chunks, and
resends them. This is repeated until all BLOB Transfer Servers have received all chunks, or the BLOB
Transfer Client gives up.

When operating in Pull BLOB Transfer Mode, the BLOB Transfer Server will request a small number of
chunks from the BLOB Transfer Client at a time, and wait for the BLOB Transfer Client to send them
before requesting more chunks. This repeats until all chunks have been transferred, or the BLOB
Transfer Server gives up.

Read more about the transfer modes in :ref:`bluetooth_mesh_blob_transfer_modes` section.

.. _bluetooth_mesh_blob_stream:

BLOB streams
============

In the BLOB Transfer models' APIs, the BLOB data handling is separated from the high-level transfer
handling. This split allows reuse of different BLOB storage and transfer strategies for different
applications. While the high level transfer is controlled directly by the application, the BLOB data
itself is accessed through a *BLOB stream*.

The BLOB stream is comparable to a standard library file stream. Through opening, closing, reading
and writing, the BLOB Transfer model gets full access to the BLOB data, whether it's kept in flash,
RAM, or on a peripheral. The BLOB stream is opened with an access mode (read or write) before it's
used, and the BLOB Transfer models will move around inside the BLOB's data in blocks and chunks,
using the BLOB stream as an interface.

Interaction
-----------

Before the BLOB is read or written, the stream is opened by calling its
:c:member:`open <bt_mesh_blob_io.open>` callback. When used with a BLOB Transfer Server, the BLOB
stream is always opened in write mode, and when used with a BLOB Transfer Client, it's always opened
in read mode.

For each block in the BLOB, the BLOB Transfer model starts by calling
:c:member:`block_start <bt_mesh_blob_io.block_start>`. Then, depending on the access mode, the BLOB
stream's :c:member:`wr <bt_mesh_blob_io.wr>` or :c:member:`rd <bt_mesh_blob_io.rd>` callback is
called repeatedly to move data to or from the BLOB. When the model is done processing the block, it
calls :c:member:`block_end <bt_mesh_blob_io.block_end>`. When the transfer is complete, the BLOB
stream is closed by calling :c:member:`close <bt_mesh_blob_io.close>`.

Implementations
---------------

The application may implement their own BLOB stream, or use the implementations provided by Zephyr:

.. toctree::
   :maxdepth: 2

   blob_flash


Transfer capabilities
=====================

Each BLOB Transfer Server may have different transfer capabilities. The transfer capabilities of
each device are controlled through the following configuration options:

* :kconfig:option:`CONFIG_BT_MESH_BLOB_SIZE_MAX`
* :kconfig:option:`CONFIG_BT_MESH_BLOB_BLOCK_SIZE_MIN`
* :kconfig:option:`CONFIG_BT_MESH_BLOB_BLOCK_SIZE_MAX`
* :kconfig:option:`CONFIG_BT_MESH_BLOB_CHUNK_COUNT_MAX`

The :kconfig:option:`CONFIG_BT_MESH_BLOB_CHUNK_COUNT_MAX` option is also used by the BLOB Transfer
Client and affects memory consumption by the BLOB Transfer Client model structure.

To ensure that the transfer can be received by as many servers as possible, the BLOB Transfer Client
can retrieve the capabilities of each BLOB Transfer Server before starting the transfer. The client
will transfer the BLOB with the highest possible block and chunk size.

.. _bluetooth_mesh_blob_transfer_modes:

Transfer modes
==============

BLOBs can be transferred using two transfer modes, Push BLOB Transfer Mode and Pull BLOB Transfer
Mode. In most cases, the transfer should be conducted in Push BLOB Transfer Mode.

In Push BLOB Transfer Mode, the send rate is controlled by the BLOB Transfer Client, which will push
all the chunks of each block without any high level flow control. Push BLOB Transfer Mode supports
any number of Target nodes, and should be the default transfer mode.

In Pull BLOB Transfer Mode, the BLOB Transfer Server will "pull" the chunks from the BLOB Transfer
Client at its own rate. Pull BLOB Transfer Mode can be conducted with multiple Target nodes, and is
intended for transferring BLOBs to Target nodes acting as :ref:`bluetooth_mesh_lpn`. When operating
in Pull BLOB Transfer Mode, the BLOB Transfer Server will request chunks from the BLOB Transfer
Client in small batches, and wait for them all to arrive before requesting more chunks. This process
is repeated until the BLOB Transfer Server has received all chunks in a block. Then, the BLOB
Transfer Client starts the next block, and the BLOB Transfer Server requests all chunks of that
block.


.. _bluetooth_mesh_blob_timeout:

Transfer timeout
================

The timeout of the BLOB transfer is based on a Timeout Base value. Both client and server use the
same Timeout Base value, but they calculate timeout differently.

The BLOB Transfer Server uses the following formula to calculate the BLOB transfer timeout::

  10 * (Timeout Base + 1) seconds


For the BLOB Transfer Client, the following formula is used::

  (10000 * (Timeout Base + 2)) + (100 * TTL) milliseconds

where TTL is time to live value set in the transfer.

API reference
*************

This section contains types and defines common to the BLOB Transfer models.

.. doxygengroup:: bt_mesh_blob
   :project: Zephyr
   :members:
