.. _ring_buffers_v2:

Ring Buffers
############

A :dfn:`ring buffer` is a circular buffer, whose contents are stored in
first-in-first-out order. Two content data modes are supported:

* **Data item mode**: Multiple 32-bit word data items with metadata
  can be enqueued and dequeued from the ring buffer in
  chunks of up to 1020 bytes.  Each data item also has two
  associated metadata values:  a type identifier and a 16-bit
  integer value, both of which are application-specific.

* **Byte mode**: raw bytes can be enqueued and dequeued.

.. contents::
    :local:
    :depth: 2

Concepts
********

Any number of ring buffers can be defined. Each ring buffer is referenced
by its memory address.

A ring buffer has the following key properties:

* A **data buffer** of 32-bit words or bytes. The data buffer contains the data
  items or raw bytes that have been added to the ring buffer but not yet
  removed.

* A **data buffer size**, measured in 32-bit words or bytes. This governs the
  maximum amount of data (including metadata values) the ring buffer can hold.

A ring buffer must be initialized before it can be used. This sets its
data buffer to empty.

Data item mode
==============

A **data item mode** ring buffer instance is declared using
:cpp:func:`RING_BUF_ITEM_DECLARE_POW2()` or
:cpp:func:`RING_BUF_ITEM_DECLARE_SIZE()` and accessed using
:cpp:func:`ring_buf_item_put()` and :cpp:func:`ring_buf_item_get()`.

A ring buffer **data item** is an array of 32-bit words from 0 to 1020 bytes
in length. When a data item is **enqueued** (:cpp:func:`ring_buf_item_put()`)
its contents are copied to the data buffer, along with its associated metadata
values (which occupy one additional 32-bit word). If the ring buffer has
insufficient space to hold the new data item the enqueue operation fails.

A data items is **dequeued** (:cpp:func:`ring_buf_item_get()`) from a ring
buffer by removing the oldest enqueued item. The contents of the dequeued data
item, as well as its two metadata values, are copied to areas supplied by the
retriever. If the ring buffer is empty, or if the data array supplied by the
retriever is not large enough to hold the data item's data, the dequeue
operation fails.

Byte mode
=========

A **byte mode** ring buffer instance is declared using
:cpp:func:`RING_BUF_DECLARE_SIZE()` and accessed using:
:cpp:func:`ring_buf_put_claim()`, :cpp:func:`ring_buf_put_finish()`,
:cpp:func:`ring_buf_get_claim()`, :cpp:func:`ring_buf_get_finish()`,
:cpp:func:`ring_buf_put()` and :cpp:func:`ring_buf_get()`.

Data can be copied into the ring buffer (see
:cpp:func:`ring_buf_put()`) or ring buffer memory can be used
directly by the user. In the latter case, the operation is split into three stages:

1. allocating the buffer (:cpp:func:`ring_buf_put_claim()`) when
   user requests the destination location where data can be written.
#. writing the data by the user (e.g. buffer written by DMA).
#. indicating the amount of data written to the provided buffer
   (:cpp:func:`ring_buf_put_finish()`). The amount
   can be less than or equal to the allocated amount.

Data can be retrieved from a ring buffer through copying
(see :cpp:func:`ring_buf_get()`) or accessed directly by address. In the latter
case, the operation is split
into three stages:

1. retrieving source location with valid data written to a ring buffer
   (see :cpp:func:`ring_buf_get_claim()`).
#. processing data
#. freeing processed data (see :cpp:func:`ring_buf_get_finish()`).
   The amount freed can be less than or equal or to the retrieved amount.

Concurrency
===========

The ring buffer APIs do not provide any concurrency control.
Depending on usage (particularly with respect to number of concurrent
readers/writers) applications may need to protect the ring buffer with
mutexes and/or use semaphores to notify consumers that there is data to
read.

For the trivial case of one producer and one consumer, concurrency
shouldn't be needed.

Internal Operation
==================

The ring buffer always maintains an empty 32-bit word (byte in bytes mode) in
its data buffer to allow it to distinguish between empty and full states.

If the size of the data buffer is a power of two, the ring buffer
uses efficient masking operations instead of expensive modulo operations
when enqueuing and dequeuing data items. This option is applicable only for
data item mode.

Implementation
**************

Defining a Ring Buffer
======================

A ring buffer is defined using a variable of type :c:type:`struct ring_buf`.
It must then be initialized by calling :cpp:func:`ring_buf_init()`.

The following code defines and initializes an empty **data item mode** ring
buffer (which is part of a larger data structure). The ring buffer's data buffer
is capable of holding 64 words of data and metadata information.

.. code-block:: c

    #define MY_RING_BUF_SIZE 64

    struct my_struct {
        struct ring_buf rb;
        u32_t buffer[MY_RING_BUF_SIZE];
        ...
    };
    struct my_struct ms;

    void init_my_struct {
        ring_buf_init(&ms.rb, sizeof(ms.buffer), ms.buffer);
        ...
    }

