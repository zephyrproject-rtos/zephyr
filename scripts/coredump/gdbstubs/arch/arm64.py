#!/usr/bin/env python3
#
# Copyright (c) 2022 Huawei Technologies SASU
#
# SPDX-License-Identifier: Apache-2.0

import binascii
import logging
import struct

from gdbstubs.gdbstub import GdbStub


logger = logging.getLogger("gdbstub")


class RegNum():
    X0 = 0          # X0-X29 - 30 GP registers
    X1 = 1
    X2 = 2
    X3 = 3
    X4 = 4
    X5 = 5
    X6 = 6
    X7 = 7
    X8 = 8
    X9 = 9
    X10 = 10
    X11 = 11
    X12 = 12
    X13 = 13
    X14 = 14
    X15 = 15
    X16 = 16
    X17 = 17
    X18 = 18
    X19 = 19
    X20 = 20
    X21 = 21
    X22 = 22
    X23 = 23
    X24 = 24
    X25 = 25
    X26 = 26
    X27 = 27
    X28 = 28
    X29 = 29         # Frame pointer register
    LR      = 30     # X30 Link Register(LR)
    SP_EL0  = 31     # Stack pointer EL0 (SP_EL0)
    PC      = 32     # Program Counter (PC)


class GdbStub_ARM64(GdbStub):
    ARCH_DATA_BLK_STRUCT    = "<QQQQQQQQQQQQQQQQQQQQQQ"

    # Default signal used by all other script, just using the same
    GDB_SIGNAL_DEFAULT = 7

    # The number of registers expected by GDB
    GDB_G_PKT_NUM_REGS = 33


    def __init__(self, logfile, elffile):
        super().__init__(logfile=logfile, elffile=elffile)
        self.registers = None
        self.gdb_signal = self.GDB_SIGNAL_DEFAULT

        self.parse_arch_data_block()

    def parse_arch_data_block(self):

        arch_data_blk = self.logfile.get_arch_data()['data']

        tu = struct.unpack(self.ARCH_DATA_BLK_STRUCT, arch_data_blk)

        self.registers = dict()

        self.registers[RegNum.X0]   = tu[0]
        self.registers[RegNum.X1]   = tu[1]
        self.registers[RegNum.X2]   = tu[2]
        self.registers[RegNum.X3]   = tu[3]
        self.registers[RegNum.X4]   = tu[4]
        self.registers[RegNum.X5]   = tu[5]
        self.registers[RegNum.X6]   = tu[6]
        self.registers[RegNum.X7]   = tu[7]
        self.registers[RegNum.X8]   = tu[8]
        self.registers[RegNum.X9]   = tu[9]
        self.registers[RegNum.X10]  = tu[10]
        self.registers[RegNum.X11]  = tu[11]
        self.registers[RegNum.X12]  = tu[12]
        self.registers[RegNum.X13]  = tu[13]
        self.registers[RegNum.X14]  = tu[14]
        self.registers[RegNum.X15]  = tu[15]
        self.registers[RegNum.X16]  = tu[16]
        self.registers[RegNum.X17]  = tu[17]
        self.registers[RegNum.X18]  = tu[18]

        # Callee saved registers are not provided in __esf structure
        # So they will be omitted (set to undefined) when stub generates the
        # packet in handle_register_group_read_packet.

        self.registers[RegNum.LR]       = tu[19]
        self.registers[RegNum.SP_EL0]   = tu[20]
        self.registers[RegNum.PC]       = tu[21]


    def handle_register_group_read_packet(self):
        reg_fmt = "<Q"

        idx = 0
        pkt = b''

        while idx < self.GDB_G_PKT_NUM_REGS:
            if idx in self.registers:
                bval = struct.pack(reg_fmt, self.registers[idx])
                pkt += binascii.hexlify(bval)
            else:
                # Register not in coredump -> unknown value
                # Send in "xxxxxxxx"
                pkt += b'x' * 16

            idx += 1

        self.put_gdb_packet(pkt)

    def handle_register_single_read_packet(self, pkt):
        # Mark registers as "<unavailable>".
        # 'p' packets are usually used for registers
        # other than the general ones (e.g. eax, ebx)
        # so we can safely reply "xxxxxxxx" here.
        self.put_gdb_packet(b'x' * 16)
