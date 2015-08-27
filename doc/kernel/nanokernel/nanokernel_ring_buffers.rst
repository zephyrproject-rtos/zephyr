.. _nanokernel_ring_buffers:

Nanokernel Ring Buffers
#######################

Definition
**********

The ring buffer is defined in :file:`include/misc/ring_buffer.h` and
:file:`kernel/nanokernel/ring_buffer.c`. This is an array-based
circular buffer, stored in first-in-first-out order. The APIs allow
for enqueueing and retrieval of chunks of data up to 1024 bytes in size,
along with two metadata values (type ID and an app-specific integer).

Unlike nanokernel FIFOs, storage of enqueued items and their metadata
is managed in a fixed buffer and there are no preconditions on the data
enqueued (other than the size limit). Since the size annotation is only
an 8-bit value, sizes are expressed in terms of 32-bit chunks.

Internally, the ring buffer always maintains an empty 32-bit block in the
buffer to distinguish between empty and full buffers. Any given entry
in the buffer will use a 32-bit block for metadata plus any data attached.
If the size of the buffer array is a power of two, the ring buffer will
use more efficient masking instead of expensive modulo operations to
maintain itself.

Concurrency
***********

Concurrency control of ring buffers is not implemented at this level.
Depending on usage (particularly with respect to number of concurrent
readers/writers) applications may need to protect the ring buffer with
mutexes and/or use semaphores to notify consumers that there is data to
read.

For the trivial case of one producer and one consumer, concurrency
shouldn't be needed.

Example: Initializing a Ring Buffer
===================================

There are three ways to initialize a ring buffer. The first two are through use
of macros which defines one (and an associated private buffer) in file scope.
You can declare a fast ring buffer that uses mask operations by declaring
a power-of-two sized buffer:

.. code-block:: c

    /* Buffer with 2^8 or 256 elements */
    SYS_RING_BUF_DECLARE_POW2(my_ring_buf, 8);

Arbitrary-sized buffers may also be declared with a different macro, but
these will always be slower due to use of modulo operations:

.. code-block:: c

    #define MY_RING_BUF_SIZE	93
    SYS_RING_BUF_DECLARE_SIZE(my_ring_buf, MY_RING_BUF_SIZE);


Alternatively, a ring buffer may be initialized manually. Whether the buffer
will use modulo or mask operations will be detected automatically:

.. code-block:: c
    #define MY_RING_BUF_SIZE	64

    struct my_struct {
        struct ring_buffer rb;
        uint32_t buffer[MY_RING_BUF_SIZE];
        ...
    };
    struct my_struct ms;

    void init_my_struct {
        sys_ring_buf_init(&ms.rb, sizeof(ms.buffer), ms.buffer);
        ...
    }

Example: Enqueuing data
=======================

.. code-block:: c
    int ret;

    ret = sys_ring_buf_put(&ring_buf, TYPE_FOO, 0, &my_foo, SIZE32_OF(my_foo));
    if (ret == -EMSGSIZE) {
        ... not enough room for the message ..
    }

If the type or value fields are sufficient, the data pointer and size may be 0.

.. code-block:: c
    int ret;

    ret = sys_ring_buf_put(&ring_buf, TYPE_BAR, 17, NULL, 0);
    if (ret == -EMSGSIZE) {
        ... not enough room for the message ..
    }

Example: Retrieving data
========================

.. code-block:: c

    int ret;
    uint32_t data[6];

    size = SIZE32_OF(data);
    ret = sys_ring_buf_get(&ring_buf, &type, &value, data, &size);
    if (ret == -EMSGSIZE) {
        printk("Buffer is too small, need %d uint32_t\n", size);
    } else if (ret == -EAGAIN) {
        printk("Ring buffer is empty\n");
    } else {
        printk("got item of type %u value &u of size %u dwords\n",
               type, value, size);
        ...
    }

APIs
****

The following APIs for ring buffers are provided by :file:`ring_buffer.h`.

+------------------------------------------------+------------------------------------+
| Call                                           | Description                        |
+================================================+====================================+
| :c:func:`sys_ring_buf_init()`                  | Initialize a ring buffer.          |
+------------------------------------------------+------------------------------------+
| :c:func:`SYS_RING_BUF_DECLARE_POW2()`          | Declare and init a file-scope      |
| :c:func:`SYS_RING_BUF_DECLARE_SIZE()`          | ring buffer.                       |
+------------------------------------------------+------------------------------------+
| :c:func:`sys_ring_buf_get_space()`             | Return the amount of free buffer   |
|                                                | storage space in 32-bit dwords     |
+------------------------------------------------+------------------------------------+
| :c:func:`sys_ring_buf_is_empty()`              | Indicate whether a buffer is empty |
+------------------------------------------------+------------------------------------+
| :c:func:`sys_ring_buf_put()`                   | Enqueue an item                    |
+------------------------------------------------+------------------------------------+
| :c:func:`sys_ring_buf_get()`                   | De-queue an item                   |
+------------------------------------------------+------------------------------------+


