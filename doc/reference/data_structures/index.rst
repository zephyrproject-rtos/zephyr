.. _data_structures:

Data Structures
###############

Zephyr provides a library of common general purpose data structures
used within the kernel, but useful by application code in general.
These include list and balanced tree structures for storing ordered
data, and a ring buffer for managing "byte stream" data in a clean
way.

Note that in general, the collections are implemented as "intrusive"
data structures.  The "node" data is the only struct used by the
library code, and it does not store a pointer or other metadata to
indicate what user data is "owned" by that node.  Instead, the
expectation is that the node will be itself embedded within a
user-defined struct.  Macros are provided to retrieve a user struct
address from the embedded node pointer in a clean way.  The purpose
behind this design is to allow the collections to be used in contexts
where dynamic allocation is disallowed (i.e. there is no need to
allocate node objects because the memory is provided by the user).

Note also that these libraries are uniformly unsynchronized; access to
them is not threadsafe by default.  These are data structures, not
synchronization primitives.  The expectation is that any locking
needed will be provided by the user.

.. toctree::
  :maxdepth: 1

  slist.rst
  dlist.rst
  rbtree.rst


Ring Buffer
===========

For circumstances where an application needs to implement asynchronous
"streaming" copying of data, Zephyr provides a ``struct ring_buf``
abstraction to manage copies of such data in and out of a shared
buffer of memory.  Ring buffers may be used in either "bytes" mode,
where the data to be streamed is an uninterpreted array of bytes, or
"items" mode where the data much be an integral number of 32 bit
words.  While the underlying data structure is the same, it is not
legal to mix these two modes on a single ring buffer instance.  A ring
buffer initialized with a byte count must be used only with the
"bytes" API, one initialized with a word count must use the "items"
calls.

A ``struct ring_buf`` may be placed anywhere in user-accessible
memory, and must be initialized with ``ring_buf_init()`` before use.
This must be provided a region of user-controlled memory for use as
the buffer itself.  Note carefully that the units of the size of the
buffer passed change (either bytes or words) depending on how the ring
buffer will be used later.  Macros for combining these steps in a
single static declaration exist for convenience.
``RING_BUF_DECLARE()`` will declare and statically initialize a ring
buffer with a specified byte count, where
``RING_BUF_ITEM_DECLARE_SIZE()`` will declare and statically
initialize a buffer with a given count of 32 bit words.
``RING_BUF_ITEM_DECLARE_POW2()`` can be used to initialize an
items-mode buffer with a memory region guaranteed to be a power of
two, which enables various optimizations internal to the
implementation.  No power-of-two initialization is available for
bytes-mode ring buffers.

"Bytes" data may be copied into the ring buffer using
``ring_buf_put()``, passing a data pointer and byte count.  These
bytes will be copied into the buffer in order, as many as will fit in
the allocated buffer.  The total number of bytes copied (which may be
fewer than provided) will be returned.  Likewise ``ring_buf_get()``
will copy bytes out of the ring buffer in the order that they were
written, into a user-provided buffer, returning the number of bytes
that were transferred.

To avoid multiply-copied-data situations, a "claim" API exists for
byte mode.  ``ring_buf_put_claim()`` takes a byte size value from the
user and returns a pointer to memory internal to the ring buffer that
can be used to receive those bytes, along with a size of the
contiguous internal region (which may be smaller than requested).  The
user can then copy data into that region at a later time without
assembling all the bytes in a single region first.  When complete,
``ring_buf_put_finish()`` can be used to signal the buffer that the
transfer is complete, passing the number of bytes actually
transferred.  At this point a new transfer can be initiated.
Similarly, ``ring_buf_get_claim()`` returns a pointer to internal ring
buffer data from which the user can read without making a verbatim
copy, and ``ring_buf_get_finish()`` signals the buffer with how many
bytes have been consumed and allows for a new transfer to begin.

"Items" mode works similarly to bytes mode, except that all transfers
are in units of 32 bit words and all memory is assumed to be aligned
on 32 bit boundaries.  The write and read operations are
``ring_buf_item_put()`` and ``ring_buf_item_get()``, and work
otherwise identically to the bytes mode APIs.  There no "claim" API
provided for items mode.  One important difference is that unlike
``ring_buf_put()``, ``ring_buf_item_put()`` will not do a partial
transfer; it will return an error in the case where the provided data
does not fit in its entirety.

The user can manage the capacity of a ring buffer without modifying it
using the ``ring_buf_space_get()`` call (which returns a value of
either bytes or items depending on how the ring buffer has been used),
or by testing the ``ring_buf_is_empty()`` predicate.

Finally, a ``ring_buf_reset()`` call exists to immediately empty a
ring buffer, discarding the tracking of any bytes or items already
written to the buffer.  It does not modify the memory contents of the
buffer itself, however.

Ring Buffer Internals
---------------------

Data streamed through a ring buffer is always written to the next byte
within the buffer, wrapping around to the first element after reaching
the end, thus the "ring" structure.  Internally, the ``struct
ring_buf`` contains its own buffer pointer and its size, and also a
"head" and "tail" index representing where the next read and write

This boundary is invisible to the user using the normal put/get APIs,
but becomes a barrier to the "claim" API, because obviously no
contiguous region can be returned that crosses the end of the buffer.
This can be surprising to application code, and produce performance
artifacts when transfers need to alias closely to the size of the
buffer, as the number of calls to claim/finish need to double for such
transfers.

When running in items mode (only), the ring buffer contains two
implementations for the modular arithmetic required to compute "next
element" offsets.  One is used for arbitrary sized buffers, but the
other is optimized for power of two sizes and can replace the compare
and subtract steps with a simple bitmask in several places, at the
cost of testing the "mask" value for each call.
