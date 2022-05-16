#!/usr/bin/env python3
#
# Copyright (c) 2021 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

import binascii
import logging
import struct

from enum import Enum
from gdbstubs.gdbstub import GdbStub

logger = logging.getLogger("gdbstub")


class XtensaSoc():
    UNKNOWN = 0
    SAMPLE_CONTROLLER = 1
    ESP32 = 2


def get_soc_definition(soc):
    if soc == XtensaSoc.SAMPLE_CONTROLLER:
        return XtensaSoc_SampleController
    elif soc == XtensaSoc.ESP32:
        return XtensaSoc_ESP32
    else:
        raise NotImplementedError


class ExceptionCodes():
    # Matches arch/xtensa/core/fatal.c->z_xtensa_exccause
    ILLEGAL_INSTRUCTION = 0
    # Syscall not fatal
    INSTR_FETCH_ERROR = 2
    LOAD_STORE_ERROR = 3
    # Level-1 interrupt not fatal
    ALLOCA = 5
    DIVIDE_BY_ZERO = 6
    PRIVILEGED = 8
    LOAD_STORE_ALIGNMENT = 9
    INSTR_PIF_DATA_ERROR = 12
    LOAD_STORE_PIF_DATA_ERROR = 13
    INSTR_PIF_ADDR_ERROR = 14
    LOAD_STORE_PIF_ADDR_ERROR = 15
    INSTR_TLB_MISS = 16
    INSTR_TLB_MULTI_HIT = 17
    INSTR_FETCH_PRIVILEGE = 18
    INST_FETCH_PROHIBITED = 20
    LOAD_STORE_TLB_MISS = 24
    LOAD_STORE_TLB_MULTI_HIT = 25
    LOAD_STORE_PRIVILEGE = 26
    LOAD_PROHIBITED = 28
    STORE_PROHIBITED = 29
    # Coprocessor disabled spans 32 - 39
    # Others (reserved / unknown) map to default signal


class GdbStub_Xtensa(GdbStub):

    GDB_SIGNAL_DEFAULT = 7

    # Mapping based on section 4.4.1.5 of the
    # Xtensa ISA Reference Manual
    # (Table 4–64. Exception Causes)
    GDB_SIGNAL_MAPPING = {
        ExceptionCodes.ILLEGAL_INSTRUCTION: 4,
        ExceptionCodes.INSTR_FETCH_ERROR: 7,
        ExceptionCodes.LOAD_STORE_ERROR: 11,
        ExceptionCodes.ALLOCA: 7,
        ExceptionCodes.DIVIDE_BY_ZERO: 8,
        ExceptionCodes.PRIVILEGED: 11,
        ExceptionCodes.LOAD_STORE_ALIGNMENT: 7,
        ExceptionCodes.INSTR_PIF_DATA_ERROR: 7,
        ExceptionCodes.LOAD_STORE_PIF_DATA_ERROR: 7,
        ExceptionCodes.INSTR_PIF_ADDR_ERROR: 11,
        ExceptionCodes.LOAD_STORE_PIF_ADDR_ERROR: 11,
        ExceptionCodes.INSTR_TLB_MISS: 11,
        ExceptionCodes.INSTR_TLB_MULTI_HIT: 11,
        ExceptionCodes.INSTR_FETCH_PRIVILEGE: 11,
        ExceptionCodes.INST_FETCH_PROHIBITED: 11,
        ExceptionCodes.LOAD_STORE_TLB_MISS: 11,
        ExceptionCodes.LOAD_STORE_TLB_MULTI_HIT: 11,
        ExceptionCodes.LOAD_STORE_PRIVILEGE: 11,
        ExceptionCodes.LOAD_PROHIBITED: 11,
        ExceptionCodes.STORE_PROHIBITED: 11,
    }

    reg_fmt = "<I"

    def __init__(self, logfile, elffile):
        super().__init__(logfile=logfile, elffile=elffile)
        self.registers = None
        self.exception_code = None
        self.gdb_signal = self.GDB_SIGNAL_DEFAULT

        self.parse_arch_data_block()
        self.compute_signal()


    def parse_arch_data_block(self):
        arch_data_blk = self.logfile.get_arch_data()['data']

        # Get SOC in order to get correct format for unpack
        self.soc = bytearray(arch_data_blk)[0]
        self.xtensaSoc = get_soc_definition(self.soc)
        tu = struct.unpack(self.xtensaSoc.ARCH_DATA_BLK_STRUCT, arch_data_blk)

        self.registers = dict()

        self.version = tu[1]

        self.map_registers(tu)


    def map_registers(self, tu):
        i = 2
        for r in self.xtensaSoc.RegNum:
            regNum = r.value
            # Dummy WINDOWBASE and WINDOWSTART to enable GDB
            # without dumping them and all AR registers;
            # avoids interfering with interrupts / exceptions
            if r == self.xtensaSoc.RegNum.WINDOWBASE:
                self.registers[regNum] = 0
            elif r == self.xtensaSoc.RegNum.WINDOWSTART:
                self.registers[regNum] = 1
            else:
                if r == self.xtensaSoc.RegNum.EXCCAUSE:
                    self.exception_code = tu[i]
                self.registers[regNum] = tu[i]
            i += 1


    def compute_signal(self):
        sig = self.GDB_SIGNAL_DEFAULT
        code = self.exception_code

        if code is None:
            sig = self.GDB_SIGNAL_DEFAULT

        # Map cause to GDB signal number
        if code in self.GDB_SIGNAL_MAPPING:
            sig = self.GDB_SIGNAL_MAPPING[code]
        # Coprocessor disabled code spans 32 - 39
        elif 32 <= code <= 39:
            sig = 8

        self.gdb_signal = sig


    def handle_register_group_read_packet(self):
        idx = 0
        pkt = b''

        GDB_G_PKT_MAX_REG = \
            max([regNum.value for regNum in self.xtensaSoc.RegNum])

        # We try to send as many of the registers listed
        # as possible, but we are constrained by the
        # maximum length of the g packet
        while idx <= GDB_G_PKT_MAX_REG and idx * 4 < self.xtensaSoc.SOC_GDB_GPKT_BIN_SIZE:
            if idx in self.registers:
                bval = struct.pack(self.reg_fmt, self.registers[idx])
                pkt += binascii.hexlify(bval)
            else:
                # Register not in coredump -> unknown value
                # Send in "xxxxxxxx"
                pkt += b'x' * 8

            idx += 1

        self.put_gdb_packet(pkt)


    def handle_register_single_read_packet(self, pkt):
        # Mark registers as "<unavailable>". 'p' packets are not sent for the registers
        # currently handled in this file so we can safely reply "xxxxxxxx" here.
        self.put_gdb_packet(b'x' * 8)

