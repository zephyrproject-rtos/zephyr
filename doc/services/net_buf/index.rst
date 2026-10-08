.. _net_buf_interface:

Network Buffers
###############

.. contents::
    :local:
    :depth: 2


Overview
********

Network buffers are a core concept of how the networking stack
(as well as the Bluetooth stack) pass data around. The API for them is
defined in :zephyr_file:`include/zephyr/net_buf.h`:.

Buffer Layout
*************

The data of a buffer is kept in a block of storage whose size is fixed
when the buffer is allocated. The data does not have to start at the
beginning of the storage: there can be free space before it, called the
headroom, and after it, called the tailroom.

.. mermaid::
   :caption: Layout of a network buffer
   :alt: The storage of a buffer from __buf to __buf + size, divided into
         headroom, len and tailroom. The data pointer marks the boundary
         between headroom and len, and data + len the boundary between len
         and tailroom. The size arrow spans all three.

   block-beta
     columns 12
     p0["__buf"] space:2 p1["data"] space:4 p2["data + len"] space:2 p3["__buf + size"]
     headroom["headroom"]:3 len["len"]:5 tailroom["tailroom"]:4
     size<["size"]>(x):12

     classDef ptr fill:none,stroke:none
     classDef free stroke-dasharray:5,fill-opacity:0.3
     class p0,p1,p2,p3 ptr
     class headroom,tailroom free

Four fields of the buffer describe this layout:

``data``
  Points to the start of the data.

``len``
  Length of the data behind the ``data`` pointer.

``size``
  Size of the storage, headroom and tailroom included.

``__buf``
  Start of the storage. Not to be accessed directly: use the ``data``
  pointer instead.

The headroom is returned by :c:func:`net_buf_headroom`, the tailroom by
:c:func:`net_buf_tailroom`, and :c:func:`net_buf_tail` returns
``data + len``, where the next byte added to the buffer goes. The room
left for more data is the tailroom, not ``size`` minus ``len``:
``size`` counts the headroom as well.

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
objects. Use :c:func:`k_fifo_put` and :c:func:`k_fifo_get` to pass buffer
from one thread to another.

Special functions exist for dealing with buffers in single linked lists,
where the :c:func:`net_buf_slist_put` and :c:func:`net_buf_slist_get`
functions must be used instead of :c:func:`sys_slist_append` and
:c:func:`sys_slist_get`.

Common Operations
*****************

The network buffer API provides some useful helpers for encoding and
decoding data in the buffers. To fully understand these helpers it's
good to understand the basic names of operations used with them:

.. mermaid::
   :caption: Where the four operations change the data of a buffer
   :alt: The buffer as headroom, len and tailroom. Push moves the start of
         the data into the headroom and pull moves it towards the end.
         Add moves the end of the data into the tailroom and remove moves
         it towards the start.

   block-beta
     columns 24
     space:2 push<["push"]>(left):4 space:10 add<["add"]>(right):4 space:4
     headroom["headroom"]:6 len["len"]:10 tailroom["tailroom"]:8
     space:6 pull<["pull"]>(right):4 space:2 remove<["remove"]>(left):4 space:8

     classDef free stroke-dasharray:5,fill-opacity:0.3
     class headroom,tailroom free

Add and Remove work at the end of the data and change only ``len``.
Push and Pull work at the start of the data and move the ``data``
pointer as well.

Add
  Add data to the end of the buffer. Modifies the data length value
  while leaving the actual data pointer intact. Requires that there is
  enough tailroom in the buffer. Some examples of APIs for adding data:

  .. code-block:: c

     void *net_buf_add(struct net_buf *buf, size_t len);
     void *net_buf_add_mem(struct net_buf *buf, const void *mem, size_t len);
     uint8_t *net_buf_add_u8(struct net_buf *buf, uint8_t value);
     void net_buf_add_le16(struct net_buf *buf, uint16_t value);
     void net_buf_add_le32(struct net_buf *buf, uint32_t value);

Remove
  Remove data from the end of the buffer. Modifies the data length value
  while leaving the actual data pointer intact. Some examples of APIs for
  removing data:

  .. code-block:: c

     void *net_buf_remove_mem(struct net_buf *buf, size_t len);
     uint8_t net_buf_remove_u8(struct net_buf *buf);
     uint16_t net_buf_remove_le16(struct net_buf *buf);
     uint32_t net_buf_remove_le32(struct net_buf *buf);

Push
  Prepend data to the beginning of the buffer. Modifies both the data
  length value as well as the data pointer. Requires that there is
  enough headroom in the buffer. Some examples of APIs for pushing data:

  .. code-block:: c

     void *net_buf_push(struct net_buf *buf, size_t len);
     void *net_buf_push_mem(struct net_buf *buf, const void *mem, size_t len);
     void net_buf_push_u8(struct net_buf *buf, uint8_t value);
     void net_buf_push_le16(struct net_buf *buf, uint16_t value);

