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

# Matches same in coredump.c
XTENSA_BLOCK_HDR_DUMMY_SOC = 255


# Must match --soc arg options; see get_soc
class XtensaSoc(Enum):
    UNKNOWN = 0
    SAMPLE_CONTROLLER = 1
    ESP32 = 2
    INTEL_ADSP_CAVS = 3
    ESP32S2 = 4
    ESP32S3 = 5
    DC233C = 6


# If a SOC can be built by two separate toolchains, there is a
# chance that the GDBs provided by the toolchains will assign
# different indices to the same registers. For example, the
# Intel ADSP family of SOCs can be built with both Zephyr's
# SDK and Cadence's XCC toolchain. With the same SOC APL,
# the SDK's GDB assigns PC the index 0, while XCC's GDB assigns
# it the index 32.
class XtensaToolchain(Enum):
    UNKNOWN = 0
    ZEPHYR = 1
    XCC = 2


def get_gdb_reg_definition(soc, toolchain):
    if soc == XtensaSoc.SAMPLE_CONTROLLER:
        return GdbRegDef_Sample_Controller
    elif soc == XtensaSoc.ESP32:
        return GdbRegDef_ESP32
    elif soc == XtensaSoc.INTEL_ADSP_CAVS:
        if toolchain == XtensaToolchain.ZEPHYR:
            return GdbRegDef_Intel_Adsp_CAVS_Zephyr
        elif toolchain == XtensaToolchain.XCC:
            return GdbRegDef_Intel_Adsp_CAVS_XCC
        else:
            raise NotImplementedError
    elif soc == XtensaSoc.ESP32S2:
        return GdbRegDef_ESP32S2
    elif soc == XtensaSoc.ESP32S3:
        return GdbRegDef_ESP32S3
    elif soc == XtensaSoc.DC233C:
        return GdbRegDef_DC233C
    else:
        raise NotImplementedError


class ExceptionCodes(Enum):
    # Matches arch/xtensa/core/fatal.c->xtensa_exccause
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
    COPROCESSOR_DISABLED_START = 32
    COPROCESSOR_DISABLED_END = 39
    Z_EXCEPT_REASON = 63
    # Others (reserved / unknown) map to default signal


# Slot order matches enum xtensa_extra_register in arch/xtensa/core/coredump.c.
EXTRA_REGISTERS = (
    "WINDOWBASE",
    "WINDOWSTART",
    "THREADPTR",
    "EPC1",
    "EPC2",
    "EPC3",
    "EPC4",
    "EPC5",
    "EPC6",
    "EPC7",
    "DEPC",
    "EPS2",
    "EPS3",
    "EPS4",
    "EPS5",
    "EPS6",
    "EPS7",
    "EXCSAVE1",
)


