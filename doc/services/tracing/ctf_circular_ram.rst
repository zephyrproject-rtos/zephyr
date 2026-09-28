.. SPDX-FileCopyrightText: Copyright Qualcomm Technologies, Inc. and/or its subsidiaries.
.. SPDX-License-Identifier: Apache-2.0

.. _ctf_circular_ram_capture:

CTF Circular RAM Capture
########################

Overview
********

The CTF circular RAM capture provides a bounded, overwrite-oldest trace store for
devices where continuously streaming trace data over UART, USB, or another transport is
not practical. It operates as a flight recorder: once the allocated RAM is full, the
oldest complete CTF packet is replaced while newer packets remain available for
post-mortem analysis.

The capture path is local to the :ref:`Common Trace Format (CTF) <ctf>` implementation.
It is intentionally not registered as a generic tracing backend. CTF events are already
available as complete, self-contained event records at ``CTF_GATHER_FIELDS()``. Writing
them directly at that point preserves event boundaries and avoids exposing a
CTF-specific packet store to other tracing frontends.

After the RAM buffer is dumped from the target, the
:zephyr_file:`scripts/tracing/sort_ctf_circular.py` host utility validates and orders the
packets. The resulting trace directory can be passed directly to Babeltrace 2 or another
CTF 1.8 consumer.

Use cases
=========

The circular capture is useful when:

* the failure of interest occurs before a host can start a trace capture;
* the target cannot dedicate an I/O peripheral to tracing;
* trace collection must continue for an indefinite period using bounded memory;
* only the events immediately preceding a crash, watchdog reset, or debugger halt are
  required; or
* preserving complete CTF event and packet boundaries is more important than retaining
  the oldest events.

It is not intended to replace a streaming backend when the complete trace history must
be retained.

Design
******

Event routing
=============

With a normal tracing output selected, the CTF path remains unchanged::

   CTF_EVENT()
       -> CTF_GATHER_FIELDS()
       -> tracing_format_raw_data()
       -> tracing core
       -> selected tracing backend

With :kconfig:option:`CONFIG_TRACING_CTF_CIRCULAR_RAM` selected, the final routing step
changes::

   CTF_EVENT()
       -> CTF_GATHER_FIELDS()
       -> ctf_circular_ram_write()
       -> ram_tracing_circular[]

The two paths are selected at build time. Circular capture uses the RAM tracing backend
selection while CTF events are routed directly to the CTF-local writer.

The local writer still uses the tracing core's enable state. Calling the existing tracing
control integration to disable tracing therefore also stops writes to the circular
buffer. The circular output itself does not provide a transport for receiving host
commands.

Why this is not a tracing backend
=================================

A generic tracing backend receives bytes after the frontend and formatting decisions
have already been made. The circular implementation has additional CTF-specific
requirements:

* each write must contain exactly one complete CTF event;
* the event timestamp must be the first serialized event field;
* CTF packet headers and packet context fields must be generated and updated;
* rollover must replace complete CTF packets rather than arbitrary byte ranges; and
* the host must use metadata matching the generated packet context.

Registering this code as a generic backend would imply that other tracing formats and
frontend APIs could use it. Those callers do not necessarily provide CTF events or
preserve event boundaries. In particular, asynchronous tracing can deliver arbitrary
byte runs that may contain a partial event or multiple events. Keeping the writer inside
``subsys/tracing/ctf`` makes these constraints explicit.

Alternatives considered
=======================

Generic tracing backend
-----------------------

A registered backend reuses the existing tracing-core dispatch mechanism and requires
fewer changes in the CTF frontend. However, it incorrectly presents a CTF-only format as
a general-purpose backend and relies on every caller providing exactly one complete CTF
event. It is also incompatible with the asynchronous formatter's arbitrary byte runs.

Linear RAM backend
------------------

The existing RAM backend is simple and produces a byte stream that can be dumped with a
debugger. Once its buffer is full, subsequent events are discarded. This retains the
beginning of the trace instead of the events closest to a late failure.

Byte-oriented circular buffer
-----------------------------

A conventional byte ring buffer can use nearly all available memory, but rollover may
split a CTF event or packet header. The host would then need to search for a valid event
boundary without enough information to distinguish payload bytes from framing bytes.
Whole-packet rotation makes recovery deterministic.

Whole CTF packet rotation
-------------------------

The selected design divides RAM into fixed-size, independently decodable CTF packets.
Rollover may leave unused bytes at the end of a packet, but it never intentionally splits
an event. Packet sequence numbers provide an unambiguous host-side ordering after the
physical buffer wraps.

Configuration
*************

Enable the capture with a configuration similar to:

.. code-block:: cfg

   CONFIG_TRACING=y
   CONFIG_TRACING_CTF=y
   CONFIG_TRACING_SYNC=y
   CONFIG_TRACING_CTF_CIRCULAR_RAM=y
   CONFIG_TRACING_BACKEND_RAM=y
   CONFIG_TRACING_CTF_TIMESTAMP=y

   CONFIG_RAM_TRACING_BUFFER_SIZE=4096
   CONFIG_TRACING_BACKEND_RAM_PACKET_COUNT=32
   CONFIG_TRACING_PACKET_MAX_SIZE=64

The relevant options are:

* :kconfig:option:`CONFIG_TRACING_CTF_CIRCULAR_RAM` enables the CTF-local circular
  capture path.
* :kconfig:option:`CONFIG_TRACING_SYNC` is required because the writer consumes one
  complete event per call.
* :kconfig:option:`CONFIG_TRACING_BACKEND_RAM` is the required tracing backend
  selection. CTF events are routed directly to the circular capture writer.
* :kconfig:option:`CONFIG_TRACING_CTF_TIMESTAMP_64` is selected automatically. The
  circular metadata and packet writer use 64-bit timestamps.
* :kconfig:option:`CONFIG_RAM_TRACING_BUFFER_SIZE` sets the total size of
  ``ram_tracing_circular``.
* :kconfig:option:`CONFIG_TRACING_BACKEND_RAM_PACKET_COUNT` sets the number of rotating
  packet slots.
* :kconfig:option:`CONFIG_TRACING_PACKET_MAX_SIZE` contributes to the build-time check
  that one slot can contain a packet header and the configured maximum trace packet.

Use the RAM tracing backend with circular CTF capture.

Buffer sizing
=============

For a buffer of size :math:`B` and :math:`N` packet slots, each slot has size:

.. math::

   S = B / N

The current packet header and context occupy 36 bytes. Therefore, the nominal event-data
capacity, before end-of-packet fragmentation, is:

.. math::

   B - (36 * N)

The build fails unless:

* At least two packet slots are configured.
* ``CONFIG_RAM_TRACING_BUFFER_SIZE`` is divisible by
  ``CONFIG_TRACING_BACKEND_RAM_PACKET_COUNT``.
* A slot can contain the 36-byte header plus ``CONFIG_TRACING_PACKET_MAX_SIZE`` bytes.
* The slot size in bits can be represented by the 32-bit ``packet_size`` field.

With the defaults of 4096 bytes and 32 slots, each slot is 128 bytes. Packet headers use
1152 bytes in total, leaving a nominal 2944 bytes for events. A smaller slot count reduces
header overhead and provides more room for large events, while a larger slot count creates
more independently ordered packets at the cost of additional headers.

.. _ctf_circular_ram_maximum_event_size:

Maximum event size
==================

The nominal capacity above is the total event-data capacity across all slots. It is not
the maximum size of one event. A single event must fit in the event-data area of one slot:

.. math::

   E_{max} = S - 36

where :math:`S` is the slot size and 36 bytes is the packet header and context size. With
the default 128-byte slot, the largest event that can be recorded is 92 bytes.

When an event fits in an empty slot but not in the active slot, the writer closes the
active packet and writes the complete event to the next packet. The unused bytes at the
end of the closed packet are padding and are excluded by its ``content_size`` field.

An event larger than :math:`E_{max}` cannot fit in the current packet or in any subsequent
packet. The writer drops it without modifying the circular buffer. It does not split an
event across two packets because every packet must remain independently decodable after
rollover or a debugger halt. Splitting would require continuation framing and would leave
an incomplete event if either packet were overwritten or captured during an update.

Set :kconfig:option:`CONFIG_TRACING_PACKET_MAX_SIZE` no larger than :math:`E_{max}`. The
build-time assertion verifies this configuration. If a larger event must be retained,
increase :kconfig:option:`CONFIG_RAM_TRACING_BUFFER_SIZE`, reduce
:kconfig:option:`CONFIG_TRACING_BACKEND_RAM_PACKET_COUNT`, or both. For a requested
maximum event size :math:`P`, the configuration must satisfy:

.. math::

   \frac{B}{N} \mathrel{\geq} 36 + P

Memory format
*************

The exported target symbol remains::

   uint8_t ram_tracing_circular[CONFIG_RAM_TRACING_BUFFER_SIZE];

The buffer consists of equally sized slots. Each populated slot begins with the following
packed, little-endian header and packet context:

