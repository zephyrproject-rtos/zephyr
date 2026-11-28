.. _mcumgr_smp_transport_specification:

SMP Transport Specification
###########################

The documents specifies information needed for implementing server and client
side SMP transports.

.. _mcumgr_smp_transport_ble:

Bluetooth Low Energy (LE)
*************************

MCUmgr Clients need to use following Bluetooth Characteristics, when implementing
SMP client:

- **Service UUID**: ``8D53DC1D-1DB7-4CD3-868B-8A527460AA84``
- **Characteristic UUID**: ``DA2E7828-FBCE-4E01-AE9E-261174997C48``

All SMP communication utilizes a single GATT characteristic.  An SMP request is
sent via a GATT Write Without Response command. An SMP response is sent in the form
of a GATT Notification

If an SMP request or response is too large to fit in a single GATT command, the
sender fragments it across several packets.  No additional framing is
introduced when a request or response is fragmented; the payload is simply
split among several packets. Since GATT guarantees ordered delivery of
packets, the SMP header in the first fragment contains sufficient information
for reassembly.

.. _mcumgr_smp_transport_ble_client:

Zephyr Bluetooth transports
===========================

Zephyr implements the two sides of the Bluetooth transport as two transports, selected by two
Kconfig options:

.. list-table::
   :header-rows: 1
   :widths: 30 20 50

   * - Kconfig option
     - Local role
     - Used for
   * - :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_BT`
     - GATT server
     - Serves the SMP service to a central, such as a phone running an MCUmgr client.
       Requires :kconfig:option:`CONFIG_BT_PERIPHERAL`.
   * - :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_BT_CLIENT`
     - GATT client
     - Sends SMP requests to the SMP service of a peer. Requires
       :kconfig:option:`CONFIG_BT_CENTRAL` and :kconfig:option:`CONFIG_BT_GATT_CLIENT`.
       The client is experimental, and enabling it also enables
       :kconfig:option:`CONFIG_EXPERIMENTAL`.

Each can be enabled on its own, or both in the same image. An image with only the client has no
SMP GATT service of its own. When both are enabled, they are separate SMP transport objects, so a
response from a peer is never mixed up with a request written by a central that is connected to
the server.

:kconfig:option:`CONFIG_MCUMGR_TRANSPORT_BT_CLIENT` requires either
:kconfig:option:`CONFIG_SMP_CLIENT` or :kconfig:option:`CONFIG_MCUMGR_GRP_TRANSPORT`, but not both,
for one of two uses:

* The MCUmgr SMP client (:kconfig:option:`CONFIG_SMP_CLIENT`) manages a peer that runs the SMP
  service. The transport registers as ``SMP_BLUETOOTH_TRANSPORT``, and the application passes
  that type to :c:func:`smp_client_object_init`.

* The outgoing side of a :ref:`transport management <mcumgr_smp_group_11>` bridge
  (:kconfig:option:`CONFIG_MCUMGR_GRP_TRANSPORT`) connects to a Bluetooth peer, so that requests
  received over another transport are forwarded to it. The incoming side of a bridge over
  Bluetooth is part of the server, so a bridge from a Bluetooth central to another transport
  needs :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_BT` as well as
  :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_BT_CLIENT`.

Attaching the SMP client
------------------------

The application scans for the peer, establishes the connection and attaches the transport to
it:

.. code-block:: c

   static struct smp_client_object smp_client;

   /* conn is a connection that the application has established to the peer */
   static int attach_peer(struct bt_conn *conn)
   {
       int rc;

       rc = smp_client_object_init(&smp_client, SMP_BLUETOOTH_TRANSPORT);
       if (rc == 0) {
           rc = smp_client_bt_attach(conn, K_SECONDS(5));
       }

       return rc;
   }

:c:func:`smp_client_bt_attach` discovers the SMP service on the peer and subscribes to its
notifications before it returns, so it must not be called from the Bluetooth RX thread, the
system work queue or the MCUmgr work queue. It fails with ``-ENOTSUP`` when the peer has no
usable SMP service, with ``-EIO`` when the peer rejects the subscription, for example because it
requires an encrypted link, and with ``-ETIMEDOUT`` when the peer does not answer within the
timeout. It fails with ``-EBUSY`` while the transport is not free, see
:ref:`mcumgr_smp_transport_ble_client_busy`.