class GdbStub_Xtensa(GdbStub):
    GDB_SIGNAL_DEFAULT = 7

    # Mapping based on section 4.4.1.5 of the
    # Xtensa ISA Reference Manual (Table 4–64. Exception Causes)
    # Somewhat arbitrary; included only because GDB requests it
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
        ExceptionCodes.Z_EXCEPT_REASON: 6,
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

        self.version = struct.unpack('<H', arch_data_blk[1:3])[0]
        if self.version not in (1, 2, 3):
            raise ValueError(f"Unsupported Xtensa coredump version: {self.version}")
        logger.debug(f"Xtensa GDB stub version: {self.version}")

        # Get SOC and toolchain to get correct format for unpack
        self.soc = XtensaSoc(bytearray(arch_data_blk)[0])
        logger.debug(f"Xtensa SOC: {self.soc.name}")

        if self.version >= 2:
            self.toolchain = XtensaToolchain(bytearray(arch_data_blk)[3])
            arch_data_blk_regs = arch_data_blk[4:]
        else:
            # v1 only supported ESP32 and sample_controller, each of which
            # only build with one toolchain
            self.toolchain = XtensaToolchain.ZEPHYR
            arch_data_blk_regs = arch_data_blk[3:]

        logger.debug(f"Xtensa toolchain: {self.toolchain.name}")

        self.gdb_reg_def = get_gdb_reg_definition(self.soc, self.toolchain)

        self.extra_valid = 0
        if self.version == 3:
            self.extra_valid = struct.unpack("<I", arch_data_blk_regs[:4])[0]
            arch_data_blk_regs = arch_data_blk_regs[4:]
            if self.extra_valid >> len(EXTRA_REGISTERS):
                raise ValueError("Unknown Xtensa extra register slots")

        self.extra_registers = {}
        if self.version == 3:
            base_size = struct.calcsize(self.gdb_reg_def.ARCH_DATA_BLK_STRUCT_REGS)
            values = iter(
                struct.unpack(f"<{self.extra_valid.bit_count()}I", arch_data_blk_regs[base_size:])
            )
            for index, name in enumerate(EXTRA_REGISTERS):
                if self.extra_valid & (1 << index):
                    value = next(values)
                    self.extra_registers[name] = value
                    logger.info("Xtensa extra register %s: 0x%08x", name, value)
            arch_data_blk_regs = arch_data_blk_regs[:base_size]

        tu = struct.unpack(self.gdb_reg_def.ARCH_DATA_BLK_STRUCT_REGS, arch_data_blk_regs)

        self.registers = dict()

        self.map_registers(tu)
        for register in self.gdb_reg_def.ExtraRegNum:
            if register.name in self.extra_registers:
                self.registers[register.value] = self.extra_registers[register.name]

    def map_registers(self, tu):
        i = 0
        for r in self.gdb_reg_def.RegNum:
            reg_num = r.value
            # Dummy WINDOWBASE and WINDOWSTART to enable GDB
            # without dumping them and all AR registers;
            # avoids interfering with interrupts / exceptions
            if r == self.gdb_reg_def.RegNum.WINDOWBASE:
                self.registers[reg_num] = 0
            elif r == self.gdb_reg_def.RegNum.WINDOWSTART:
                self.registers[reg_num] = 1
            else:
                if r == self.gdb_reg_def.RegNum.EXCCAUSE:
                    self.exception_code = tu[i]
                self.registers[reg_num] = tu[i]
            i = i + 1

    def compute_signal(self):
        sig = self.GDB_SIGNAL_DEFAULT
        code = ExceptionCodes(self.exception_code)

        if code is None:
            sig = self.GDB_SIGNAL_DEFAULT

        if code in self.GDB_SIGNAL_MAPPING:
            sig = self.GDB_SIGNAL_MAPPING[code]
        elif (
            ExceptionCodes.COPROCESSOR_DISABLED_START.value
            <= code
            <= ExceptionCodes.COPROCESSOR_DISABLED_END.value
        ):
            sig = 8

        self.gdb_signal = sig

    def handle_register_group_read_packet(self):
        max_reg = max(register.value for register in self.gdb_reg_def.RegNum)
        size = min((max_reg + 1) * 4, self.gdb_reg_def.SOC_GDB_GPKT_BIN_SIZE)
        pkt = bytearray(b'x' * (size * 2))
        offsets = getattr(self.gdb_reg_def, "GDB_G_PKT_OFFSETS", {})
        for reg_num, value in self.registers.items():
            offset = offsets.get(reg_num, reg_num * 4)
            if offset + 4 <= size:
                encoded = binascii.hexlify(struct.pack(self.reg_fmt, value))
                pkt[offset * 2 : (offset + 4) * 2] = encoded

        self.put_gdb_packet(bytes(pkt))

    def handle_register_single_read_packet(self, pkt):
        # format is pXX, where XX is the hex representation of the idx
        regIdx = int('0x' + pkt[1:].decode('utf8'), 16)
        try:
            bval = struct.pack(self.reg_fmt, self.registers[regIdx])
            self.put_gdb_packet(binascii.hexlify(bval))
        except KeyError:
            self.put_gdb_packet(b'x' * 8)


