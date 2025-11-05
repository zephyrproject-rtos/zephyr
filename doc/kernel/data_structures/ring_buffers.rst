.. _ring_buffers_v2:

Ring Buffers
############

A :dfn:`ring buffer` is a circular buffer, whose contents are stored in
first-in-first-out order.

For circumstances where an application needs to implement asynchronous
"streaming" copying of data, Zephyr provides a ``struct ring_buf``
abstraction to manage copies of such data in and out of a shared
buffer of memory.

.. contents::
    :local:
    :depth: 2

Concepts
********

Any number of ring buffers can be defined (limited only by available RAM). Each
ring buffer is referenced by its memory address.

A ring buffer has the following key properties:

* A **data buffer** of bytes. The data buffer contains the raw
  bytes that have been added to the ring buffer but not yet
  removed.

* A **data buffer size**, measured in bytes. This governs
  the maximum amount of data the ring buffer can hold.

A ring buffer must be initialized before it can be used. This sets its
data buffer to empty.

A ``struct ring_buf`` may be placed anywhere in user-accessible
memory, and must be initialized with :c:func:`ring_buf_init` before use. This must be provided a region
of user-controlled memory for use as the buffer itself.  Note carefully that the units of the size of the
buffer passed change (either bytes or words) depending on how the ring
buffer will be used later.  Macros for combining these steps in a
single static declaration exist for convenience.
:c:macro:`RING_BUF_DECLARE` will declare and statically initialize a ring
buffer with a specified byte count, where

"Bytes" data may be copied into the ring buffer using
:c:func:`ring_buf_put`, passing a data pointer and byte count.  These
bytes will be copied into the buffer in order, as many as will fit in
the allocated buffer.  The total number of bytes copied (which may be
fewer than provided) will be returned.  Likewise :c:func:`ring_buf_get`
will copy bytes out of the ring buffer in the order that they were
written, into a user-provided buffer, returning the number of bytes
that were transferred.

To avoid multiply-copied-data situations, a "claim" API exists.
:c:func:`ring_buf_put_claim` takes a byte size value from the
user and returns a pointer to memory internal to the ring buffer that
can be used to receive those bytes, along with a size of the
contiguous internal region (which may be smaller than requested).  The
user can then copy data into that region at a later time without
assembling all the bytes in a single region first.  When complete,
:c:func:`ring_buf_put_finish` can be used to signal the buffer that the
transfer is complete, passing the number of bytes actually
transferred.  At this point a new transfer can be initiated.
Similarly, :c:func:`ring_buf_get_claim` returns a pointer to internal ring
buffer data from which the user can read without making a verbatim
copy, and :c:func:`ring_buf_get_finish` signals the buffer with how many
bytes have been consumed and allows for a new transfer to begin.

The user can manage the capacity of a ring buffer without modifying it
using either :c:func:`ring_buf_space_get` which returns the number of free bytes,
or by testing the :c:func:`ring_buf_is_empty` predicate.

Finally, a :c:func:`ring_buf_reset` call exists to immediately empty a
ring buffer, discarding the tracking of any bytes already
written to the buffer.  It does not modify the memory contents of the
buffer itself, however.


Instantiation and Usage
=======================

A ring buffer instance is declared using
:c:macro:`RING_BUF_DECLARE()` and accessed using:
:c:func:`ring_buf_put_claim`, :c:func:`ring_buf_put_finish`,
:c:func:`ring_buf_get_claim`, :c:func:`ring_buf_get_finish`,
:c:func:`ring_buf_put` and :c:func:`ring_buf_get`.

Data can be copied into the ring buffer (see
:c:func:`ring_buf_put`) or ring buffer memory can be used
directly by the user. In the latter case, the operation is split into three stages:

1. allocating the buffer (:c:func:`ring_buf_put_claim`) when
   user requests the destination location where data can be written.
#. writing the data by the user (e.g. buffer written by DMA).
#. indicating the amount of data written to the provided buffer
   (:c:func:`ring_buf_put_finish`). The amount
   can be less than or equal to the allocated amount.

