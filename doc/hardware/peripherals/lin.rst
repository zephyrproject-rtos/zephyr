.. _lin:

Local Interconnect Network (LIN)
################################

Overview
********

LIN (Local Interconnect Network) is a serial bus system. Since 2016, it is standardized
internationally by ISO. In 2025, several parts of the ISO 17987 series have been updated.
The ISO 17987 document series covers the requirements of the seven OSI
(Open Systems Interconnection) layers and the corresponding conformance test plans.

LIN lower layers
================

The LIN data link layer, often called the LIN protocol, specifies a commander-responder protocol.
The LIN Commander uses one or more pre-programmed scheduling tables to start the sending and
receiving to the LIN frames. These scheduling tables contain at least the relative timing, when
the LIN frame sending is initiated. The LIN frame consists of two parts: header and response.
The LIN Commander sends the header, while either one dedicated LIN Responder or the LIN Commander
itself transmits the response.

The header comprises the 1-byte Break, the Inter-Byte Space, the 1-byte SYNC (5516),
the Identifier, and the Response Space. The responses consist of the payload-bytes and
the CRC byte.

Transmitted data within the LIN frame is transmitted serially as 8-bit data-bytes with
one additional start bit, one additional stop-bit, but no parity bit. Note that the break field in
the header has no start bit and no stop bit.

Bit rates may vary within the range of 1 kbit/s to 20 kbit/s. Bit values on the bus are recessive
(logical high) or dominant (logical low). The time normal is considered by the LIN Commanders
stable clock source, the smallest entity is one bit time (52 µs at a bitrate of 19,2 kbit/s).
Two bus states are defined: sleep mode and active mode. While data is on the bus, all LIN-nodes are
requested to be in active mode. After a specified timeout, the nodes enter the sleep mode and are
released back to active mode by a wake-up frame. This frame may be sent by any node requesting
activity on the bus, either the LIN Commander following its internal schedule, or one of the
attached LIN Responders being activated by its internal software application. After all nodes are
awakened, the LIN Commander continues to schedule the next LIN frame.

For more technical information on LIN you may visit
`LIN on Wikipedia <https://en.wikipedia.org/wiki/Local_Interconnect_Network>`_.

Zephyr LIN controller support following LIN features:

* Send and receive LIN frames in commander and responder mode.
* Filters with header masking (Responder mode), that allows a header ID to trigger responsder callback.
* Wake up pulse sending.

Samples
*******

We have a sample demonstrating the use of Zephyr LIN controller API:

* :zephyr:code-sample:`lin-ncv7430`: Demonstrates how to use the LIN API in commander mode.

API Reference
*************

.. doxygengroup:: lin_controller
