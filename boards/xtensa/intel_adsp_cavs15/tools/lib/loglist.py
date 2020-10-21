#!/usr/bin/env python3
#
# Copyright (c) 2019 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

from ctypes import string_at

MAGIC = 0x55aa
SLOT_LEN = 64
SLOT_NUM = int(8192 / SLOT_LEN)

def read_bytes(buffer):
    return int.from_bytes(buffer, byteorder='little')

class Loglist:
    """Loglist class"""

    def __init__(self, argument, debug=False):
        """Constructor for the loglist takes argument filename or buffer"""

        if isinstance(argument, str):
            f = open(argument, "rb")
            self.buffer = f.read(SLOT_NUM * SLOT_LEN)
        elif isinstance(argument, int):
            self.buffer = string_at(argument, SLOT_NUM * SLOT_LEN)
        else:
            return

        self.loglist = []
        self.parse()
        self.debug = debug

    def parse_slot(self, slot):
        magic = read_bytes(slot[0:2])

        if magic == MAGIC:
            id_num = read_bytes(slot[2:4])
            logstr = slot[4:].decode(errors='replace')
            self.loglist.append((id_num, logstr))

    def parse(self):
        for x in range(0, SLOT_NUM):
            slot = self.buffer[x * SLOT_LEN : (x + 1) * SLOT_LEN]
            self.parse_slot(slot)

    def print(self):
        for pair in sorted(self.loglist):
            if self.debug:
                # Add slot number when debug is enabled
                print('[{}] : {}'.format(*pair), end='')
            else:
                print('{}'.format(pair[1]), end='')
