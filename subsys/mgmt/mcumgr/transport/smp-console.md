# SMP over console

This document specifies how the mcumgr Simple Management Protocol (SMP) is
transmitted over text consoles.

## Overview

Mcumgr packets sent over serial are fragmented into frames of 127 bytes or
fewer.  This 127-byte maximum applies to the entire frame, including header,
CRC, and terminating newline.

The initial frame in a packet has the following format:

```
    offset 0:    0x06 0x09
    === Begin base64 encoding ===
    offset 2:    <16-bit packet-length>
    offset ?:    <body>
    offset ?:    <crc16> (if final frame)
    === End base64 encoding ===
    offset ?:    0x0a (newline)
```

All subsequent frames have the following format:

```
    offset 0:    0x04 0x14
    === Begin base64 encoding ===
    offset 2:    <body>
    offset ?:    <crc16> (if final frame)
    === End base64 encoding ===
    offset ?:    0x0a (newline)
```

All integers are represented in big-endian.  The packet fields are described
below:

| Field | Description |
| ----- | ----------- |
| 0x06 0x09 | Byte pair indicating the start of a packet. |
| 0x04 0x14 | Byte pair indicating the start of a continuation frame. |
| Packet length | The combined total length of the *unencoded* body plus the final CRC (2 bytes). Length is in Big-Endian format. |
| Body | The actual SMP data (i.e., 8-byte header and CBOR key-value map). |
| CRC16 | A CRC16 of the *unencoded* body of the entire packet.  This field is only present in the final frame of a packet. |
| Newline | A 0x0a byte; terminates a frame. |

The packet is fully received when <packet-length> bytes of body has been
received.

## CRC details

The CRC16 should be calculated with the following parameters:

| Field         | Value         |
| ------------- | ------------- |
| Polynomial    | 0x1021        |
| Initial Value | 0             |