Alternatively, a ring buffer can be defined and initialized at compile time
using one of two macros at file scope. Each macro defines both the ring
buffer itself and its data buffer.

The following code defines a ring buffer with a power-of-two sized data buffer,
which can be accessed using efficient masking operations.

.. code-block:: c

    /* Buffer with 2^8 (or 256) words */
    RING_BUF_ITEM_DECLARE_POW2(my_ring_buf, 8);

The following code defines an application-specific sized **byte mode** ring
buffer enqueued and dequeued as raw bytes:

.. code-block:: c

    #define MY_RING_BUF_WORDS 93
    RING_BUF_ITEM_DECLARE_SIZE(my_ring_buf, MY_RING_BUF_WORDS);

The following code defines a ring buffer with an arbitrary-sized data buffer,
which can be accessed using less efficient modulo operations. Ring buffer is
intended to be used for raw bytes.

.. code-block:: c

    #define MY_RING_BUF_BYTES 93
    RING_BUF_DECLARE_SIZE(my_ring_buf, MY_RING_BUF_BYTES);

Enqueuing Data
==============

A data item is added to a ring buffer by calling
:cpp:func:`ring_buf_item_put()`.

.. code-block:: c

    u32_t data[MY_DATA_WORDS];
    int ret;

    ret = ring_buf_item_put(&ring_buf, TYPE_FOO, 0, data, SIZE32_OF(data));
    if (ret == -EMSGSIZE) {
        /* not enough room for the data item */
	...
    }

If the data item requires only the type or application-specific integer value
(i.e. it has no data array), a size of 0 and data pointer of :c:macro:`NULL`
can be specified.

.. code-block:: c

    int ret;

    ret = ring_buf_item_put(&ring_buf, TYPE_BAR, 17, NULL, 0);
    if (ret == -EMSGSIZE) {
        /* not enough room for the data item */
	...
    }

Bytes are copied to a **byte mode** ring buffer by calling
:cpp:func:`ring_buf_put()`.

.. code-block:: c

    u8_t my_data[MY_RING_BUF_BYTES];
    u32_t ret;

    ret = ring_buf_put(&ring_buf, my_data, SIZE_OF(my_data));
    if (ret != SIZE_OF(my_data)) {
        /* not enough room, partial copy. */
	...
    }

Data can be added to a **byte mode** ring buffer by directly accessing the
ring buffer's memory.  For example:

.. code-block:: c

    u32_t size;
    u32_t rx_size;
    u8_t *data;
    int err;

    /* Allocate buffer within a ring buffer memory. */
    size = ring_buf_put_claim(&ring_buf, &data, MY_RING_BUF_BYTES);

    /* Work directly on a ring buffer memory. */
    rx_size = uart_rx(data, size);

    /* Indicate amount of valid data. rx_size can be equal or less than size. */
    err = ring_buf_put_finish(&ring_buf, rx_size);
    if (err != 0) {
        /* No space to put requested amount of data to ring buffer. */
	...
    }

Retrieving Data
===============

A data item is removed from a ring buffer by calling
:cpp:func:`ring_buf_item_get()`.

.. code-block:: c

    u32_t my_data[MY_DATA_WORDS];
    u16_t my_type;
    u8_t  my_value;
    u8_t  my_size;
    int ret;

    my_size = SIZE32_OF(my_data);
    ret = ring_buf_item_get(&ring_buf, &my_type, &my_value, my_data, &my_size);
    if (ret == -EMSGSIZE) {
        printk("Buffer is too small, need %d u32_t\n", my_size);
    } else if (ret == -EAGAIN) {
        printk("Ring buffer is empty\n");
    } else {
        printk("Got item of type %u value &u of size %u dwords\n",
               my_type, my_value, my_size);
        ...
    }

Data bytes are copied out from a **byte mode** ring buffer by calling
:cpp:func:`ring_buf_get()`. For example:

.. code-block:: c

    u8_t my_data[MY_DATA_BYTES];
    size_t  ret;

    ret = ring_buf_get(&ring_buf, my_data, sizeof(my_data));
    if (ret != sizeof(my_size)) {
        /* Less bytes copied. */
    } else {
        /* Requested amount of bytes retrieved. */
        ...
    }

Data can be retrieved from a **byte mode** ring buffer by direct
operations on the ring buffer's memory.  For example:

.. code-block:: c

    u32_t size;
    u32_t proc_size;
    u8_t *data;
    int err;

    /* Get buffer within a ring buffer memory. */
    size = ring_buf_get_claim(&ring_buf, &data, MY_RING_BUF_BYTES);

    /* Work directly on a ring buffer memory. */
    proc_size = process(data, size);

    /* Indicate amount of data that can be freed. proc_size can be equal or less
     * than size.
     */
    err = ring_buf_get_finish(&ring_buf, proc_size);
    if (err != 0) {
        /* proc_size exceeds amount of valid data in a ring buffer. */
	...
    }

API Reference
*************

The following ring buffer APIs are provided by :zephyr_file:`include/ring_buffer.h`:

.. doxygengroup:: ring_buffer_apis
   :project: Zephyr
