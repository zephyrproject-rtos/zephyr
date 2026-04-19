.. _ipc_service_backend_serial:

Serial backend
##############

The serial backend enables inter-processor communication between two MCUs (or
CPUs) connected via a UART link. It uses
:abbr:`COBS (Consistent Overhead Byte Stuffing)` framing for reliable
byte-stream transport and supports multiple IPC service endpoints multiplexed
on a single serial connection.

Overview
========

The serial backend uses a UART device for exchanging data. Each frame is
COBS-encoded and delimited by ``0x00`` bytes, providing reliable framing over
an ordinary byte stream.

The backend supports the registration of multiple endpoints on a single
instance. Endpoints on both sides are matched by name. When both sides have
registered an endpoint with the same name, the endpoint is bound and data can
flow.

::

   +------------------+                      +------------------+
   |      MCU A       |                      |      MCU B       |
   |                  |                      |                  |
   |  +------------+  |                      |  +------------+  |
   |  | ipc-serial |--+--TX--------------RX->+--| ipc-serial |  |
   |  |   (UART)   |--+<-RX--------------TX--+--|   (UART)   |  |
   |  +--^------^--+  |                      |  +--^------^--+  |
   |     |      |     |                      |     |      |     |
   |    +-+    +-+    |                      |    +-+    +-+    |
   |    |a|    |b|    |                      |    |a|    |b|    |
   |    +-+    +-+    |                      |    +-+    +-+    |
   +------------------+                      +------------------+

Configuration
=============

The backend is configured via Kconfig and devicetree.

Devicetree
----------

The IPC serial instance is defined as a child node of a UART device node:

.. code-block:: devicetree

   &uart1 {
      ipc_serial: ipc-serial {
         compatible = "zephyr,ipc-serial";
         status = "okay";
      };
   };

You must provide a matching configuration on both sides of the communication,
each using its own UART device. The UART TX of one side must be connected to
the UART RX of the other side.

The ``timeout-ms`` property controls how long
:c:func:`ipc_service_register_endpoint` blocks waiting for the remote to
respond. Set it to ``0`` (the default) to return immediately, allowing the
binding to complete asynchronously.

.. code-block:: devicetree

   &uart1 {
      ipc_serial: ipc-serial {
         compatible = "zephyr,ipc-serial";
         timeout-ms = <1000>;
         status = "okay";
      };
   };

Kconfig
-------

The following Kconfig options are available:

* :kconfig:option:`CONFIG_IPC_SERVICE_BACKEND_SERIAL` - Enable the serial IPC
  backend. Enabled by default when the devicetree contains a
  ``zephyr,ipc-serial`` node and :kconfig:option:`CONFIG_UART_INTERRUPT_DRIVEN`
  is enabled.
* :kconfig:option:`CONFIG_IPC_SERVICE_BACKEND_SERIAL_NUM_EP` - Maximum number
  of endpoints per instance (default: 8).
* :kconfig:option:`CONFIG_IPC_SERVICE_BACKEND_SERIAL_RX_BUF_SIZE` - UART
  receive ring buffer size in bytes (default: 512). Must be large enough to
  hold at least one complete COBS-encoded frame.
* :kconfig:option:`CONFIG_IPC_SERVICE_BACKEND_SERIAL_FRAME_BUF_SIZE` - Maximum
  decoded frame size in bytes (default: 256). This limits the maximum payload
  per :c:func:`ipc_service_send` call, including internal protocol overhead.
* :kconfig:option:`CONFIG_IPC_SERVICE_BACKEND_SERIAL_CRC` - Enable CRC-16
  frame integrity checking (default: disabled). When enabled, a 2-byte
  CRC-16 checksum is appended to every transmitted frame and verified on
  reception; frames that fail the check are discarded. **Both ends of the
  link must have this option set identically.**

Binding
=======

When an endpoint is registered on one side, the following happens:

1. A discovery request containing the endpoint name and local ID is sent to the
   remote side over the UART link.