.. list-table:: Circular CTF packet layout
   :header-rows: 1
   :widths: 20 15 15 50

   * - Field
     - Offset
     - Size
     - Description
   * - ``magic``
     - 0
     - 4 bytes
     - CTF packet magic, ``0xC1FC1FC1``.
   * - ``timestamp_begin``
     - 4
     - 8 bytes
     - Timestamp of the first event written to the packet.
   * - ``timestamp_end``
     - 12
     - 8 bytes
     - Timestamp of the most recently completed event in the packet.
   * - ``content_size``
     - 20
     - 4 bytes
     - Valid packet content size in bits, including this header.
   * - ``packet_size``
     - 24
     - 4 bytes
     - Full slot size in bits.
   * - ``packet_seq_num``
     - 28
     - 8 bytes
     - Monotonically increasing logical packet sequence number.
   * - Event data
     - 36
     - Variable
     - Complete serialized CTF events up to ``content_size``.

The layout is described by
:zephyr_file:`subsys/tracing/ctf/tsdl/metadata_circular`. Use this metadata instead of the
standard CTF metadata when processing a circular dump.

Packet lifecycle
****************

Initialization
==============

At initialization, the implementation:

#. clears the complete RAM buffer;
#. formats each slot as an empty packet;
#. assigns sequence number zero to the unused slots; and
#. opens slot zero with the first live sequence number.

Empty packets contain only the 36-byte packet header. The host utility ignores them
because they contain no event records.

Writing an event
================

For each event, the writer:

#. verifies that tracing is enabled;
#. rejects an event that cannot fit in an otherwise empty slot;
#. reads the 64-bit timestamp from the beginning of the event;
#. checks whether the event fits in the active slot;
#. advances to and reinitializes the next slot when rollover is required;
#. copies the complete event into the active slot; and
#. updates ``content_size`` and ``timestamp_end``.

The packet context is updated after every completed event. Consequently, a debugger dump
taken after halting the target ends at the last complete event instead of exposing unused
slot bytes as valid trace content.

Rollover
========

When an event does not fit, the active packet is closed and the next physical slot is
opened. If the next slot contains the oldest packet, it is overwritten as a unit. The new
packet receives a sequence number greater than the previous packet, regardless of its
physical slot index.

After rollover, physical slot order and chronological order are different. This is why a
raw memory dump must be normalized before it is given to Babeltrace.

Dumping and decoding a trace
****************************

Halt before dumping
===================

Halt the target before reading the buffer. Although packet context is updated after each
event, a live debugger read can race with an event copy or context update and produce an
inconsistent snapshot.

For example, with GDB:

.. code-block:: console

   (gdb) p sizeof(ram_tracing_circular)
   (gdb) dump binary memory ram_tracing_circular.bin \
         &ram_tracing_circular[0] \
         &ram_tracing_circular[0] + sizeof(ram_tracing_circular)

An equivalent debugger or Trace32 memory-save operation can be used as long as it dumps
the complete symbol without adding a textual or debugger-specific header.

Sort the packets
================

Run the host utility with either the configured packet count or the calculated slot size:

.. code-block:: console

   python3 $ZEPHYR_BASE/scripts/tracing/sort_ctf_circular.py \
       ram_tracing_circular.bin ctf-trace --packet-count 32

Alternatively:

.. code-block:: console

   python3 $ZEPHYR_BASE/scripts/tracing/sort_ctf_circular.py \
       ram_tracing_circular.bin ctf-trace --slot-size 128

The default metadata path is resolved relative to the script file, not the current working
directory. The command can therefore be run from any directory as long as the script
remains in its normal Zephyr repository location. Use ``--metadata`` when processing the
dump with a copied script or custom metadata:

.. code-block:: console

   python3 sort_ctf_circular.py ram_tracing_circular.bin ctf-trace \
       --packet-count 32 --metadata /path/to/metadata_circular

The utility creates:

* ``ctf-trace/metadata``, copied from ``metadata_circular``; and
* ``ctf-trace/channel0``, containing valid packets in chronological sequence-number
  order.

Existing output files are not replaced unless ``--force`` is supplied.

The utility rejects or ignores slots with:

* an invalid CTF magic value;
* a packet size that does not match the configured slot size;
* an invalid content size;
* no completed event data;
* a begin timestamp later than the end timestamp; or
* a duplicate packet sequence number.

Decode the trace
================

Pass the generated directory to Babeltrace 2:

.. code-block:: console

   babeltrace2 ctf-trace

Because the host utility has already restored chronological packet order, Babeltrace sees
a conventional CTF stream and does not need to understand the target's physical circular
buffer layout.

Advantages
**********

Bounded memory use
   The target allocation is fixed at build time and does not grow with capture duration.

Recent-event retention
   The capture preserves events nearest to a late crash or debugger halt instead of
   stopping when the buffer first becomes full.

Complete-event rollover
   Events are never intentionally split across the end of a slot. Host recovery does not
   need to scan arbitrary payload bytes for a possible event boundary.

Self-describing packet state
   Every populated slot contains CTF magic, valid-content size, timestamps, and a sequence
   number. A halted target can be decoded through standard CTF tooling after normalization.