Data can be retrieved from a ring buffer through copying
(see :c:func:`ring_buf_get`) or accessed directly by address. In the latter
case, the operation is split into three stages:

1. retrieving source location with valid data written to a ring buffer
   (see :c:func:`ring_buf_get_claim`).
#. processing data
#. freeing processed data (see :c:func:`ring_buf_get_finish`).
   The amount freed can be less than or equal or to the retrieved amount.

Concurrency
===========

The ring buffer APIs do not provide any concurrency control.
Depending on usage (particularly with respect to number of concurrent
readers/writers) applications may need to protect the ring buffer with
mutexes and/or use semaphores to notify consumers that there is data to
read.

For the trivial case of one producer and one consumer, concurrency
control shouldn't be needed.

Internal Operation
==================

Data streamed through a ring buffer is always written to the next byte
within the buffer, wrapping around to the first element after reaching
the end, thus the "ring" structure.  Internally, the ``struct
ring_buf`` contains its own buffer pointer and its size, and also a
set of "head" and "tail" indices representing where the next read and write
operations may occur.

This boundary is invisible to the user using the normal put/get APIs,
but becomes a barrier to the "claim" API, because obviously no
contiguous region can be returned that crosses the end of the buffer.
This can be surprising to application code, and produce performance
artifacts when transfers need to happen close to the end of the
buffer, as the number of calls to claim/finish needs to double for such
transfers.


Implementation
**************

Defining a Ring Buffer
======================

A ring buffer is defined using a variable of type :c:struct:`ring_buf`.
It must then be initialized by calling :c:func:`ring_buf_init`.

A ring buffer can be defined and initialized at compile time
using the macros at file scope. The macro defines the ring buffer
itself and its data buffer.

The following code defines a ring buffer:

.. code-block:: c

    #define MY_RING_BUF_BYTES 93
    RING_BUF_DECLARE(my_ring_buf, MY_RING_BUF_BYTES);

Enqueuing Data
==============

Bytes are copied to a ring buffer by calling
:c:func:`ring_buf_put`.

.. code-block:: c

    uint8_t my_data[MY_RING_BUF_BYTES];
    uint32_t ret;

    ret = ring_buf_put(&ring_buf, my_data, MY_RING_BUF_BYTES);
    if (ret != MY_RING_BUF_BYTES) {
        /* not enough room, partial copy. */
	...
    }

Data can be added to a ring buffer by directly accessing the
ring buffer's memory.  For example:

.. code-block:: c

    uint32_t size;
    uint32_t rx_size;
    uint8_t *data;
    int err;

    /* Allocate buffer within a ring buffer memory. */
    size = ring_buf_put_claim(&ring_buf, &data, MY_RING_BUF_BYTES);

    /* Work directly on a ring buffer memory. */
    rx_size = uart_rx(data, size);

    /* Indicate amount of valid data. rx_size can be equal or less than size. */
    err = ring_buf_put_finish(&ring_buf, rx_size);
    if (err != 0) {
        /* This shouldn't happen unless rx_size > size */
	...
    }


Retrieving Data
===============

Data bytes are copied out from a ring buffer by calling
:c:func:`ring_buf_get`. For example:

.. code-block:: c

    uint8_t my_data[MY_DATA_BYTES];
    size_t  ret;

    ret = ring_buf_get(&ring_buf, my_data, sizeof(my_data));
    if (ret != sizeof(my_data)) {
        /* Fewer bytes copied. */
    } else {
        /* Requested amount of bytes retrieved. */
        ...
    }

Data can be retrieved from a ring buffer by direct
operations on the ring buffer's memory.  For example:

.. code-block:: c

    uint32_t size;
    uint32_t proc_size;
    uint8_t *data;
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

Configuration Options
*********************

Related configuration options:

* :kconfig:option:`CONFIG_RING_BUFFER`: Enable ring buffer.

API Reference
*************

The following ring buffer APIs are provided by :zephyr_file:`include/zephyr/sys/ring_buffer.h`:

.. doxygengroup:: ring_buffer_apis
