.. _ring_buffers_v2:

Ring Buffers
############

A :dfn:`ring buffer` is a circular buffer that stores bytes in a first-in, first-out (FIFO) order.

.. contents::
    :local:
    :depth: 2

Concepts
********

An arbitrary number of ring buffers can be defined, limited only by the available RAM. Each ring
buffer is identified by its memory address.

A :c:type:`ring_buffer` can reside anywhere in user-accessible memory but must be initialized using
:c:func:`ring_buffer_init` before it can be used.

To simplify setup, macros are provided for defining both the ring buffer and its associated memory.
The :c:macro:`RING_BUFFER_DEFINE` macro declares and statically initializes a ring buffer with the
specified size.

Data can be written to the ring buffer using :c:func:`ring_buffer_write`. Bytes are copied into the
buffer in order, up to the amount that will fit. The function returns the number of bytes actually
written, which may be less than requested.

Similarly, :c:func:`ring_buffer_read` copies bytes out of the buffer in the order they were written
and returns the number of bytes successfully read.
:c:func:`ring_buffer_peek` allows data to be read from the buffer without consuming it, so that it
can be read again later.

To avoid unnecessary data copying, users can manage the ring buffer directly. The
:c:func:`ring_buffer_write_ptr` function returns a pointer to a contiguous region of internal buffer
memory, along with its size. This allows data to be written directly into the buffer without first
assembling it in a separate memory region. After writing, the user must call
:c:func:`ring_buffer_commit` to indicate how many bytes were actually written. At that point, a new
transfer can begin using either :c:func:`ring_buffer_write` or another call to
:c:func:`ring_buffer_write_ptr`.

Likewise, :c:func:`ring_buffer_read_ptr` provides a pointer to data stored in the buffer, enabling the
user to read directly without making a full copy. Once the data has been consumed,
:c:func:`ring_buffer_consume` should be called to notify the buffer of how many bytes were read.

The user can querry the capacity of a ring buffer without modifying it using the following functions:
* :c:func:`ring_buffer_capacity` - returns the size of the ring buffer in bytes
* :c:func:`ring_buffer_size` - returns the number of bytes currently stored in the ring buffer
* :c:func:`ring_buf_space` - returns the number of bytes available for writing
* :c:func:`ring_buffer_full` - indicates whether the ring buffer is full
* :c:func:`ring_buffer_empty` - indicates whether the ring buffer is empty

Finally, a :c:func:`ring_buffer_reset` call exists to immediately empty a
ring buffer, discarding any bytes already written to the buffer.

A ring buffer instance can be defined using :c:macro:`RING_BUFFER_DEFINE()`.

Data can be copied into the ring buffer (see :c:func:`ring_buffer_write`) or accessing the ring
buffer memory directly. In the latter case, the operation is broken into three steps:

1. Retrieving the pointer and size of contiguous free space within the ring buffer (see
   :c:func:`ring_buffer_write_ptr`).
#. writing the data by the user (e.g. buffer written by DMA).
#. indicating the amount of data written to the ring buffer
   (:c:func:`ring_buffer_commit`). The amount can be less than or equal to the allocated amount.

Similarly, data can be read from the ring buffer by copying it out with :c:func:`ring_buffer_read`,
or by accessing the buffer memory directly. Direct access also involves three steps:

1. Retrieving the pointer and size of contiguous allocated data within the ring buffer
   (see :c:func:`ring_buffer_read_ptr`).
#. processing data
#. freeing processed data (see :c:func:`ring_buffer_consume`).
   The amount freed can be less than or equal or to the retrieved amount.

Concurrency
===========

The ring buffer API do not provide any concurrency control, if multiple threads
access the same ring buffer, the application should provide concurrency control.

Example usage
*************

Defining a Ring Buffer
======================

A ring buffer is defined using a variable of type :c:type:`ring_buffer`.
It must then be initialized by calling :c:func:`ring_buffer_init`;

A ring buffer can be defined and initialized at compile time
using the following macro at file scope. The macro defines both the ring
buffer itself and its data buffer.

.. code-block:: c

    #define MY_RING_BUF_BYTES 93
    RING_BUFFER_DEFINE(my_ring_buf, MY_RING_BUF_BYTES);

Writing Data to the Ring Buffer
===============================

:c:func:`ring_buffer_write`.

.. code-block:: c

    uint8_t my_data[MY_RING_BUF_BYTES];
    size_t written;

    written = ring_buf_write(&ring_buf, my_data, MY_RING_BUF_BYTES);
    if (written != MY_RING_BUF_BYTES) {
        /* not enough room, partial copy. */
	...
    }

Data can be added to a ring buffer by directly accessing its internal memory:

.. code-block:: c

    uint32_t writable_size;
    uint32_t rx_size;
    uint8_t *data;

    /* Retrives a pointer to writable space within the ring buffer and returns the size of the
     * writable space.
     */
    writable_size = ring_buffer_write_ptr(&ring_buf, &data);

    /* Work directly on a ring buffer memory. */
    rx_size = uart_rx(data, writable_size);

    /* Indicate amount of data that has been committed into the ring_buffer.
     * rx_size can be equal or less than size.
     */
    ring_buffer_commit(&ring_buf, rx_size);
	...
    }


Reading Data from the Ring Buffer
=================================

.. code-block:: c

    uint8_t my_data[MY_DATA_BYTES];
    size_t bytes_read;

    bytes_read = ring_buffer_read(&ring_buf, my_data, sizeof(my_data));
    if (bytes_read != sizeof(my_data)) {
        /* Fewer bytes copied. */
    } else {
        /* Requested amount of bytes retrieved. */
        ...
    }

Data can be retrieved from a ring buffer by directly accessing its internal memory:

.. code-block:: c

    size_t readable_size;
    size_t consumed;
    uint8_t *data;

    /* Get a reference to readable space within a ring buffer memory and the readable spaces size.*/
    readable_size = ring_buf_read_ptr(&ring_buf, &data);

    /* Work directly on a ring buffer memory. */
    consumed = uart_tx(data, readable_size);

    /* Indicate amount of data that can be freed. proc_size can be equal or less than size. */
    ring_buffer_consume(&ring_buf, consumed);

Alternatively, data could be observed without consuming it by using :c:func:`ring_buffer_peek`:

.. code-block:: c

    uint8_t my_data[MY_DATA_BYTES];
    size_t bytes_peeked;

    bytes_peeked = ring_buffer_peek(&ring_buf, my_data, sizeof(my_data));
    if (bytes_peeked != sizeof(my_data)) {
        /* Fewer bytes available than requested */
    } else {
        /* Requested amount of bytes retrieved. */
        ...
    }

API Reference
*************

The following ring buffer APIs are provided by :zephyr_file:`include/zephyr/sys/ring_buffer.h`:

.. doxygengroup:: ring_buffer_apis