# The following classes map registers to their index (idx) on
# a specific SOC. Since SOCs can have different numbers of
# registers (e.g. 32 VS 64 ar), the index of a register will
# differ per SOC. See xtensa_config.c.

# WARNING: IF YOU CHANGE THE ORDER OF THE REGISTERS IN ONE
# SOC's MAPPING, YOU MUST CHANGE THE ORDER TO MATCH IN THE OTHERS
# AND IN arch/xtensa/core/coredump.c's xtensa_arch_block.r.
# See map_registers.

# For the same reason, even though the WINDOWBASE and WINDOWSTART
# values are dummied by this script, they have to be last in the
# mapping below.

# sdk-ng -> overlays/xtensa_sample_controller/gdb/gdb/xtensa-config.c
class XtensaSoc_SampleController:
    ARCH_DATA_BLK_STRUCT = '<BHIIIIIIIIIIIIIIIIIIIIII'

    # This fits the maximum possible register index (110)
    # unlike on ESP32 GDB, there doesn't seem to be an
    # actual hard limit to how big the g packet can be
    SOC_GDB_GPKT_BIN_SIZE = 444

    class RegNum(Enum):
        PC = 0
        EXCCAUSE = 77
        EXCVADDR = 83
        SAR = 33
        PS = 38
        SCOMPARE1 = 39
        A0 = 89
        A1 = 90
        A2 = 91
        A3 = 92
        A4 = 93
        A5 = 94
        A6 = 95
        A7 = 96
        A8 = 97
        A9 = 98
        A10 = 99
        A11 = 100
        A12 = 101
        A13 = 102
        A14 = 103
        A15 = 104
        # LBEG, LEND, and LCOUNT not on sample_controller
        WINDOWBASE = 34
        WINDOWSTART = 35

# espressif xtensa-overlays -> xtensa_esp32/gdb/gdb/xtensa-config.c
class XtensaSoc_ESP32:
    ARCH_DATA_BLK_STRUCT = '<BHIIIIIIIIIIIIIIIIIIIIIIIII'

    # Maximum index register that can be sent in a group packet is
    # 104, which prevents us from sending An registers directly.
    # We get around this by assigning each An in the dump to ARn
    # and setting WINDOWBASE to 0 and WINDOWSTART to 1; ESP32 GDB
    # will recalculate the corresponding An
    SOC_GDB_GPKT_BIN_SIZE = 420

    class RegNum(Enum):
        PC = 0
        EXCCAUSE = 143
        EXCVADDR = 149
        SAR = 68
        PS = 73
        SCOMPARE1 = 76
        AR0 = 1
        AR1 = 2
        AR2 = 3
        AR3 = 4
        AR4 = 5
        AR5 = 6
        AR6 = 7
        AR7 = 8
        AR8 = 9
        AR9 = 10
        AR10 = 11
        AR11 = 12
        AR12 = 13
        AR13 = 14
        AR14 = 15
        AR15 = 16
        LBEG = 65
        LEND = 66
        LCOUNT = 67
        WINDOWBASE = 69
        WINDOWSTART = 70