Pull
  Remove data from the beginning of the buffer. Modifies both the data
  length value as well as the data pointer. Some examples of APIs for
  pulling data:

  .. code-block:: c

     void *net_buf_pull(struct net_buf *buf, size_t len);
     void *net_buf_pull_mem(struct net_buf *buf, size_t len);
     uint8_t net_buf_pull_u8(struct net_buf *buf);
     uint16_t net_buf_pull_le16(struct net_buf *buf);
     uint32_t net_buf_pull_le32(struct net_buf *buf);

The Add and Push operations are used when encoding data into the buffer,
whereas the Remove and Pull operations are used when decoding data from a
buffer.

Reference Counting
******************

Each network buffer is reference counted. The buffer is initially
acquired from a free buffers pool by calling :c:func:`net_buf_alloc()`,
resulting in a buffer with reference count 1. The reference count can be
incremented with :c:func:`net_buf_ref()` or decremented with
:c:func:`net_buf_unref()`. When the count drops to zero the buffer is
automatically placed back to the free buffers pool.

Ownership
*********

Each reference to a buffer has exactly one owner. The owner either
releases its reference with :c:func:`net_buf_unref` or moves it to a new
owner, such as a FIFO, a list or a function that takes ownership of the
buffer. After the move the previous owner must no longer use the buffer,
unless it holds another reference of its own.

A fragment chain is owned through its head. Each buffer in the chain
owns a reference to the next fragment and releases it when the buffer
itself is freed, so releasing the last reference to the head releases
the chain up to the first fragment that is still referenced elsewhere.
:c:func:`net_buf_frag_insert` takes ownership of the fragment it is
given, and so does :c:func:`net_buf_frag_add` when the head is not
``NULL``. With a ``NULL`` head, :c:func:`net_buf_frag_add` returns the
fragment with a new reference and the caller keeps its own.

A function or callback that is given a buffer does one of the following
with the reference, and its documentation should say which:

Borrows
  The function may use the buffer until it returns, but may neither keep
  nor move it. To keep a borrowed buffer, it acquires a reference of its
  own with :c:func:`net_buf_ref` first.

Takes ownership
  The reference moves to the function on every return, errors included.

Takes ownership on success
  The reference moves only if the function succeeds. On error the caller
  still owns the buffer.

Two helpers operate on the pointer that holds a reference, so that the
previous owner is left with ``NULL`` instead of a pointer to a buffer it
no longer owns. :c:func:`net_buf_take` moves the reference out of the
pointer, and :c:func:`net_buf_drop` releases it. :c:func:`net_buf_drop`
is meant for a pointer that outlives the release, such as a structure
member, or one that may already be ``NULL``. A local pointer that holds a
reference as it goes out of scope only needs :c:func:`net_buf_unref`.

Putting a buffer in a FIFO or a list moves the reference to the queue.
The receiving side may process and free the buffer even before
:c:func:`k_fifo_put` returns, for instance when a higher-priority thread
is waiting on the FIFO. Move the reference with :c:func:`net_buf_take` in
the same expression:

.. code-block:: c

   k_fifo_put(&tx_queue, net_buf_take(&buf));

Do this even where ``buf`` goes out of scope right after the call: a
plain pointer argument looks the same as one that is only borrowed, while
:c:func:`net_buf_take` shows at the call site that the reference moves.

Passing ``net_buf_take(&buf)`` as an argument only works for calls that
always take ownership, such as :c:func:`k_fifo_put`,
:c:func:`net_buf_slist_put` and :c:func:`net_buf_frag_insert`. When a
function that takes ownership only on success is given a plain pointer,
the caller's pointer is unchanged either way, and only the return value
tells whether the buffer is still its own.

New functions that take ownership should therefore take a pointer to the
caller's buffer pointer, and move the reference out with
:c:func:`net_buf_take` when they take it. The caller's pointer is then
``NULL`` exactly when ownership has moved, and :c:func:`net_buf_drop` is
correct after the call whatever the outcome:

.. code-block:: c

   int foo_send(struct foo *foo, struct net_buf **buf)
   {
       if (!foo->ready) {
           return -EAGAIN;
       }

       k_fifo_put(&foo->tx_queue, net_buf_take(buf));

       return 0;
   }

   err = foo_send(foo, &buf);
   if (err != 0) {
       LOG_WRN("Not sent (err %d)", err);
   }

   /* Releases the buffer only if foo_send() did not take it */
   net_buf_drop(&buf);

Changing an existing function to this form breaks its callers, so it is
best done when the function is reworked anyway.


API Reference
*************

.. doxygengroup:: net_buf
