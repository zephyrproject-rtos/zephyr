.. _ring_buffers_v2:

Ring Buffers
############

A :dfn:`ring buffer` is a circular buffer of 32-bit words, whose contents
are stored in first-in-first-out order. Data items can be enqueued and dequeued
from a ring buffer in chunks of up to 1020 bytes. Each data item also has
two associated metadata values: a type identifier and a 16-bit integer value,
both of which are application-specific.

.. contents::
    :local:
    :depth: 2

Concepts
********

Any number of ring buffers can be defined. Each ring buffer is referenced
by its memory address.

A ring buffer has the following key properties:

* A **data buffer** of 32-bit words. The data buffer contains the data items
  that have been added to the ring buffer but not yet removed.

* A **data buffer size**, measured in 32-bit words. This governs the maximum
  amount of data (including metadata values) the ring buffer can hold.

A ring buffer must be initialized before it can be used. This sets its
data buffer to empty.

A ring buffer **data item** is an array of 32-bit words from 0 to 1020 bytes
in length. When a data item is **enqueued** its contents are copied
to the data buffer, along with its associated metadata values (which occupy
one additional 32-bit word).
If the ring buffer has insufficient space to hold the new data item
the enqueue operation fails.

A data items is **dequeued** from a ring buffer by removing the oldest
enqueued item. The contents of the dequeued data item, as well as its
two metadata values, are copied to areas supplied by the retriever.
If the ring buffer is empty, or if the data array supplied by the retriever
is not large enough to hold the data item's data, the dequeue operation fails.

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

The ring buffer always maintains an empty 32-bit word in its data buffer
to allow it to distinguish between empty and full states.

If the size of the data buffer is a power of two, the ring buffer
uses efficient masking operations instead of expensive modulo operations
when enqueuing and dequeuing data items.

Implementation
**************

Defining a Ring Buffer
======================

A ring buffer is defined using a variable of type :c:type:`struct ring_buf`.
It must then be initialized by calling :cpp:func:`sys_ring_buf_init()`.

The following code defines and initializes an empty ring buffer
(which is part of a larger data structure). The ring buffer's data buffer
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
        sys_ring_buf_init(&ms.rb, sizeof(ms.buffer), ms.buffer);
        ...
    }

Alternatively, a ring buffer can be defined and initialized at compile time
using one of two macros at file scope. Each macro defines both the ring
buffer itself and its data buffer.

The following code defines a ring buffer with a power-of-two sized data buffer,
which can be accessed using efficient masking operations.

.. code-block:: c

    /* Buffer with 2^8 (or 256) words */
    SYS_RING_BUF_DECLARE_POW2(my_ring_buf, 8);

The following code defines a ring buffer with an arbitrary-sized data buffer,
which can be accessed using less efficient modulo operations.

.. code-block:: c

    #define MY_RING_BUF_WORDS 93
    SYS_RING_BUF_DECLARE_SIZE(my_ring_buf, MY_RING_BUF_WORDS);

Enqueuing Data
==============

A data item is added to a ring buffer by calling :cpp:func:`sys_ring_buf_put()`.

.. code-block:: c

    u32_t my_data[MY_DATA_WORDS];
    int ret;

    ret = sys_ring_buf_put(&ring_buf, TYPE_FOO, 0, my_data, SIZE32_OF(my_data));
    if (ret == -EMSGSIZE) {
        /* not enough room for the data item */
	...
    }

If the data item requires only the type or application-specific integer value
(i.e. it has no data array), a size of 0 and data pointer of :c:macro:`NULL`
can be specified.

.. code-block:: c

    int ret;

    ret = sys_ring_buf_put(&ring_buf, TYPE_BAR, 17, NULL, 0);
    if (ret == -EMSGSIZE) {
        /* not enough room for the data item */
	...
    }

Retrieving Data
===============

A data item is removed from a ring buffer by calling
:cpp:func:`sys_ring_buf_get()`.

.. code-block:: c

    u32_t my_data[MY_DATA_WORDS];
    u16_t my_type;
    u8_t  my_value;
    u8_t  my_size;
    int ret;

    my_size = SIZE32_OF(my_data);
    ret = sys_ring_buf_get(&ring_buf, &my_type, &my_value, my_data, &my_size);
    if (ret == -EMSGSIZE) {
        printk("Buffer is too small, need %d u32_t\n", my_size);
    } else if (ret == -EAGAIN) {
        printk("Ring buffer is empty\n");
    } else {
        printk("Got item of type %u value &u of size %u dwords\n",
               my_type, my_value, my_size);
        ...
    }

APIs
****

The following ring buffer APIs are provided by :file:`include/ring_buffer.h`:

* :cpp:func:`SYS_RING_BUF_DECLARE_POW2()`
* :cpp:func:`SYS_RING_BUF_DECLARE_SIZE()`
* :cpp:func:`sys_ring_buf_init()`
* :cpp:func:`sys_ring_buf_is_empty()`
* :cpp:func:`sys_ring_buf_space_get()`
* :cpp:func:`sys_ring_buf_put()`
* :cpp:func:`sys_ring_buf_get()`