#. If the remote side has registered an endpoint with the same name, it records
   the mapping and replies with a discovery response.
#. Upon receiving the response, the originating side marks the endpoint as
   bound and calls the :c:member:`ipc_service_cb.bound` callback on both sides.

If ``timeout-ms`` is non-zero, :c:func:`ipc_service_register_endpoint` waits up
to the specified duration for the remote to respond. A timeout does not fail the
registration; the endpoint remains registered and will bind when the remote side
becomes available.

When the instance is opened, a reset command is sent to the remote to
invalidate any stale endpoint bindings from a previous session and to
exchange protocol capabilities (version and maximum frame size).  The
remote replies with a ``RESET_ACK`` carrying its own capabilities, so both
sides learn each other's parameters regardless of startup order.

Detailed Protocol Specification
===============================

The serial backend uses COBS-encoded frames separated by ``0x00`` delimiter
bytes. Each decoded frame starts with a 1-byte command ID followed by
command-specific data.

Frame Format
------------

.. list-table::
   :header-rows: 1

   * - Field name
     - Size (bytes)
     - Description
   * - ``cmd_id``
     - 1
     - Command identifier.
   * - ``data``
     - variable
     - Command-specific payload.
   * - ``crc`` *(optional)*
     - 2
     - CRC-16 checksum over all preceding bytes in the decoded frame,
       present only when :kconfig:option:`CONFIG_IPC_SERVICE_BACKEND_SERIAL_CRC`
       is enabled. Stored big-endian.

Commands
--------

.. list-table::
   :header-rows: 1

   * - Command
     - ID
     - Payload
     - Description
   * - ``RESET``
     - 1
     - ``[version:u8][max_rx_frame:u16_be]``
     - Sent when a side opens the instance. Carries the sender's protocol
       version and maximum decoded frame size (big-endian). The receiver
       stores these capabilities, replies with ``RESET_ACK``, then unbinds
       all endpoints and fires unbound callbacks so applications can detect
       a peer restart.
   * - ``RESET_ACK``
     - 2
     - ``[version:u8][max_rx_frame:u16_be]``
     - Sent in reply to ``RESET``. Carries the replying side's protocol
       version and maximum decoded frame size. Does not trigger endpoint
       unbinding.
   * - ``DISC_REQ``
     - 3
     - ``[local_id:u8][name...]``
     - Endpoint discovery request. Sent when registering an endpoint. The
       receiver looks up a local endpoint with a matching name, records the
       remote ID, and replies with ``DISC_RSP``.
   * - ``DISC_RSP``
     - 4
     - ``[local_id:u8][name...]``
     - Endpoint discovery response. The receiver records the remote ID and
       marks the endpoint as bound. No further reply is sent.
   * - ``DEREGISTER``
     - 5
     - ``[local_id:u8]``
     - Endpoint deregistration. Triggers the unbound callback on the remote.
   * - ``DATA``
     - 6
     - ``[ept_id:u8][payload...]``
     - Data frame addressed to a bound endpoint, identified by the sender's
       local endpoint ID.

Protocol Version and Capacity Exchange
---------------------------------------

When an instance is opened, both sides exchange their protocol version and
maximum decoded frame size via the ``RESET`` / ``RESET_ACK`` handshake:

.. code-block:: none

   Side A (opens/restarts)           Side B (already open or opens later)
   ================================   ================================
   send RESET(ver, max_rx_A) ----->
                                      store ver, max_rx_A
                                      unbind stale endpoints
                                      send RESET_ACK(ver, max_rx_B) -->
   store ver, max_rx_B
   (no unbinding)

This ensures both sides know each other's ``max_rx_frame`` regardless of
startup order.  If the received version does not match the local
``SERIAL_PROTOCOL_VERSION``, an error is logged.

Once the remote's ``max_rx_frame`` is known,
:c:func:`ipc_service_send` rejects payloads that would exceed the remote's
decode buffer and returns ``-EMSGSIZE``.
