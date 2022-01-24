# SMP over Bluetooth

This document specifies how the mcumgr Simple Management Procotol (SMP) is
transmitted over Bluetooth.

## Overview

All SMP communication utilizes a single GATT characteristic.  An SMP request is
sent in the form of either 1) a GATT Write Command, or 2) a GATT Write Without
Response command.  An SMP response is sent in the form of a GATT Notification
specifying the same characteristic that was written.

If an SMP request or response is too large to fit in a single GATT command, the
sender fragments it across several commands.  No additional framing is
introduced when a request or response is fragmented; the payload is simply
split among several commands.  Since Bluetooth guarantees ordered delivery of
packets, the SMP header in the first fragment contains sufficient information
for reassembly.

## Services

### SMP service

UUID: `8D53DC1D-1DB7-4CD3-868B-8A527460AA84`

### Characteristics

#### SMP Characteristic

| Field | Value                                                             |
| ----- | ----------------------------------------------------------------- |
| Name                  | SMP                                               |
| Description           | Used for both SMP requests and responses.         |
| Read                  | Excluded                                          |
| Write                 | Mandatory                                         |
| WriteWithoutResponse  | Mandatory                                         |
| SignedWrite           | Excluded                                          |
| Notify                | Mandatory                                         |
| Indicate              | Excluded                                          |
| WritableAuxiliaries   | Excluded                                          |
| Broadcast             | Excluded                                          |
| ExtendedProperties    |                                                   |

As indicated, SMP requests can be sent in the form of either a Write or a Write
Without Response.  The Write Without Response form is generally preferred, as
an application-layer response is always sent in the form of a Notification.
The regular Write form is accepted in case the client requires a GATT response
to initiate pairing.

Security for this characteristic is optional.