# The following classes map registers to their index used by
# the GDB of a specific SOC and toolchain. See xtensa_config.c.

# WARNING: IF YOU CHANGE THE ORDER OF THE REGISTERS IN ONE
# MAPPING, YOU MUST CHANGE THE ORDER TO MATCH IN THE OTHERS
# AND IN arch/xtensa/core/coredump.c's xtensa_arch_block.r.
# See map_registers.

# For the same reason, even though the WINDOWBASE and WINDOWSTART
# values are dummied by this script, they have to be last in the
# mapping below.


# sample_controller is unique to Zephyr SDK
# sdk-ng -> overlays/xtensa_sample_controller/gdb/gdb/xtensa-config.c
class GdbRegDef_Sample_Controller:
    ARCH_DATA_BLK_STRUCT_REGS = '<IIIIIIIIIIIIIIIIIIIIII'

    # This fits the maximum possible register index (110).
    # Unlike on ESP32 GDB, there doesn't seem to be an
    # actual hard limit to how big the g packet can be.
    SOC_GDB_GPKT_BIN_SIZE = 444

    # Extra registers do not change the base payload order in RegNum.
    class ExtraRegNum(Enum):
        EPC1 = 51
        EPC2 = 52
        EPC3 = 53
        EPC4 = 54
        EPC5 = 55
        EPC6 = 56
        EPC7 = 57
        DEPC = 58
        EPS2 = 59
        EPS3 = 60
        EPS4 = 61
        EPS5 = 62
        EPS6 = 63
        EPS7 = 64
        EXCSAVE1 = 65

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


# xtensa-overlays -> xtensa_esp32/gdb/gdb/xtensa-config.c
class GdbRegDef_ESP32:
    ARCH_DATA_BLK_STRUCT_REGS = '<IIIIIIIIIIIIIIIIIIIIIIIII'
    SOC_GDB_GPKT_BIN_SIZE = 420

    # Extra registers do not change the base payload order in RegNum.
    class ExtraRegNum(Enum):
        THREADPTR = 74
        EPC1 = 116
        EPC2 = 117
        EPC3 = 118
        EPC4 = 119
        EPC5 = 120
        EPC6 = 121
        EPC7 = 122
        DEPC = 123
        EPS2 = 124
        EPS3 = 125
        EPS4 = 126
        EPS5 = 127
        EPS6 = 128
        EPS7 = 129
        EXCSAVE1 = 130

    class RegNum(Enum):
        PC = 0
        EXCCAUSE = 143
        EXCVADDR = 149
        SAR = 68
        PS = 73
        SCOMPARE1 = 76
        A0 = 157
        A1 = 158
        A2 = 159
        A3 = 160
        A4 = 161
        A5 = 162
        A6 = 163
        A7 = 164
        A8 = 165
        A9 = 166
        A10 = 167
        A11 = 168
        A12 = 169
        A13 = 170
        A14 = 171
        A15 = 172
        LBEG = 65
        LEND = 66
        LCOUNT = 67
        WINDOWBASE = 69
        WINDOWSTART = 70


class GdbRegDef_ESP32S2:
    ARCH_DATA_BLK_STRUCT_REGS = '<IIIIIIIIIIIIIIIIIIIII'
    SOC_GDB_GPKT_BIN_SIZE = 420

    # Extra registers do not change the base payload order in RegNum.
    class ExtraRegNum(Enum):
        THREADPTR = 71
        EPC1 = 82
        EPC2 = 83
        EPC3 = 84
        EPC4 = 85
        EPC5 = 86
        EPC6 = 87
        EPC7 = 88
        DEPC = 89
        EPS2 = 90
        EPS3 = 91
        EPS4 = 92
        EPS5 = 93
        EPS6 = 94
        EPS7 = 95
        EXCSAVE1 = 96

    class RegNum(Enum):
        PC = 0
        EXCCAUSE = 99
        EXCVADDR = 115
        SAR = 65
        PS = 70
        A0 = 155
        A1 = 156
        A2 = 157
        A3 = 158
        A4 = 159
        A5 = 160
        A6 = 161
        A7 = 162
        A8 = 163
        A9 = 164
        A10 = 165
        A11 = 166
        A12 = 167
        A13 = 168
        A14 = 169
        A15 = 170
        WINDOWBASE = 66
        WINDOWSTART = 67


