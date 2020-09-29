.. _net_pkt_interface:

Packet Management
#################

.. contents::
    :local:
    :depth: 2

Overview
********

Network packets are the main data the networking stack manipulates.
Such data is represented through the net_pkt structure which provides
a means to hold the packet, write and read it, as well as necessary
metadata for the core to hold important information. Such an object is
called net_pkt in this document.

The data structure and the whole API around it are defined in
:zephyr_file:`include/net/net_pkt.h`.

Architectural notes
===================

There are two network packets flows within the stack, **TX** for the
transmission path, and **RX** for the reception one. In both paths,
each net_pkt is written and read from the beginning to the end, or
more specifically from the headers to the payload.


Memory management
*****************

Allocation
==========

All net_pkt objects come from a pre-defined pool of struct net_pkt.
Such pool is defined via

.. code-block:: c

    NET_PKT_SLAB_DEFINE(name, count)

Note, however, one will rarely have to use it, as the core provides
already two pools, one for the TX path and one for the RX path.

Allocating a raw net_pkt can be done through:

.. code-block:: c

    pkt = net_pkt_alloc(timeout);

However, by its nature, a raw net_pkt is useless without a buffer and
needs various metadata information to become relevant as well.  It
requires at least to get the network interface it is meant to be sent
through or through which it was received. As this is a very common
operation, a helper exist:

.. code-block:: c

    pkt = net_pkt_alloc_on_iface(iface, timeout);

A more complete allocator exists, where both the net_pkt and its buffer
can be allocated at once:

.. code-block:: c

    pkt = net_pkt_alloc_with_buffer(iface, size, family, proto, timeout);

See below how the buffer is allocated.


Buffer allocation
=================

The net_pkt object does not define its own buffer, but instead uses an
existing object for this: :c:struct:`net_buf`. (See
:ref:`net_buf_interface` for more information). However, it mostly
hides the usage of such a buffer because net_pkt brings network
awareness to buffer allocation and, as we will see later, its
operation too.

To allocate a buffer, a net_pkt needs to have at least its network
interface set. This works if the family of the packet is unknown at
the time of buffer allocation. Then one could do:

.. code-block:: c

    net_pkt_alloc_buffer(pkt, size, proto, timeout);

Where proto could be 0 if unknown (there is no IPPROTO_UNSPEC).

As seen previously, the net_pkt and its buffer can be allocated at
once via :c:func:`net_pkt_alloc_with_buffer`. It is actually the most
widely used allocator.

The network interface, the family, and the protocol of the packet are
used by the buffer allocation to determine if the requested size can
be allocated.  Indeed, the allocator will use the network interface to
know the MTU and then the family and protocol for the headers space
(if only these 2 are specified).  If the whole fits within the MTU,
the allocated space will be of the requested size plus, eventually,
the headers space. If there is insufficient MTU space, the requested
size will be shrunk so the possible headers space and new size will
fit within the MTU.

For instance, on an Ethernet network interface, with an MTU of 1500
bytes:

.. code-block:: c

    pkt = net_pkt_alloc_with_buffer(iface, 800, AF_INET4, IPPROTO_UDP, K_FOREVER);

will successfully allocate 800 + 20 + 8 bytes of buffer for the new
net_pkt where:

.. code-block:: c

    pkt = net_pkt_alloc_with_buffer(iface, 1600, AF_INET4, IPPROTO_UDP, K_FOREVER);

will successfully allocate 1500 bytes, and where 20 + 8 bytes (IPv4 +
UDP headers) will not be used for the payload.

On the receiving side, when the family and protocol are not known:

.. code-block:: c

    pkt = net_pkt_rx_alloc_with_buffer(iface, 800, AF_UNSPEC, 0, K_FOREVER);

will allocate 800 bytes and no extra header space.
But a:

.. code-block:: c

    pkt = net_pkt_rx_alloc_with_buffer(iface, 1600, AF_UNSPEC, 0, K_FOREVER);

will allocate 1514 bytes, the MTU + Ethernet header space.

One can increase the amount of buffer space allocated by calling
:c:func:`net_pkt_alloc_buffer`, as it will take into account the
existing buffer. It will also account for the header space if
net_pkt's family is a valid one, as well as the proto parameter. In
that case, the newly allocated buffer space will be appended to the
existing one, and not inserted in the front. Note however such a use
case is rather limited.  Usually, one should know from the start how
much size should be requested.


Deallocation
============

Each net_pkt is reference counted. At allocation, the reference is set
to 1.  The reference count can be incremented with
:c:func:`net_pkt_ref()` or decremented with
:c:func:`net_pkt_unref()`. When the count drops to zero the buffer is
also un-referenced and net_pkt is automatically placed back into the
free net_pkt_slabs

If net_pkt's buffer is needed even after net_pkt deallocation, one
will need to reference once more all the chain of net_buf before
calling last net_pkt_unref. See :ref:`net_buf_interface` for more
information.


Operations
**********

There are two ways to access the net_pkt buffer, explained in the
following sections: basic read/write access and data access, the
latter being the preferred way.

Read and Write access
=====================

As said earlier, though net_pkt uses net_buf for its buffer, it
provides its own API to access it. Indeed, a network packet might be
scattered over a chain of net_buf objects, the functions provided by
net_buf are then limited for such case.  Instead, net_pkt provides
functions which hide all the complexity of potential non-contiguous
access.

Data movement into the buffer is made through a cursor maintained
within each net_pkt.  All read/write operations affect this
cursor. Note as well that read or write functions are strict on their
length parameters: if it cannot r/w the given length it will
fail. Length is not interpreted as an upper limit, it is instead the
exact amount of data that must be read or written.

