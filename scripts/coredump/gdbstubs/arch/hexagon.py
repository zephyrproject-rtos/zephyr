#!/usr/bin/env python3
#
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
#
# SPDX-License-Identifier: Apache-2.0

import binascii
import logging
import struct

from gdbstubs.gdbstub import GdbStub

logger = logging.getLogger("gdbstub")


# Only the registers in the dump are described. DWARF numbers are the
# ones the Hexagon toolchain uses in debug info: r0-r31 are 0-31, pc is 76.
class RegNum:
    SP = 29
    FP = 30
    LR = 31
    PC = 32


REG_NAMES = [f"r{i}" for i in range(32)] + ["pc"]

REG_DWARF = list(range(32)) + [76]

REG_GENERIC = {
    RegNum.SP: "sp",
    RegNum.FP: "fp",
    RegNum.LR: "ra",
    RegNum.PC: "pc",
}


class GdbStub_Hexagon(GdbStub):
    # r0-r31, pc, event_type, g0-g3
    ARCH_DATA_BLK_STRUCT = "<" + "I" * 38

    GDB_SIGNAL_DEFAULT = 7

    def __init__(self, logfile, elffile):
        super().__init__(logfile=logfile, elffile=elffile)
        self.registers = None
        self.gdb_signal = self.GDB_SIGNAL_DEFAULT

        self.parse_arch_data_block()

    def parse_arch_data_block(self):
        arch_data_blk = self.logfile.get_arch_data()['data']
        tu = struct.unpack(self.ARCH_DATA_BLK_STRUCT, arch_data_blk)

        self.registers = list(tu[:33])

        event_type, gsr, badva = tu[33], tu[35], tu[37]
        logger.info(f"Event {event_type}, GSR 0x{gsr:08x}, BADVA 0x{badva:08x}")

    def handle_register_group_read_packet(self):
        pkt = b''.join(binascii.hexlify(struct.pack("<I", r)) for r in self.registers)
        self.put_gdb_packet(pkt)

    def handle_register_single_read_packet(self, pkt):
        try:
            regnum = int(pkt[1:], 16)
        except ValueError:
            regnum = -1

        if 0 <= regnum < len(self.registers):
            self.put_gdb_packet(binascii.hexlify(struct.pack("<I", self.registers[regnum])))
        else:
            self.put_gdb_packet(b"E01")

    def handle_general_query_packet(self, pkt):
        # lldb has no Hexagon register layout of its own, so describe it.
        if pkt.startswith(b"qRegisterInfo"):
            try:
                regnum = int(pkt[13:], 16)
            except ValueError:
                regnum = -1

            if not 0 <= regnum < len(REG_NAMES):
                self.put_gdb_packet(b"E45")
                return

            info = (
                f"name:{REG_NAMES[regnum]};bitsize:32;offset:{regnum * 4};"
                f"encoding:uint;format:hex;set:General Purpose Registers;"
                f"dwarf:{REG_DWARF[regnum]};"
            )
            if regnum in REG_GENERIC:
                info += f"generic:{REG_GENERIC[regnum]};"
            self.put_gdb_packet(info.encode())
            return

        super().handle_general_query_packet(pkt)
