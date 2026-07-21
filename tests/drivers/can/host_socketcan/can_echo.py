#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0
"""Reflect every CAN frame received on a SocketCAN interface back onto the bus.

Used as the companion for the CAN host-SocketCAN test: the guest sends a frame
in normal mode and this echo sends it back so the guest can receive it. Only the
standard library is required. By default a raw CAN socket does not receive the
frames it sends, so this does not echo its own reflections.
"""

import socket
import sys


def main():
    iface = sys.argv[1]
    sock = socket.socket(socket.PF_CAN, socket.SOCK_RAW, socket.CAN_RAW)
    sock.bind((iface,))
    while True:
        frame = sock.recv(16)
        sock.send(frame)


if __name__ == "__main__":
    main()
