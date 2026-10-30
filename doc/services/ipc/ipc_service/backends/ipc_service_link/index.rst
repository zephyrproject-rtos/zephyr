.. _ipc_service_backend_link:

IPC link backend
################

The IPC link backend runs the IPC service between two processors that do not share memory, over
any transport that moves frames between them. It supports:

* Multiple endpoints, bound by name.
* Holding RX buffers.
* Sending from an interrupt context.
* Detection of a peer reboot, and of a peer that stops answering.
* Recovery from corrupted and lost frames.
* A :ref:`UART transport <ipc_service_backend_link_uart>` and an
  :ref:`SPI transport <ipc_service_backend_link_spi>`, with a small interface for further
  transports.

Overview
========

An instance is a devicetree child node of the bus controller that connects to the peer, which
runs the same backend over the same kind of transport. The backend is split in three:

.. mermaid::
   :caption: Two processors connected by the IPC link backend
   :alt: Two processors with mirrored stacks, read from left to right: in processor A, endpoints
         x and y, drawn with dashed borders, then one block headed IPC service API in bold with
         IPC link backend below it, then the bus controller, drawn as hardware; a line labelled
         UART, SPI and so on to the bus controller of processor B, then its IPC link backend
         block, then its endpoints x and y.

   ---
   config:
     flowchart:
       useMaxWidth: false
   ---
   flowchart LR
     subgraph a["Processor A"]
       ax(Endpoint x)
       ay(Endpoint y)
       alink["`**IPC service API**
       IPC link backend`"]
       abus[[Bus controller]]
       ax --- alink
       ay --- alink
       alink --- abus
     end
     subgraph b["Processor B"]
       bbus[[Bus controller]]
       blink["`**IPC service API**
       IPC link backend`"]
       bx(Endpoint x)
       by(Endpoint y)
       bbus --- blink
       blink --- bx
       blink --- by
     end
     abus -- "UART, SPI, ..." --- bbus

     classDef endpoint stroke-dasharray: 5 3
     classDef backend stroke-width: 3px
     class ax,ay,bx,by endpoint
     class alink,blink backend

* The **protocol core** builds the frames to send and processes the frames received, without
  any I/O: a frame carries a sequence number, a cumulative acknowledgment and the receiver's free
  buffers as credits, so payloads lost on the way are sent again and a sender never sends into a
  full receiver. A session starts with a boot announcement, so a rebooted peer is detected without
  timeouts.
* The **backend** implements the IPC service operations once for every transport: endpoints bind
  by name over control messages, messages are delivered from a work queue, and one timer per
  instance takes care of the retransmissions, the polls for credits and the boot announcements.
* A **transport** moves one frame at a time: as a stream where frames go each way on their own,
  or as transactions where one frame goes each way at a time.

Configuration
=============

The backend is configured using Kconfig and devicetree. The transports add their own options and
properties, described on their pages.

Kconfig
-------

:kconfig:option:`CONFIG_IPC_SERVICE_BACKEND_LINK` is selected by the transports. Its options
apply to all instances:

:kconfig:option:`CONFIG_IPC_SERVICE_BACKEND_LINK_NUM_EP` - endpoints per instance.

:kconfig:option:`CONFIG_IPC_SERVICE_BACKEND_LINK_EPT_NAME_MAX` - longest endpoint name.

:kconfig:option:`CONFIG_IPC_SERVICE_BACKEND_LINK_RX_BUFFERS` - receive frame buffers per
instance. One is always armed for the next frame; the others hold received frames until their
messages are delivered and released, and are the credits given to the peer. One less than this
is also the number of payloads sent before the first is acknowledged, as a peer configured alike
has no credits for more.

:kconfig:option:`CONFIG_IPC_SERVICE_BACKEND_LINK_MAX_RETRANSMITS` - timeouts in a row without an
answer from the peer after which the session ends.

:kconfig:option:`CONFIG_IPC_SERVICE_BACKEND_LINK_WQ_ENABLE`,
:kconfig:option:`CONFIG_IPC_SERVICE_BACKEND_LINK_WQ_STACK_SIZE` and
:kconfig:option:`CONFIG_IPC_SERVICE_BACKEND_LINK_WQ_PRIORITY` - the work queue calling the
endpoint callbacks, shared by all instances. Without it, the callbacks run on the system work
queue.

:kconfig:option:`CONFIG_IPC_SERVICE_LINK_STATS` - count the frames, payloads, retransmissions,
errors and session changes of each link, read with :c:func:`ipc_link_stats_get` and cleared
with :c:func:`ipc_link_stats_reset`.

:kconfig:option:`CONFIG_IPC_SERVICE_BACKEND_LINK_SHELL` - the ``ipc_link stats`` and
``ipc_link reset`` shell commands, which show and clear these counters for one or all instances.

Devicetree
----------

Every transport node has the ``mtu`` property: the frame size in bytes, a multiple of 4, the same
on both sides. A frame holds a 12 byte header, the messages, each with a 4 byte header, and a
2 byte CRC, so the largest message, as returned by :c:func:`ipc_service_get_tx_buffer_size`, is
20 bytes less than the frame size.

Every transport node also has the ``retransmit-timeout-ms`` property: the time without
acknowledgment after which a payload is sent again, also the interval of the boot announcements
and of the polls for credits. It is longer than a frame takes on the wire both ways, with the
peer's processing in between.

Behavior
========

* :c:func:`ipc_service_register_endpoint` returns at once. The :c:member:`ipc_service_cb.bound`
  callback follows once the peer registers an endpoint of the same name, in this session or a
  later one.
* :c:func:`ipc_service_send` copies the message into a frame and returns, from any context. It
  returns ``-ENOMEM`` when the transmit buffers are full: room comes with the peer's
  acknowledgment, which the :c:member:`ipc_service_cb.received` callback of an echo endpoint
  follows closely enough to send from. It returns ``-EBUSY`` for an endpoint not bound.
* The callbacks run on the work queue, one instance's deliveries at a time: a callback that
  blocks stalls the deliveries of every instance sharing the queue. The data passed to the
  :c:member:`ipc_service_cb.received` callback is valid until the callback returns, or until
  the buffer is released when it was held with :c:func:`ipc_service_hold_rx_buffer`.
* The :c:member:`ipc_service_cb.unbound` callback is called when the peer deregisters the
  endpoint, and for every bound endpoint when the session ends: the peer rebooted, stopped
  answering or came back incompatible. It is the signal that the peer is gone.

Limitations
===========

* No-copy sending is not supported: :c:func:`ipc_service_send_nocopy`,
  :c:func:`ipc_service_get_tx_buffer` and :c:func:`ipc_service_drop_tx_buffer` return
  ``-EIO``.
* A message must fit in one frame: fragmentation is not supported.
* The link is not secured: no authentication, no encryption.
* The :c:member:`ipc_service_cb.error` callback is not called.

Detailed Protocol Specification
===============================

All multi-byte fields are little-endian. Both CRCs are CRC-16/MCRF4XX: polynomial 0x1021
reflected (0x8408), initial value 0xFFFF, no final XOR; the check value over ``123456789`` is
0x6F91.

Frame
-----

A frame is a header, covered by its own CRC, and an optional payload, covered by a second CRC. A
corrupted payload still delivers the acknowledgment and the credits in the header. A frame
shorter than the header, longer than the frame size, or whose header CRC does not match has no
valid header.

.. mermaid::
   :caption: Frame header
   :alt: The 12 byte frame header: version, flags, sequence number, acknowledgment, credits, a
         reserved byte, the 16 bit payload length, the 16 bit frame size and the 16 bit header
         CRC.

   packet
     0-7: "Version"
     8-15: "Flags"
     16-23: "Sequence"
     24-31: "Ack"
     32-39: "Credits"
     40-47: "Reserved"
     48-63: "Payload length"
     64-79: "Frame size"
     80-95: "Header CRC-16"

.. list-table::
   :header-rows: 1

   * - Field
     - Description
   * - Version
     - Protocol version, 1.
   * - Flags
     - ``BOOT`` (bit 0): the sender started a session and waits for the acknowledgment.
       ``BOOT_ACK`` (bit 1): acknowledges the peer's ``BOOT``, once per announcement received.
       ``HDR_ERR`` (bit 2): the last frame received had no valid header, send the acknowledgment
       and credits again; sent once per such frame. ``POLL`` (bit 3): the sender waits for
       credits and asks for a header back, as the one carrying them may have been lost. The other
       bits are sent as 0 and ignored.
   * - Sequence
     - Sequence number of the payload, when the payload length is not 0, modulo 256.
   * - Ack
     - The next sequence number expected from the peer: all payloads before it are acknowledged.
   * - Credits
     - Free receive buffers of the sender. The peer keeps the number of its payloads not covered
       by this header's ``Ack`` at or below this value.
   * - Payload length
     - Length of the payload, a multiple of 4, or 0 for a frame carrying only an acknowledgment,
       credits, a boot announcement or a poll.
   * - Frame size
     - The sender's frame size, ``mtu``.
   * - Reserved
     - Sent as 0, ignored on receipt.
   * - Header CRC-16
     - Over the first 10 bytes.

The payload follows the header: messages back to back, each starting at a multiple of 4 bytes
and padded to one, and after the last padding a CRC-16 over the whole payload. The messages fill
the payload exactly: a payload that ends elsewhere is acknowledged and dropped, as sending it
again would not fix it.

.. mermaid::
   :caption: Message
   :alt: A message in the payload: a one byte endpoint address, a reserved flags byte, the 16
         bit data length, and the data, padded to a multiple of 4 bytes.

   packet
     0-7: "Address"
     8-15: "Flags"
     16-31: "Data length"
     32-63: "Data, padded to a multiple of 4 bytes"

The address is the receiver's endpoint address, 1 to 254, or 0 for the control channel. The flags
are sent as 0 and ignored on receipt. A control message is an operation byte and the sender's
endpoint address, followed for ``BIND`` (1) by the endpoint name without terminator and for
``BIND_ACK`` (2) by the address of the endpoint it answers; ``UNBIND`` (3) and ``UNBIND_ACK``
(4) carry the address alone. Control messages are messages like any other: they travel in
numbered payloads, acknowledged and sent again like the rest, so none is lost while the session
lasts.

Session
-------

A side that starts announces its boot with the ``BOOT`` flag, in every frame until the peer
acknowledges it with ``BOOT_ACK``, repeating the announcement every ``retransmit-timeout-ms``
while nothing answers. A peer receiving ``BOOT`` resets its sequence numbers, drops what it had
queued, unbinds every endpoint, and acknowledges. Until its own boot is acknowledged, a side
ignores the rest of what the peer sends: it belongs to an earlier session. Both sides may boot at
once: each then sees the other's ``BOOT``, and the session starts for each side once it has the
other's ``BOOT_ACK``.

.. mermaid::
   :caption: Session start and peer reboot
   :alt: Sequence diagram: A announces its boot, B resets and acknowledges, and both bind their
         endpoints. Later B reboots and announces it; A unbinds its endpoints, resets and
         acknowledges, and the endpoints bind again.

   ---
   config:
     sequence:
       useMaxWidth: false
   ---
   sequenceDiagram
     participant A
     participant B
     A->>B: BOOT
     Note over B: reset session
     B->>A: BOOT, BOOT_ACK
     A->>B: BOOT_ACK
     Note over A,B: session 1, endpoints bind
     Note over B: reboot
     B->>A: BOOT
     Note over A: unbound(), reset session
     A->>B: BOOT_ACK
     Note over A,B: session 2, endpoints bind again

A valid header with another protocol version or frame size is reported once and never starts a
session. From a peer the session was established with, it ends the session as a reboot would,
and the boot announcement starts again.

Endpoints
---------

Endpoint addresses are local: each side numbers its registered endpoints from 1. Binding tells
the peer which address belongs to which name.

.. mermaid::
   :caption: Endpoint binding
   :alt: Sequence diagram: A registers endpoint "echo" at address 1 and sends BIND; B has no such
         endpoint yet. B registers "echo" at address 3 and sends BIND; A matches the name, stores
         address 3, calls bound() and replies BIND_ACK naming address 3; B stores address 1 and
         calls bound().
         Later B deregisters it and sends UNBIND; A calls unbound() and replies UNBIND_ACK,
         after which B may give address 3 to another endpoint.

   ---
   config:
     sequence:
       useMaxWidth: false
   ---
   sequenceDiagram
     participant A
     participant B
     Note over A: register "echo" as 1
     A->>B: BIND 1 "echo"
     Note over B: no match, ignored
     Note over B: register "echo" as 3
     B->>A: BIND 3 "echo"
     Note over A: 1 is bound to 3, bound()
     A->>B: BIND_ACK 1 to 3
     Note over B: 3 is bound to 1, bound()
     A->>B: data to 3
     B->>A: data to 1
     Note over B: deregister 3
     B->>A: UNBIND 3
     Note over A: unbound()
     A->>B: UNBIND_ACK 3
     Note over B: 3 is free again

After a session starts, every registered endpoint is announced again with ``BIND``, and the first
side to see a matching name replies. A ``BIND_ACK`` names the endpoint it answers: one for an
address that is not registered, answering a bind withdrawn since, is ignored rather than taken
by an endpoint registered later under the same name. A ``BIND_ACK`` always precedes data from
the replying side, and data is only delivered to an endpoint bound in the session the frame was
sent in, so a reused endpoint address never receives what the peer sent to its previous
occupant. ``UNBIND`` is also
sent for an endpoint that was announced but never bound. The peer answers ``UNBIND_ACK`` once it
has unbound the endpoint and sends nothing more to the address, which is only then given to
another endpoint: until the acknowledgment, what still arrives for the address is dropped.

Acknowledgments, retransmissions and credits
--------------------------------------------

Payloads are numbered and acknowledged cumulatively. A sender keeps at most a window of payloads
in flight, and never more than the peer's last credits allow. The receiver takes payloads in
order: a payload out of order, or arriving when the receiver has no free buffer, is dropped and
the current ``Ack`` is returned, so the sender sends it again.

A payload not acknowledged within ``retransmit-timeout-ms`` is sent again with the ones after
it. A sender waiting for credits polls the peer for a header at the same interval, with
``POLL``, as the header that returned the credits may have been lost. Every such timeout without
a valid header from the peer counts, also one in which the transport could not send, and any
valid header starts the count over: after
:kconfig:option:`CONFIG_IPC_SERVICE_BACKEND_LINK_MAX_RETRANSMITS` timeouts in a row the session
ends, every endpoint is unbound and the link announces its boot again. A peer that holds every
buffer but keeps answering the polls holds the session open for as long as it likes. A peer that
is gone while nothing waits for it is not noticed until something does.

.. mermaid::
   :caption: A lost payload
   :alt: Sequence diagram: A sends payload 0, which B acknowledges; A sends payload 1, which is
         lost; after the retransmission timeout A sends payload 1 again, and B acknowledges 2.

   ---
   config:
     sequence:
       useMaxWidth: false
   ---
   sequenceDiagram
     participant A
     participant B
     A->>B: seq 0
     B->>A: ack 1
     A--xB: seq 1
     Note over A: retransmit-timeout-ms
     A->>B: seq 1
     B->>A: ack 2

The receiver's credits are its free receive buffers. A buffer is freed once every message in it
is delivered and released, so a buffer held with :c:func:`ipc_service_hold_rx_buffer` holds a
credit. When the credits advertised last were 0, a freed buffer is advertised right away.

Transports
==========

.. toctree::
   :maxdepth: 1

   spi.rst
   uart.rst

Transport Interface
===================

A transport is part of the backend, in :zephyr_file:`subsys/ipc/ipc_service/backends`: a device
whose API is :c:var:`ipc_link_backend_ops`, with a config starting with a
:c:struct:`ipc_link_config`, whose state and buffers the backend's private header defines with
``IPC_LINK_DEFINE()`` and fills in with ``IPC_LINK_CONFIG()``. The transport implements
:c:struct:`ipc_link_transport_api`, two operations: open, and kick, which tells it a frame is
pending. It drives the link with :c:func:`ipc_link_pending`, :c:func:`ipc_link_tx_prepare`,
:c:func:`ipc_link_rx_buf` and :c:func:`ipc_link_rx`, which may be called from any context; the
CRCs of a received frame are computed in the caller's context, before the link is locked.

.. mermaid::
   :caption: A transport exchanging frames
   :alt: Sequence diagram between the link and a transport: the link kicks the transport, which
         asks whether a frame is pending, gets the next frame from ipc_link_tx_prepare, sends
         it, receives a frame into the buffer from ipc_link_rx_buf and hands it to ipc_link_rx.

   ---
   config:
     sequence:
       useMaxWidth: false
   ---
   sequenceDiagram
     participant L as IPC link
     participant T as Transport
     participant B as Bus
     L->>T: kick()
     T->>L: ipc_link_pending()
     L-->>T: true
     T->>L: ipc_link_tx_prepare()
     L-->>T: frame
     T->>B: send frame
     B-->>T: frame received
     T->>L: ipc_link_rx_buf()
     L-->>T: buffer
     T->>L: ipc_link_rx(len, ok)
     L-->>T: events

.. doxygengroup:: ipc_link_api
