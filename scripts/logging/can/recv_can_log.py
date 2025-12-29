#!/usr/bin/env python3
# Copyright (c) 2026 Paul Wedeck <paulwedeck@gmail.com>
# SPDX-License-Identifier: Apache-2.0

import sys

import can

if len(sys.argv) == 1:
    print(f"{sys.argv[0]}: <can id...>")
    print("can id as 3 (short id) or 8 (extended id) hex chars")
    print(
        "For CAN configuration, see: https://python-can.readthedocs.io/en/stable/configuration.html"
    )
    quit()

filters = []

for arg in sys.argv[1:]:
    arg_num = 0
    try:
        arg_num = int(arg)
    except ValueError:
        sys.exit(f"CAN ID {arg} is not a number")

    if len(arg) == 3:
        filters += [{"can_id": arg_num, "can_mask": 0x7FF, "extended": False}]
    elif len(arg) == 8:
        filters += [{"can_id": arg_num, "can_mask": 0x1FFFFFFF, "extended": True}]
    else:
        sys.exit(f"CAN ID {arg} is too long/short")

buffer = {}

for f in filters:
    key = (f["can_id"], f["extended"])
    buffer[key] = ""

with can.Bus(can_filters=filters) as bus:
    for msg in bus:
        key = (msg.arbitration_id, msg.is_extended_id)
        buffer[key] += msg.data.decode("utf-8", "backslashreplace")
        parts = buffer[key].split("\n")
        buffer[key] = parts[-1]
        for part in parts[:-1]:
            print(f"{msg.arbitration_id}: {part}")
