#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
#
# SPDX-License-Identifier: Apache-2.0

"""Act as the CANopen slave booted by the Zephyr CANopen sample in master mode."""

import argparse
import sys
import time

import canopen
from canopen.objectdictionary import ODVariable, datatypes

DEVICE_NAME = "python-canopen slave"
EXPECTED_HEARTBEAT_MS = 1000
EXPECTED_VALUE = 0x11223344


def add_variable(od, index, data_type, access, default):
    var = ODVariable(f"object_{index:04x}", index, 0)
    var.data_type = data_type
    var.access_type = access
    var.default = default
    od.add_object(var)


def create_node(node_id):
    od = canopen.ObjectDictionary()
    add_variable(od, 0x1000, datatypes.UNSIGNED32, "ro", 0)
    add_variable(od, 0x1008, datatypes.VISIBLE_STRING, "const", DEVICE_NAME)
    add_variable(od, 0x1017, datatypes.UNSIGNED16, "rw", 0)
    add_variable(od, 0x2000, datatypes.UNSIGNED32, "rw", 0)

    return canopen.LocalNode(node_id, od)


def wait_for_start(node, timeout):
    end = time.monotonic() + timeout
    while time.monotonic() < end:
        if node.nmt.state == "OPERATIONAL":
            return True
        time.sleep(0.05)
    return False


def check(node, timeout):
    failed = 0
    started = wait_for_start(node, timeout)

    for name, ok, detail in (
        ("NMT start from the master", started, node.nmt.state),
        (
            "1017h written by the master",
            int.from_bytes(node.get_data(0x1017, 0), "little") == EXPECTED_HEARTBEAT_MS,
            f"{int.from_bytes(node.get_data(0x1017, 0), 'little')}",
        ),
        (
            "2000h written by the master",
            int.from_bytes(node.get_data(0x2000, 0), "little") == EXPECTED_VALUE,
            f"0x{int.from_bytes(node.get_data(0x2000, 0), 'little'):08X}",
        ),
    ):
        print(f"{'PASS' if ok else 'FAIL'}: {name} ({detail})")
        if not ok:
            failed += 1

    return failed


def main():
    parser = argparse.ArgumentParser(description=__doc__, allow_abbrev=False)
    parser.add_argument("--channel", default="can0", help="SocketCAN interface")
    parser.add_argument("--node-id", type=lambda v: int(v, 0), default=0x20, help="node-ID")
    parser.add_argument("--timeout", type=float, default=60.0, help="seconds to wait")
    args = parser.parse_args()

    network = canopen.Network()
    network.connect(interface="socketcan", channel=args.channel)
    node = create_node(args.node_id)
    network.add_node(node)

    node.nmt.state = "PRE-OPERATIONAL"
    node.nmt.start_heartbeat(EXPECTED_HEARTBEAT_MS)
    print(f"node 0x{args.node_id:02x} waiting for the master, reset the Zephyr board now")

    try:
        failed = check(node, args.timeout)
    finally:
        node.nmt.stop_heartbeat()
        network.disconnect()

    print(f"\n{'ALL CHECKS PASSED' if failed == 0 else f'{failed} CHECK(S) FAILED'}")

    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
