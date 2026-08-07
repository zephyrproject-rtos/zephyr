.. _spsc_pbuf:

Single Producer Single Consumer Packet Buffer
=============================================

A :dfn:`Single Producer Single Consumer Packet Buffer (SPSC_PBUF)` is a circular
buffer, whose contents are stored in first-in-first-out order. Variable size
packets are stored in the buffer. Packet buffer works under assumption that there
is a single context that produces packets and a single context that consumes the
data.

Implementation is focused on performance and memory footprint.

Packets are added to the buffer using :c:func:`spsc_pbuf_write` which copies a
data into the buffer. If the buffer is full error is returned.

Packets are copied out of the buffer using :c:func:`spsc_pbuf_read`.