Frontend isolation
   CTF-specific packet construction is kept out of the generic tracing backend API. Other
   tracing formats retain their normal backend behavior.

No continuous transport requirement
   Trace collection does not depend on host availability, UART bandwidth, USB setup, or a
   filesystem on the target.

Stable debugger symbol
   The ``ram_tracing_circular`` symbol provides a simple and deterministic debugger dump
   target.

Trade-offs and limitations
**************************

Old events are lost
   Overwriting is the intended policy. Once all slots have been used, the oldest complete
   packet is no longer recoverable.

Packet headers consume RAM
   Every slot uses 36 bytes for framing. More slots increase metadata overhead. Unused
   bytes at the end of a closed packet add internal fragmentation.

Synchronous execution cost
   Event data and packet context are copied and updated in the tracing call path. CTF
   events are commonly emitted while interrupts are locked, so large events or a high
   event rate can increase interrupt latency.

CTF-only format
   The stored data is not a generic byte log. It depends on CTF event layout,
   ``metadata_circular``, and a timestamp at the start of every event.

No asynchronous mode
   Asynchronous tracing does not preserve one-event-per-write boundaries and is therefore
   unsupported.

Little-endian target assumption
   Packet header fields are written in the target's native representation and the metadata
   declares little-endian byte order. A big-endian target requires explicit byte-order
   conversion or different metadata.

Offline normalization step
   The raw dump is not chronological after wraparound. It must be processed by
   ``sort_ctf_circular.py`` before use with standard CTF tools.

Target must be halted for a reliable snapshot
   Updating an event and its packet context is not an atomic memory transaction. Reading
   the buffer while the target runs may capture partially updated state.

Limited SMP synchronization
   The existing CTF event path uses interrupt locking to prevent local interrupt reentry.
   It does not provide a global lock between CPUs. Concurrent writers on an SMP system
   require additional serialization or per-CPU packet buffers.

Oversized events are dropped
   See :ref:`ctf_circular_ram_maximum_event_size` for the maximum event size, why event
   records are not split between packets, and configuration options for larger events.

Timestamp interpretation
   Circular events use the 64-bit cycle counter to avoid the short rollover interval of a
   32-bit nanosecond timestamp. Consumers that need seconds or wall-clock time must know
   the target counter frequency or extend the metadata with an appropriate CTF clock
   description.

Trace confidentiality
   Kernel and application events can contain identifiers, addresses, names, and other
   sensitive state. Protect RAM dumps and generated trace directories according to the
   product's security requirements.

Comparison with the linear RAM backend
**************************************

.. list-table:: RAM tracing output comparison
   :header-rows: 1
   :widths: 25 35 40

   * - Property
     - Linear RAM backend
     - CTF circular RAM capture
   * - Full-buffer policy
     - Discard new trace data.
     - Overwrite the oldest complete packet.
   * - Retained history
     - Beginning of the capture.
     - Most recent capture window.
   * - Format support
     - Generic tracing byte output.
     - CTF events only.
   * - Packet framing overhead
     - None beyond the emitted trace data.
     - 36 bytes per packet slot plus end-of-packet fragmentation.
   * - Host preparation
     - Place the dump beside matching metadata.
     - Validate and sort packets, then use the generated trace directory.
   * - Best suited for
     - Short, deliberately started captures.
     - Long-running flight recording and post-mortem debugging.

Implementation files
********************

The implementation is divided as follows:

* :zephyr_file:`subsys/tracing/ctf/ctf_top.h` selects the local writer or normal tracing
  formatter from ``CTF_GATHER_FIELDS()``.
* :zephyr_file:`subsys/tracing/ctf/ctf_circular_ram.c` implements packet rotation and
  exports the RAM symbol.
* :zephyr_file:`subsys/tracing/ctf/ctf_circular_ram.h` declares the CTF-local writer.
* :zephyr_file:`subsys/tracing/ctf/tsdl/metadata_circular` describes the generated CTF
  packet and event layout.
* :zephyr_file:`scripts/tracing/sort_ctf_circular.py` validates and orders the dumped
  packets.

Verification recommendations
****************************

When changing the implementation or metadata:

#. Build a CTF synchronous configuration with circular capture enabled.
#. Confirm that ``ram_tracing_circular`` has the configured size in the ELF file.
#. Generate enough events to wrap through every packet slot at least once.
#. Halt the target and dump the complete symbol.
#. Run ``sort_ctf_circular.py`` and verify that reported sequence numbers are increasing.
#. Decode the generated directory with Babeltrace 2.
#. Confirm that event timestamps and event types are ordered as expected across the
   physical slot-zero boundary.
#. Repeat with a debugger halt while the current packet is partially filled.
#. Verify that a normal CTF backend still uses ``tracing_format_raw_data()`` when circular
   capture is disabled.
