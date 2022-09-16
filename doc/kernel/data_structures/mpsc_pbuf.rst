.. _mpsc_pbuf:

Multi Producer Single Consumer Packet Buffer
============================================

A :dfn:`Multi Producer Single Consumer Packet Buffer (MPSC_PBUF)` is a circular
buffer, whose contents are stored in first-in-first-out order. Variable size
packets are stored in the buffer. Packet buffer works under assumption that there
is a single context that consumes the data. However, it is possible that another
context may interfere to flush the data and never come back (panic case).
Packet is produced in two steps: first requested amount of data is allocated,
producer fills the data and commits it. Consuming a packet is also performed in
two steps: consumer claims the packet, gets pointer to it and length and later
on packet is freed. This approach reduces memory copying.

A :dfn:`MPSC Packet Buffer` has the following key properties:

* Allocate, commit scheme used for packet producing.
* Claim, free scheme used for packet consuming.
* Allocator ensures that continue memory of requested length is allocated.
* Following policies can be applied when requested space cannot be allocated:

  * **Overwrite** - oldest entries are dropped until requested amount of memory can
    be allocated. For each dropped packet user callback is called.
  * **No overwrite** - When requested amount of space cannot be allocated,
    allocation fails.
* Dedicated, optimized API for storing short packets.
* Allocation with timeout.

Internals
---------

Each packet in the buffer contains ``MPSC_PBUF`` specific header which is used
for internal management. Header consists of 2 bit flags. In order to optimize
memory usage, header can be added on top of the user header using
:c:macro:`MPSC_PBUF_HDR` and remaining bits in the first word can be application
specific. Header consists of following flags:

* valid - bit set to one when packet contains valid user packet
* busy - bit set when packet is being consumed (claimed but not free)

Header state:

+-------+------+----------------------+
| valid | busy | description          |
+-------+------+----------------------+
| 0     | 0    | space is free        |
+-------+------+----------------------+
| 1     | 0    | valid packet         |
+-------+------+----------------------+
| 1     | 1    | claimed valid packet |
+-------+------+----------------------+
| 0     | 1    | internal skip packet |
+-------+------+----------------------+

Packet buffer space contains free space, valid user packets and internal skip
packets. Internal skip packets indicates padding, e.g. at the of the buffer.

Allocation
^^^^^^^^^^

Using pairs for read and write indexes, available space is determined. If
space can be allocated, temporary write index is moved and pointer to a space
within buffer is returned. Packet header is reset. If allocation required
wrapping of the write index, a skip packet is added to the end of buffer. If
space cannot be allocated and overwrite is disabled then ``NULL`` pointer is
returned or context blocks if allocation was with timeout.

Allocation with overwrite
^^^^^^^^^^^^^^^^^^^^^^^^^

If overwrite is enabled, oldest packets are dropped until requested amount of
space can be allocated. When packets are dropped ``busy`` flag is checked in the
header to ensure that currently consumed packet is not overwritten. In that case,
skip packet is added before busy packet and packets following the busy packet
are dropped. When busy packet is being freed, such situation is detected and
packet is converted to skip packet to avoid double processing.

Usage
-----

Packet header definition
^^^^^^^^^^^^^^^^^^^^^^^^

Packet header details can be found in :zephyr_file:`include/zephyr/sys/mpsc_packet.h`.
API functions can be found in :zephyr_file:`include/zephyr/sys/mpsc_pbuf.h`. Headers
are split to avoid include spam when declaring the packet.

User header structure must start with internal header:

.. code-block:: c

   #include <zephyr/sys/mpsc_packet.h>

   struct foo_header {
           MPSC_PBUF_HDR;
           uint32_t length: 32 - MPSC_PBUF_HDR_BITS;
   };

Packet buffer configuration
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Configuration structure contains buffer details, configuration flags and
callbacks. Following callbacks are used by the packet buffer:

* Drop notification - callback called whenever a packet is dropped due to
  overwrite.
* Get packet length - callback to determine packet length

Packet producing
^^^^^^^^^^^^^^^^

Standard, two step method:

.. code-block:: c

   foo_packet *packet = mpsc_pbuf_alloc(buffer, len, K_NO_WAIT);

   fill_data(packet);

   mpsc_pbuf_commit(buffer, packet);

Performance optimized storing of small packets:

* 32 bit word packet
* 32 bit word with pointer packet

Note that since packets are written by value, they should already contain
``valid`` bit set in the header.

.. code-block:: c

   mpsc_pbuf_put_word(buffer, data);
   mpsc_pbuf_put_word_ext(buffer, data, ptr);

Packet consuming
^^^^^^^^^^^^^^^^

Two step method:

.. code-block:: c

   foo_packet *packet = mpsc_pbuf_claim(buffer);

   process(packet);

   mpsc_pbuf_free(buffer, packet);