class GdbRegDef_ESP32S3:
    ARCH_DATA_BLK_STRUCT_REGS = '<IIIIIIIIIIIIIIIIIIIIIIIII'
    SOC_GDB_GPKT_BIN_SIZE = 420

    # Extra registers do not change the base payload order in RegNum.
    class ExtraRegNum(Enum):
        THREADPTR = 74
        EPC1 = 139
        EPC2 = 140
        EPC3 = 141
        EPC4 = 142
        EPC5 = 143
        EPC6 = 144
        EPC7 = 145
        DEPC = 146
        EPS2 = 147
        EPS3 = 148
        EPS4 = 149
        EPS5 = 150
        EPS6 = 151
        EPS7 = 152
        EXCSAVE1 = 153

    class RegNum(Enum):
        PC = 0
        EXCCAUSE = 166
        EXCVADDR = 172
        SAR = 68
        PS = 73
        SCOMPARE1 = 76
        A0 = 212
        A1 = 213
        A2 = 214
        A3 = 215
        A4 = 216
        A5 = 217
        A6 = 218
        A7 = 219
        A8 = 220
        A9 = 221
        A10 = 222
        A11 = 223
        A12 = 224
        A13 = 225
        A14 = 226
        A15 = 227
        LBEG = 65
        LEND = 66
        LCOUNT = 67
        WINDOWBASE = 69
        WINDOWSTART = 70


# sdk-ng -> overlays/xtensa_intel_apl_adsp.tar.gz -> gdb/gdb/xtensa-config.c
class GdbRegDef_Intel_Adsp_CAVS_Zephyr:
    ARCH_DATA_BLK_STRUCT_REGS = '<IIIIIIIIIIIIIIIIIIIIIIIII'

    # If you send all the registers below (up to index 173)
    # GDB incorrectly assigns 0 to EXCCAUSE / EXCVADDR... for some
    # reason. Since APL GDB sends p packets for every An register
    # even if it was sent in the g packet, I arbitrarily shrunk the
    # G packet to include up to A1, which fixed the issue.
    SOC_GDB_GPKT_BIN_SIZE = 640

    # Extra registers do not change the base payload order in RegNum.
    class ExtraRegNum(Enum):
        THREADPTR = 75
        EPC1 = 121
        EPC2 = 122
        EPC3 = 123
        EPC4 = 124
        EPC5 = 125
        EPC6 = 126
        EPC7 = 127
        DEPC = 128
        EPS2 = 129
        EPS3 = 130
        EPS4 = 131
        EPS5 = 132
        EPS6 = 133
        EPS7 = 134
        EXCSAVE1 = 135

    # Audio registers occupy more than four bytes in the GDB g packet.
    GDB_G_PKT_OFFSETS = {
        121: 564,
        122: 568,
        123: 572,
        124: 576,
        125: 580,
        126: 584,
        127: 588,
        128: 592,
        129: 596,
        130: 600,
        131: 604,
        132: 608,
        133: 612,
        134: 616,
        135: 620,
        148: 672,
        154: 696,
        158: 712,
        159: 716,
        160: 720,
        161: 724,
        162: 728,
        163: 732,
        164: 736,
        165: 740,
        166: 744,
        167: 748,
        168: 752,
        169: 756,
        170: 760,
        171: 764,
        172: 768,
        173: 772,
    }

    class RegNum(Enum):
        PC = 0
        EXCCAUSE = 148
        EXCVADDR = 154
        SAR = 68
        PS = 74
        SCOMPARE1 = 77
        A0 = 158
        A1 = 159
        A2 = 160
        A3 = 161
        A4 = 162
        A5 = 163
        A6 = 164
        A7 = 165
        A8 = 166
        A9 = 167
        A10 = 168
        A11 = 169
        A12 = 170
        A13 = 171
        A14 = 172
        A15 = 173
        LBEG = 65
        LEND = 66
        LCOUNT = 67
        WINDOWBASE = 70
        WINDOWSTART = 71


