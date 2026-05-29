#!/usr/bin/env python3
# Copyright (c) 2022 Rodrigo Peixoto <rodrigopex@gmail.com>
# SPDX-License-Identifier: Apache-2.0
import argparse
import json
from time import sleep

import serial

j = """
[
{"name":"version","on_changed": false, "read_only": true, "message_size": 4},
{"name":"sensor_data","on_changed": true, "read_only": false, "message_size": 4},
{"name":"start_measurement","on_changed": false, "read_only": false, "message_size": 1}
]"""

channels = json.loads(j)
parser = argparse.ArgumentParser(description='Read zbus events via serial.', allow_abbrev=False)
parser.add_argument("port", type=str, help='The tty or COM port to be used')

args = parser.parse_args()


def fetch_sentence(ser):
    channel_id = int.from_bytes(ser.read(), "little")
    channel_name = channels[channel_id]['name']
    msg_size = channels[channel_id]['message_size']
    msg = ser.read(msg_size)
    ser.read(1)  # skip '*'
    return (channel_name, msg_size, msg)


def pub_start_measurement(ser, action: bool):
    print(
        f"Proxy PUB [{channels[2]['name']}] -> start measurement")
    ser.write(b'$')
    ser.write(b'\x02')  # idx
    ser.write(b'\x01')
    ser.write(b'*')
    ser.flush()


ser = serial.Serial(args.port)
pub_start_measurement(ser, True)
while True:
    d = ser.read()
    if d == b'$':
        channel_name, msg_size, msg = fetch_sentence(ser)
        if channel_name == "sensor_data":
            print(
                f"Proxy NOTIFY: [{channel_name}] -> sensor value {int.from_bytes(msg, 'little')}")
            sleep(1)
            pub_start_measurement(ser, True)