The transport must be the only subscriber to the SMP characteristic of the peer. The Bluetooth
host makes no CCC write for a second subscription to a characteristic, so an attach to a peer
whose SMP characteristic the application has subscribed to itself fails with ``-ETIMEDOUT``.

:c:func:`smp_client_bt_detach` unsubscribes from the notifications, releases the target and
leaves the connection up. The target is also released when the peer disconnects. While it is the
target, the transport holds a reference of its own on the connection, so the application manages
its own reference as usual.

The transport writes each SMP packet in fragments of ATT_MTU - 3 bytes. Unless the peer starts the
ATT MTU exchange, enable :kconfig:option:`CONFIG_BT_GATT_AUTO_UPDATE_MTU` or call
:c:func:`bt_gatt_exchange_mtu`; otherwise every fragment carries 20 bytes. The image upload client
sizes its frames from :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_NETBUF_SIZE`, which must not exceed
the value the peer uses. The number of fragments in flight and how long a packet waits for a
transmit credit are set with :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_BT_CLIENT_TX_CREDITS` and
:kconfig:option:`CONFIG_MCUMGR_TRANSPORT_BT_CLIENT_TX_CREDIT_TIMEOUT`.

A sample that attaches the transport and uploads an image to a peer is
:zephyr:code-sample:`smp-bt-client`.

.. _mcumgr_smp_transport_ble_client_busy:

One target at a time
--------------------

The transport serves one peer at a time. While a target is attached, :c:func:`smp_client_bt_attach`
fails with ``-EBUSY``, and while a bridge holds the transport, another bridge connect fails with
``TRANSPORT_MGMT_ERR_CONNECT_FAILED``. This also applies until the link of a disconnected bridge has
ended, and after :c:func:`smp_client_bt_detach` until the host reports the response to the CCC write
of the unsubscribe, or the link ends. If the peer rejects that write and the link has no other
subscriptions, the host does not report it, and the transport stays busy until the link ends.

A bridge connect waits up to 30 seconds for the link to come up and for the SMP service to be
discovered and subscribed. It fails with ``TRANSPORT_MGMT_ERR_CONNECT_FAILED`` when that does not
happen, for example because the peer has no SMP service or rejects the subscription, and the
link that the bridge created is disconnected. When a bridge is disconnected, the transport
disconnects its link, which also ends the subscription.

.. _mcumgr_smp_transport_uart:

UART/serial and console
***********************

SMP protocol specification by MCUmgr subsystem of Zephyr uses basic framing
of data to allow multiplexing of UART channel. Multiplexing requires
prefixing each frame with two byte marker and terminating it with newline.
Currently MCUmgr imposes a 127 byte limit on frame size, although there
are no real protocol constraints that require that limit.
The limit includes the prefix and the newline character, so the allowed payload
size is actually 124 bytes.

Although no such transport exists in Zephyr, it is possible to implement
MCUmgr client/server over UART transport that does not have framing at all,
or uses hardware serial port control, or other means of framing.

Frame fragmenting
=================

SMP protocol over serial is fragmented into frames of up to 127 bytes; each
frame consists of two byte start marker, body and terminating newline
character.

There are four types of types of frames: initial, partial, partial-final
and initial-final; each frame type differs by start marker and/or body
contents.

Frame formats
-------------

Initial frame requires to be followed by optional sequence of partial
frames and finally by partial-final frame.
Body is always Base64 encoded, so the body size, here described as
MTU - 3, is able to actually carry N = (MTU - 3) / 4 * 3 bytes
of raw data.

Body of initial frame is preceded by two byte total packet length,
encoded in Big Endian, and equals size of a raw body plus two bytes,
size of CRC16; this means that actual body size allowed into an
initial frame is N - 2.

If a body size is smaller than N - 4, than it is possible to carry
entire body with preceding length and following it CRC in a single
frame, here called initial-final; for the description of initial-final
frame look below.

Initial frame format:

.. table::
    :align: center

    +---------------+---------------+---------------------------+
    | Content       | Size          | Description               |
    +===============+===============+===========================+
    | 0x06 0x09     | 2 bytes       | Frame start marker        |
    +---------------+---------------+---------------------------+
    | <base64-i>    | no more than  | Base64 encoded body       |
    |               | MTU - 3 bytes |                           |
    +---------------+---------------+---------------------------+
    | 0x0a          | 1 byte        | Frame termination         |
    +---------------+---------------+---------------------------+

``<base64-i>`` is Base64 encoded body of format:

.. table::
    :align: center

    +---------------+---------------+---------------------------+
    | Content       | Size          | Description               |
    +===============+===============+===========================+
    | total length  | 2 bytes       | Big endian 16-bit value   |
    |               |               | representing total length |
    |               |               | of body + 2 bytes for     |
    |               |               | CRC16; note that size of  |
    |               |               | total length field is not |
    |               |               | added to total length     |
    |               |               | value.                    |
    +---------------+---------------+---------------------------+
    | body          | no more than  | Raw body data fragment    |
    |               | MTU - 5       |                           |
    +---------------+---------------+---------------------------+

Initial-final frame format is similar to initial frame format,
but differs by ``<base64-i>`` definition.

``<base64-i>`` of initial-final frame, is Base64 encoded data taking
form:

.. table::
    :align: center

    +---------------+---------------+---------------------------+
    | Content       | Size          | Description               |
    +===============+===============+===========================+
    | total length  | 2 bytes       | Big endian 16-bit value   |
    |               |               | representing total length |
    |               |               | of body + 2 bytes for     |
    |               |               | CRC16; note that size of  |
    |               |               | total length field is not |
    |               |               | added to total length     |
    |               |               | value.                    |
    +---------------+---------------+---------------------------+
    | body          | no more than  | Raw body data fragment    |
    |               | MTU - 7       |                           |
    +---------------+---------------+---------------------------+
    | crc16         | 2 bytes       | CRC16 of entire packet    |
    |               |               | body, preceding length    |
    |               |               | not included.             |
    +---------------+---------------+---------------------------+

Partial frame is continuation after previous initial or other partial
frame. Partial frame takes form:

.. table::
    :align: center

    +---------------+---------------+---------------------------+
    | Content       | Size          | Description               |
    +===============+===============+===========================+
    | 0x04 0x14     | 2 bytes       | Frame start marker        |
    +---------------+---------------+---------------------------+
    | <base64-i>    | no more than  | Base64 encoded body       |
    |               | MTU - 3 bytes |                           |
    +---------------+---------------+---------------------------+
    | 0x0a          | 1 byte        | Frame termination         |
    +---------------+---------------+---------------------------+

The ``<base64-i>`` of partial frame is Base64 encoding of data,
taking form:

.. table::
    :align: center

    +---------------+---------------+---------------------------+
    | Content       | Size          | Description               |
    +===============+===============+===========================+
    | body          | no more than  | Raw body data fragment    |
    |               | MTU - 3       |                           |
    +---------------+---------------+---------------------------+

The ``<base64-i>`` of partial-final frame is Base64 encoding of data,
taking form:

.. table::
    :align: center

    +---------------+---------------+---------------------------+
    | Content       | Size          | Description               |
    +===============+===============+===========================+
    | body          | no more than  | Raw body data fragment    |
    |               | MTU - 5       |                           |
    +---------------+---------------+---------------------------+
    | crc16         | 2 bytes       | CRC16 of entire packet    |
    |               |               | body, preceding length    |
    |               |               | not included.             |
    +---------------+---------------+---------------------------+


CRC Details
-----------

The CRC16 included in final type frames is calculated over only
raw data and does not include packet length.
CRC16 polynomial is 0x1021 and initial value is 0.

.. _mcumgr_smp_transport_raw_uart:

Raw UART/serial (without console)
*********************************

In Zephyr, an alternative transport is available for UART which does not use Base64 as the SMP
over console transport does, this allows for smaller code size and faster transfers - but
cannot be used on devices which have shell or log output on the same UART or are connected to
terminals that are displaying ASCII data, as all communication is done using the raw binary
SMP protocol.

To use this protocol, use :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_RAW_UART`, it requires the
MCUmgr UART console driver to be enabled in raw mode.

Timeout
=======

Because there is no framing when using this transport, it is possible that there might be invalid
data received on the UART which causes the system to wait for e.g. a very long packet which is
not going to arrive. To prevent this, a timeout system exists whereby if the full packet is not
received within a specific timeframe, the whole receive buffer is cleared. It is recommended that
this option should be enabled on UART ports, for transports like USB CDC whereby it is a "virtual"
UART and the data can be verified before being passed, this option is not needed. The option can
be enabled with :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_RAW_UART_INPUT_TIMEOUT`, the timeout for
this can be set with :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_RAW_UART_INPUT_TIMEOUT_TIME_MS`.

API Reference
*************

.. doxygengroup:: mcumgr_transport_smp

.. doxygengroup:: mcumgr_transport_bt_client
