.. _net_buf_interface:

Network Buffer
##############

.. contents::
    :local:
    :depth: 2


Overview
********

Network buffers are a core concept of how the networking stack
(as well as the Bluetooth stack) pass data around. The API for them is
defined in :zephyr_file:`include/net/buf.h`:.

Creating buffers
****************

Network buffers are created by first defining a pool of them:

.. code-block:: c

   NET_BUF_POOL_DEFINE(pool_name, buf_count, buf_size, user_data_size, NULL);

The pool is a static variable, so if it's needed to be exported to
another module a separate pointer is needed.

Once the pool has been defined, buffers can be allocated from it with:

.. code-block:: c

   buf = net_buf_alloc(&pool_name, timeout);

There is no explicit initialization function for the pool or its
buffers, rather this is done implicitly as :c:func:`net_buf_alloc` gets
called.

If there is a need to reserve space in the buffer for protocol headers
to be prepended later, it's possible to reserve this headroom with:

.. code-block:: c

   net_buf_reserve(buf, headroom);

In addition to actual protocol data and generic parsing context, network
buffers may also contain protocol-specific context, known as user data.
Both the maximum data and user data capacity of the buffers is
compile-time defined when declaring the buffer pool.

The buffers have native support for being passed through k_fifo kernel
objects. This is a very practical feature when the buffers need to be
passed from one thread to another. However, since a net_buf may have a
fragment chain attached to it, instead of using the :c:func:`k_fifo_put`
and :c:func:`k_fifo_get` APIs, special :c:func:`net_buf_put` and
:c:func:`net_buf_get` APIs must be used when passing buffers through
FIFOs. These APIs ensure that the buffer chains stay intact. The same
applies for passing buffers through a singly linked list, in which case
the :c:func:`net_buf_slist_put` and :c:func:`net_buf_slist_get`
functions must be used instead of :c:func:`sys_slist_append` and
:c:func:`sys_slist_get`.

Common Operations
*****************

The network buffer API provides some useful helpers for encoding and
decoding data in the buffers. To fully understand these helpers it's
good to understand the basic names of operations used with them:

Add
  Add data to the end of the buffer. Modifies the data length value
  while leaving the actual data pointer intact. Requires that there is
  enough tailroom in the buffer. Some examples of APIs for adding data:

  .. code-block:: c

     void *net_buf_add(struct net_buf *buf, size_t len);
     void *net_buf_add_mem(struct net_buf *buf, const void *mem, size_t len);
     u8_t *net_buf_add_u8(struct net_buf *buf, u8_t value);
     void net_buf_add_le16(struct net_buf *buf, u16_t value);
     void net_buf_add_le32(struct net_buf *buf, u32_t value);

Push
  Prepend data to the beginning of the buffer. Modifies both the data
  length value as well as the data pointer. Requires that there is
  enough headroom in the buffer. Some examples of APIs for pushing data:

  .. code-block:: c

     void *net_buf_push(struct net_buf *buf, size_t len);
     void net_buf_push_u8(struct net_buf *buf, u8_t value);
     void net_buf_push_le16(struct net_buf *buf, u16_t value);

Pull
  Remove data from the beginning of the buffer. Modifies both the data
  length value as well as the data pointer. Some examples of APIs for
  pulling data:

  .. code-block:: c

     void *net_buf_pull(struct net_buf *buf, size_t len);
     void *net_buf_pull_mem(struct net_buf *buf, size_t len);
     u8_t net_buf_pull_u8(struct net_buf *buf);
     u16_t net_buf_pull_le16(struct net_buf *buf);
     u32_t net_buf_pull_le32(struct net_buf *buf);

The Add and Push operations are used when encoding data into the buffer,
whereas Pull is used when decoding data from a buffer.

Reference Counting
******************

Each network buffer is reference counted. The buffer is initially
acquired from a free buffers pool by calling :c:func:`net_buf_alloc()`,
resulting in a buffer with reference count 1. The reference count can be
incremented with :c:func:`net_buf_ref()` or decremented with
:c:func:`net_buf_unref()`. When the count drops to zero the buffer is
automatically placed back to the free buffers pool.


API Reference
*************

.. doxygengroup:: net_buf
   :project: Zephyr
