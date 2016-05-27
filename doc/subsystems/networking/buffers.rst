Network Buffers
###############

Network buffers are a core concept of how the networking stack
(as well as the Bluetooth stack) pass data around. The API for them is
defined in ``include/net/buf.h``.

Creating buffers
================

Network buffers are created by first declaring a pool of them:

.. code-block:: c

   static struct nano_fifo free_fifo;
   static NET_BUF_POOL(pool_name, buf_count, buf_size, &free_fifo, NULL,
                       user_data_size);

Before operating on the pool it also needs to be initialized at runtime:

.. code-block:: c

   net_buf_pool_init(pool_name);

Once the pool has been initialized the available buffers are managed
with the help of a nano_fifo object and can be acquired with:

.. code-block:: c

   buf = net_buf_get(&free_fifo, reserve_headroom);

In addition to actual protocol data and generic parsing context, network
buffers may also contain protocol-specific context, known as user data.
Both the maximum data and user data capacity of the buffers is
compile-time defined when declaring the buffer pool.

Since the free buffers are managed with the help of a nano_fifo it means
the buffers have native support for being passed through other nano_fifos
as well. This is a very practical feature when the buffers need to be
passed from one fiber to another.

Common Operations
=================

The network buffer API provides some useful helpers for encoding and
decoding data in the buffers. To fully understand these helpers it's
good to understand the basic names of operations used with them:

Add
  Add data to the end of the buffer. Modifies the data length value
  while leaving the actual data pointer intact. Requires that there is
  enough tailroom in the buffer. Some examples of APIs for adding data:

  .. code-block:: c

     void *net_buf_add(struct net_buf *buf, size_t len);
     uint8_t *net_buf_add_u8(struct net_buf *buf, uint8_t value);
     void net_buf_add_le16(struct net_buf *buf, uint16_t value);
     void net_buf_add_le32(struct net_buf *buf, uint32_t value);

Push
  Prepend data to the beginning of the buffer. Modifies both the data
  length value as well as the data pointer. Requires that there is
  enough headroom in the buffer. Some examples of APIs for pushing data:

  .. code-block:: c

     void *net_buf_push(struct net_buf *buf, size_t len);
     void net_buf_push_le16(struct net_buf *buf, uint16_t value);
     uint32_t net_buf_pull_le32(struct net_buf *buf);

Pull
  Remove data from the beginning of the buffer. Modifies both the data
  length value as well as the data pointer. Some examples of APIs for
  pulling data:

  .. code-block:: c

     void *net_buf_pull(struct net_buf *buf, size_t len);
     uint8_t net_buf_pull_u8(struct net_buf *buf);
     uint16_t net_buf_pull_le16(struct net_buf *buf);

The Add and Push operations are used when encoding data into the buffer,
whereas Pull is used when decoding data from a buffer.

Reference Counting
==================

Each network buffer is reference counted. The buffer is initially
acquired from a free buffers pool by calling :c:func:`net_buf_get()`,
resulting in a buffer with reference count 1. The reference count can be
incremented with :c:func:`net_buf_ref()` or decremented with
:c:func:`net_buf_unref()`. When the count drops to zero the buffer is
automatically placed back to the free buffers pool.
