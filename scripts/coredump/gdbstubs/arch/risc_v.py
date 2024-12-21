#!/usr/bin/env python3
#
# Copyright (c) 2021 Facebook, Inc. and its affiliates
#
# SPDX-License-Identifier: Apache-2.0

import binascii
import logging
import struct

from gdbstubs.gdbstub import GdbStub


logger = logging.getLogger("gdbstub")

class RegNum():
    ZERO = 0
    RA = 1
    SP = 2
    GP = 3
    TP = 4
    T0 = 5
    T1 = 6
    T2 = 7
    FP = 8
    S1 = 9
    A0 = 10
    A1 = 11
    A2 = 12
    A3 = 13
    A4 = 14
    A5 = 15
    A6 = 16
    A7 = 17
    S2 = 18
    S3 = 19
    S4 = 20
    S5 = 21
    S6 = 22
    S7 = 23
    S8 = 24
    S9 = 25
    S10 = 26
    S11 = 27
    T3 = 28
    T4 = 29
    T5 = 30
    T6 = 31
    PC = 32


class GdbStub_RISC_V(GdbStub):
    ARCH_DATA_BLK_STRUCT    = "<IIIIIIIIIIIIIIIIIII"
    ARCH_DATA_BLK_STRUCT_2  = "<QQQQQQQQQQQQQQQQQQQ"

    GDB_SIGNAL_DEFAULT = 7

    GDB_G_PKT_NUM_REGS = 33

    def __init__(self, logfile, elffile):
        super().__init__(logfile=logfile, elffile=elffile)
        self.registers = None
        self.gdb_signal = self.GDB_SIGNAL_DEFAULT

        self.parse_arch_data_block()

    def parse_arch_data_block(self):
        arch_data_blk = self.logfile.get_arch_data()['data']
        self.arch_data_ver = self.logfile.get_arch_data()['hdr_ver']

        if self.arch_data_ver == 1:
            tu = struct.unpack(self.ARCH_DATA_BLK_STRUCT, arch_data_blk)
        elif self.arch_data_ver == 2:
            tu = struct.unpack(self.ARCH_DATA_BLK_STRUCT_2, arch_data_blk)

        self.registers = dict()

        self.registers[RegNum.RA] = tu[0]
        self.registers[RegNum.T0] = tu[2]
        self.registers[RegNum.T1] = tu[3]
        self.registers[RegNum.T2] = tu[4]
        self.registers[RegNum.A0] = tu[5]
        self.registers[RegNum.A1] = tu[6]
        self.registers[RegNum.A2] = tu[7]
        self.registers[RegNum.A3] = tu[8]
        self.registers[RegNum.A4] = tu[9]
        self.registers[RegNum.A5] = tu[10]
        self.registers[RegNum.A6] = tu[11]
        self.registers[RegNum.A7] = tu[12]
        self.registers[RegNum.T3] = tu[13]
        self.registers[RegNum.T4] = tu[14]
        self.registers[RegNum.T5] = tu[15]
        self.registers[RegNum.T6] = tu[16]
        self.registers[RegNum.PC] = tu[17]
        self.registers[RegNum.SP] = tu[18]

    def handle_register_group_read_packet(self):
        reg_fmt = "<I" if self.arch_data_ver == 1 else "<Q"

        idx = 0
        pkt = b''

        while idx < self.GDB_G_PKT_NUM_REGS:
            if idx in self.registers:
                bval = struct.pack(reg_fmt, self.registers[idx])
                pkt += binascii.hexlify(bval)
            else:
                # Register not in coredump -> unknown value
                # Send in "xxxxxxxx"
                length = 8 if self.arch_data_ver == 1 else 16
                pkt += b'x' * length

            idx += 1

        self.put_gdb_packet(pkt)

    def handle_register_single_read_packet(self, pkt):
        # Mark registers as "<unavailable>". 'p' packets are not sent for the registers
        # currently handled in this file so we can safely reply "xxxxxxxx" here.
        length = 8 if self.arch_data_ver == 1 else 16
        self.put_gdb_packet(b'x' * length)