As there are two paths, TX and RX, there are two access modes: write
and overwrite.  This might sound a bit unusual, but is in fact simple
and provides flexibility.

In write mode, whatever is written in the buffer affects the length of
actual data present in the buffer. Buffer length should not be
confused with the buffer size which is a limit any mode cannot pass.
In overwrite mode then, whatever is written must happen on valid data,
and will not affect the buffer length. By default, a newly allocated
net_pkt is on write mode, and its cursor points to the beginning of
its buffer.

Let's see now, step by step, the functions and how they behave
depending on the mode.

When freshly allocated with a buffer of 500 bytes, a net_pkt has 0
length, which means no valid data is in its buffer. One could verify
this by:

.. code-block:: c

    len = net_pkt_get_len(pkt);

Now, let's write 8 bytes:

.. code-block:: c

    net_pkt_write(pkt, data, 8);

The buffer length is now 8 bytes.
There are various helpers to write a byte, or big endian uint16_t, uint32_t.

.. code-block:: c

    net_pkt_write_u8(pkt, &foo);
    net_pkt_write_be16(pkt, &ba);
    net_pkt_write_be32(pkt, &bar);

Logically, net_pkt's length is now 15. But if we try to read at this
point, it will fail because there is nothing to read at the cursor
where we are at in the net_pkt. It is possible, while in write mode,
to read what has been already written by resetting the cursor of the
net_pkt. For instance:

.. code-block:: c

    net_pkt_cursor_init(pkt);
    net_pkt_read(pkt, data, 15);

This will reset the cursor of the pkt to the beginning of the buffer
and then let you read the actual 15 bytes present. The cursor is then
again pointing at the end of the buffer.

To set a large area with the same byte, a memset function is provided:

.. code-block:: c

    net_pkt_memset(pkt, 0, 5);

Our net_pkt has now a length of 20 bytes.

Switching between modes can be achieved via
:c:func:`net_pkt_set_overwrite` function. It is possible to switch
mode back and forth at any time.  The net_pkt will be set to overwrite
and its cursor reset:

.. code-block:: c

    net_pkt_set_overwrite(pkt, true);
    net_pkt_cursor_init(pkt);

Now the same operators can be used, but it will be limited to the
existing data in the buffer, i.e. 20 bytes.

If it is necessary to know how much space is available in the net_pkt
call:

.. code-block:: c

    net_pkt_available_buffer(pkt);

Or, if headers space needs to be accounted for, call:

.. code-block:: c

    net_pkt_available_payload_buffer(pkt, proto);

If you want to place the cursor at a known position use the function
:c:func:`net_pkt_skip`.  For example, to go after the IP header, use:

.. code-block:: c

    net_pkt_cursor_init(pkt);
    net_pkt_skip(pkt, net_pkt_ip_header_len(pkt));


Data access
===========

Though the API shown previously is rather simple, it involves always
copying things to and from the net_pkt buffer. In many occasions, it
is more relevant to access the information stored in the buffer
contiguously, especially with network packets which embed headers.

These headers are, most of the time, a known fixed set of bytes. It is
then more natural to have a structure representing a certain type of
header.  In addition to this, if it is known the header size appears
in a contiguous area of the buffer, it will be way more efficient to
cast the actual position in the buffer to the type of header. Either
for reading or writing the fields of such header, accessing it
directly will save memory.

Net pkt comes with a dedicated API for this, built on top of the
previously described API. It is able to handle both contiguous and
non-contiguous access transparently.

There are two macros used to define a data access descriptor:
:c:macro:`NET_PKT_DATA_ACCESS_DEFINE` when it is not possible to
tell if the data will be in a contiguous area, and
:c:macro:`NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE` when
it is guaranteed the data is in a contiguous area.

Let's take the example of IP and UDP. Both IPv4 and IPv6 headers are
always found at the beginning of the packet and are small enough to
fit in a net_buf of 128 bytes (for instance, though 64 bytes could be
chosen).

.. code-block:: c

    NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(ipv4_access, struct net_ipv4_hdr);
    struct net_ipv4_hdr *ipv4_hdr;

    ipv4_hdr = (struct net_ipv4_hdr *)net_pkt_get_data(pkt, &ipv4_acess);

It would be the same for struct net_ipv4_hdr. For a UDP header it
is likely not to be in a contiguous area in IPv6
for instance so:

.. code-block:: c

    NET_PKT_DATA_ACCESS_DEFINE(udp_access, struct net_udp_hdr);
    struct net_udp_hdr *udp_hdr;

    udp_hdr = (struct net_udp_hdr *)net_pkt_get_data(pkt, &udp_access);

At this point, the cursor of the net_pkt points at the beginning of
the requested data. On the RX path, these headers will be read but not
modified so to proceed further the cursor needs to advance past the
data. There is a function dedicated for this:

.. code-block:: c

    net_pkt_acknowledge_data(pkt, &ipv4_access);

On the TX path, however, the header fields have been modified. In such
a case:

.. code-block:: c

    net_pkt_set_data(pkt, &ipv4_access);

If the data are in a contiguous area, it will advance the cursor
relevantly. If not, it will write the data and the cursor will be
updated. Note that :c:func:`net_pkt_set_data` could be used in the RX
path as well, but it is slightly faster to use
:c:func:`net_pkt_acknowledge_data` as this one does not care about
contiguity at all, it just advances the cursor via
:c:func:`net_pkt_skip` directly.


API Reference
*************

.. doxygengroup:: net_pkt
   :project: Zephyr
