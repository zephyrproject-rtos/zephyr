#!/usr/bin/env python3
#
# Copyright (c) 2019 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

import logging
from ctypes import c_uint8, addressof

import lib.platforms as plat_def

import sys
sys.path.append('..')
from logtool import Loglist


def is_ascii(c):
    if 32 <= c <= 126:
        return str(chr(c))
    else:
        return "."


class Etrace:
    def __init__(self, dev, win_id=3, size=0x2000):
        self.drv = dev.drv
        self.dsp_base_p = dev.dev_info.dsp_bar.base_p
        self.win_id = win_id
        self.size = size

        self.offset = self.get_sram_win_offset(self.win_id)

        # memory map MBOX UPLINK
        self.mmap = self.drv.mmap(self.dsp_base_p + self.offset, self.size)
        self.mmap_addr = addressof(self.mmap)

    @staticmethod
    def get_sram_win_offset(win_id):
        return plat_def.FW_SRAM + (win_id * 0x20000)

    def hexdump(self):
        data = (c_uint8 * self.size).from_address(self.mmap_addr)

        i = 1
        s = ""
        a = ""
        offset = 0x00

        for r in data:
            s = s + ("%02X " % r)
            a = a + ("%s" % is_ascii(r))
            offset += 1

            if not i % 16:
                logging.info("0x%04X:  %s %s" % ((offset - 16), s, a))
                s = ""
                a = ""

            i += 1

        if s:
            logging.info("0x%04X:  %s %s" % ((offset - 16), s, a))

    def print(self):
        l = Loglist(self.mmap_addr)
        l.print()

    def save(self, output_file):
        data = (c_uint8 * self.size).from_address(self.mmap_addr)
        with open(output_file, "wb+") as f:
            f.write(data)
