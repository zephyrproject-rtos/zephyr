#!/usr/bin/env python3
#
# Copyright (c) 2026 Zephyr Contributors
#
# SPDX-License-Identifier: Apache-2.0

"""Shared AArch32 GDB remote-protocol register block.

Cortex-M, Cortex-R and Cortex-A all encode the coredump arch-data block
in the same 17-register order (r0-r12, sp, lr, pc, cpsr/xpsr) and share
the same V1-V3 header layouts, so the register parsing and 'g'/'p'/'P'
packet handling below is common to all of them. Anything that depends
on a specific exception model (e.g. Cortex-M's EXC_RETURN-based thread
register recovery) does NOT belong here -- see arm_cortex_m.py.
"""

import binascii
import logging
import struct

from gdbstubs.gdbstub import GdbStub

logger = logging.getLogger("gdbstub")


class RegNum:
    R0 = 0
    R1 = 1
    R2 = 2
    R3 = 3
    R4 = 4
    R5 = 5
    R6 = 6
    R7 = 7
    R8 = 8
    R9 = 9
    R10 = 10
    R11 = 11
    R12 = 12
    SP = 13
    LR = 14
    PC = 15
    XPSR = 16


class GdbStub_ARM_AArch32(GdbStub):
    ARCH_DATA_BLK_STRUCT = "<IIIIIIIII"
    ARCH_DATA_BLK_STRUCT_V2 = "<IIIIIIIIIIIIIIIII"
    ARCH_DATA_BLK_STRUCT_V3 = "<IIIIIIIIIIIIIIIIIII"

    GDB_SIGNAL_DEFAULT = 7

    GDB_G_PKT_NUM_REGS = 17

    def __init__(self, logfile, elffile):
        super().__init__(logfile=logfile, elffile=elffile)
        self.registers = None
        self.gdb_signal = self.GDB_SIGNAL_DEFAULT
        self.callee_saved_offset = None

        self.parse_arch_data_block()

    def parse_arch_data_block(self):
        arch_data_blk = self.logfile.get_arch_data()['data']
        arch_data_ver = self.logfile.get_arch_data()['hdr_ver']

        if arch_data_ver == 1:
            tu = struct.unpack(self.ARCH_DATA_BLK_STRUCT, arch_data_blk)
        elif arch_data_ver == 2:
            tu = struct.unpack(self.ARCH_DATA_BLK_STRUCT_V2, arch_data_blk)
        elif arch_data_ver == 3:
            tu = struct.unpack(self.ARCH_DATA_BLK_STRUCT_V3, arch_data_blk)

        self.registers = dict()

        self.registers[RegNum.R0] = tu[0]
        self.registers[RegNum.R1] = tu[1]
        self.registers[RegNum.R2] = tu[2]
        self.registers[RegNum.R3] = tu[3]
        self.registers[RegNum.R12] = tu[4]
        self.registers[RegNum.LR] = tu[5]
        self.registers[RegNum.PC] = tu[6]
        self.registers[RegNum.XPSR] = tu[7]
        self.registers[RegNum.SP] = tu[8]

        if arch_data_ver > 1:
            self.registers[RegNum.R4] = tu[9]
            self.registers[RegNum.R5] = tu[10]
            self.registers[RegNum.R6] = tu[11]
            self.registers[RegNum.R7] = tu[12]
            self.registers[RegNum.R8] = tu[13]
            self.registers[RegNum.R9] = tu[14]
            self.registers[RegNum.R10] = tu[15]
            self.registers[RegNum.R11] = tu[16]

        if arch_data_ver > 2:
            callee_saved_valid = tu[17]
            if callee_saved_valid:
                self.callee_saved_offset = tu[18]

    def send_registers_packet(self, registers):
        reg_fmt = "<I"

        idx = 0
        pkt = b''

        while idx < self.GDB_G_PKT_NUM_REGS:
            if idx in registers:
                bval = struct.pack(reg_fmt, registers[idx])
                pkt += binascii.hexlify(bval)
            else:
                # Register not in coredump -> unknown value
                # Send in "xxxxxxxx"
                pkt += b'x' * 8

            idx += 1

        self.put_gdb_packet(pkt)

    def handle_register_group_read_packet(self):
        self.send_registers_packet(self.registers)

    def handle_register_single_read_packet(self, pkt):
        # Mark registers as "<unavailable>".
        # 'p' packets are usually used for registers
        # other than the general ones (e.g. eax, ebx)
        # so we can safely reply "xxxxxxxx" here.
        self.put_gdb_packet(b'x' * 8)

    def handle_register_single_write_packet(self, pkt):
        pkt_str = pkt.decode("ascii")
        separator_index = pkt_str.index('=')

        if separator_index < 0:
            raise ValueError(f"Malformed register write packet: {pkt_str}")

        reg = int(pkt_str[1:separator_index], 16)
        self.registers[reg] = int.from_bytes(
            binascii.unhexlify(pkt[(separator_index + 1) :]), byteorder='little'
        )
        self.put_gdb_packet(b'+')