# Reverse-engineered from:
# sof -> src/debug/gdb/gdb.c
# sof -> src/arch/xtensa/include/xtensa/specreg.h
class GdbRegDef_Intel_Adsp_CAVS_XCC:
    ARCH_DATA_BLK_STRUCT_REGS = '<IIIIIIIIIIIIIIIIIIIIIIIII'

    # xt-gdb doesn't use the g packet at all
    SOC_GDB_GPKT_BIN_SIZE = 0

    # Extra registers do not change the base payload order in RegNum.
    class ExtraRegNum(Enum):
        THREADPTR = 0x3E7
        EPC1 = 0x2B1
        EPC2 = 0x2B2
        EPC3 = 0x2B3
        EPC4 = 0x2B4
        EPC5 = 0x2B5
        EPC6 = 0x2B6
        EPC7 = 0x2B7
        DEPC = 0x2C0
        EPS2 = 0x2C2
        EPS3 = 0x2C3
        EPS4 = 0x2C4
        EPS5 = 0x2C5
        EPS6 = 0x2C6
        EPS7 = 0x2C7
        EXCSAVE1 = 0x2D1

    class RegNum(Enum):
        PC = 32
        EXCCAUSE = 744
        EXCVADDR = 750
        SAR = 515
        PS = 742
        SCOMPARE1 = 524
        A0 = 256
        A1 = 257
        A2 = 258
        A3 = 259
        A4 = 260
        A5 = 261
        A6 = 262
        A7 = 263
        A8 = 264
        A9 = 265
        A10 = 266
        A11 = 267
        A12 = 268
        A13 = 269
        A14 = 270
        A15 = 271
        LBEG = 512
        LEND = 513
        LCOUNT = 514
        WINDOWBASE = 584
        WINDOWSTART = 585


# sdk-ng -> overlays/xtensa_dc233c/gdb/gdb/xtensa-config.c
class GdbRegDef_DC233C:
    ARCH_DATA_BLK_STRUCT_REGS = '<IIIIIIIIIIIIIIIIIIIIIIIII'

    SOC_GDB_GPKT_BIN_SIZE = 568

    # Extra registers do not change the base payload order in RegNum.
    class ExtraRegNum(Enum):
        THREADPTR = 43
        EPC1 = 66
        EPC2 = 67
        EPC3 = 68
        EPC4 = 69
        EPC5 = 70
        EPC6 = 71
        EPC7 = 72
        DEPC = 73
        EPS2 = 74
        EPS3 = 75
        EPS4 = 76
        EPS5 = 77
        EPS6 = 78
        EPS7 = 79
        EXCSAVE1 = 80

    class RegNum(Enum):
        PC = 0
        EXCCAUSE = 93
        EXCVADDR = 99
        SAR = 36
        PS = 42
        SCOMPARE1 = 44
        A0 = 105
        A1 = 106
        A2 = 107
        A3 = 108
        A4 = 109
        A5 = 110
        A6 = 111
        A7 = 112
        A8 = 113
        A9 = 114
        A10 = 115
        A11 = 116
        A12 = 117
        A13 = 118
        A14 = 119
        A15 = 120
        LBEG = 33
        LEND = 34
        LCOUNT = 35
        WINDOWBASE = 38
        WINDOWSTART = 39
