#!/usr/bin/env python3
# Copyright (c) 2022 Rodrigo Peixoto <rodrigopex@gmail.com>
# SPDX-License-Identifier: Apache-2.0
import argparse
import json

import serial

j = """
[
    {"name":"version","on_changed": false, "read_only": true, "message_size": 4},
    {"name":"sensor_data","on_changed": true, "read_only": false, "message_size":8},
    {"name":"start_measurement","on_changed": false, "read_only": false, "message_size": 1},
    {"name":"finish","on_changed": false, "read_only": false, "message_size": 1}
]
"""

parser = argparse.ArgumentParser(description='Read zbus events via serial.', allow_abbrev=False)
parser.add_argument("port", type=str, help='The tty or COM port to be used')

args = parser.parse_args()

channels = json.loads(j)


def fetch_sentence():
    name = b""
    while True:
        b = ser.read(size=1)
        if b == b",":
            break
        name += b
    print(name)
    found_msg_size = int.from_bytes(ser.read(size=1), byteorder="little")
    found_msg = ser.read(found_msg_size)
    return name, found_msg_size, found_msg


try:
    ser = serial.Serial(args.port)
    while True:
        d = ser.read()
        if d == b'$':
            channel_name, msg_size, msg = fetch_sentence()
            print(f"PUB [{channel_name}] -> {msg}")
except serial.serialutil.SerialException as e:
    print(e)
