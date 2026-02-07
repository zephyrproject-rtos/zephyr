# Copyright (c) 2025 Paul Wedeck <paulwedeck@gmail.com>
# SPDX-License-Identifier: Apache-2.0

Simple tool to receive logs from CAN backend
********************************************


Quickstart
**********
- Enable CAN backend (see samples/logging/can).
- Select can id (7ff in this document).
- Configure can device
- Configure python-can (e.g. export CAN_INTERFACE=socketcan)
- Run `recv_can_log.py 7ff` to receive logs over can.

How to use
**********
This tool listens on various CAN IDs to receive logs sent from Zephyr devices.
Each sender on the bus must use a unique CAN ID to avoid mangling of log messages.
CAN IDs must be in can-utils format: 3 hex digits for standard can ids and
8 hex digits for extended can ids.

Example: `recv_can_log.py 7ff 00000001`

Frame format
************
The data in the CAN frames is encoded as UTF-8.
Successive frames are concatenated to form complete messages.
Example (candump -a):
  can0       7FF   [8]  20 73 61 6D 70 6C 65 5F   ' sample_'
  can0       7FF   [8]  6D 61 69 6E 3A 20 65 78   'main: ex'
  can0       7FF   [8]  61 6D 70 6C 65 20 6D 65   'ample me'
  can0       7FF   [8]  73 73 61 67 65 20 32 0D   'ssage 2.'

ASCII LF (newline; '\n') marks the end of a log message.
Other ASCII control characters have no special meaning.
Any possible handling (passthrough, masking, escaping, etc.) is valid.

If the CAN bus supports it, CAN-FD frames may be used.
If the current send buffer is smaller than a full CAN-FD frame
but does not exactly match a possible frame length, the next lower
frame length must be selected. The remaining data must be sent using
a second frame.
Specifically, sending any bytes that are not part of the log message
(e.g. padding at the end), is invalid and might mangle legimite log
messages.

Known limitations
*****************
Received data is only printed once a newline character has been received.
This will hide the last message if it was only partially received.

This tool can not differentiate between log messages and binary data
accidentially sent on a logging CAN ID. This is by design to keep the data
format as generic and lightweight as possible.
