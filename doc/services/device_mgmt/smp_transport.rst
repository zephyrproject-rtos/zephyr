.. _mcumgr_smp_transport_specification:

SMP Transport Specification
###########################

The documents specifies information needed for implementing server and client
side SMP transports.

.. _mcumgr_smp_transport_ble:

BLE (Bluetooth Low Energy)
**************************

MCUmgr Clients need to use following BLE Characteristics, when implementing
SMP client:

- **Service UUID**: `8D53DC1D-1DB7-4CD3-868B-8A527460AA84`
- **Characteristic UUID**: `DA2E7828-FBCE-4E01-AE9E-261174997C48`

All SMP communication utilizes a single GATT characteristic.  An SMP request is
sent via a GATT Write Without Response command. An SMP response is sent in the form
of a GATT Notification

If an SMP request or response is too large to fit in a single GATT command, the
sender fragments it across several packets.  No additional framing is
introduced when a request or response is fragmented; the payload is simply
split among several packets. Since GATT guarantees ordered delivery of
packets, the SMP header in the first fragment contains sufficient information
for reassembly.

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

SMP protocol over serial is fragmented into MTU size frames; each
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

API Reference
*************

.. doxygengroup:: mcumgr_transport_smp
