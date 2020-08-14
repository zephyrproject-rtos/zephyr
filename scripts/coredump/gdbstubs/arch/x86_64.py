#!/usr/bin/env python3
#
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

import binascii
import logging
import struct

from gdbstubs.gdbstub import GdbStub


logger = logging.getLogger("gdbstub")


class RegNum():
    # Matches the enum amd64_regnum in GDB
    RAX = 0
    RBX = 1
    RCX = 2
    RDX = 3
    RSI = 4
    RDI = 5
    RBP = 6
    RSP = 7
    R8  = 8
    R9  = 9
    R10 = 10
    R11 = 11
    R12 = 12
    R13 = 13
    R14 = 14
    R15 = 15
    RIP = 16
    EFLAGS = 17
    CS = 18
    SS = 19
    DS = 20
    ES = 21
    FS = 22
    GS = 23
    FS_BASE = 24
    GS_BASE = 25
    K_GS_BASE = 26


class ExceptionVectors():
    # Matches arch/x86/include/kernel_arch_data.h
    IV_DIVIDE_ERROR = 0
    IV_DEBUG = 1
    IV_NON_MASKABLE_INTERRUPT = 2
    IV_BREAKPOINT = 3
    IV_OVERFLOW = 4
    IV_BOUND_RANGE = 5
    IV_INVALID_OPCODE = 6
    IV_DEVICE_NOT_AVAILABLE = 7
    IV_DOUBLE_FAULT = 8
    IV_COPROC_SEGMENT_OVERRUN = 9
    IV_INVALID_TSS = 10
    IV_SEGMENT_NOT_PRESENT = 11
    IV_STACK_FAULT = 12
    IV_GENERAL_PROTECTION = 13
    IV_PAGE_FAULT = 14
    IV_RESERVED = 15
    IV_X87_FPU_FP_ERROR = 16
    IV_ALIGNMENT_CHECK = 17
    IV_MACHINE_CHECK = 18
    IV_SIMD_FP = 19
    IV_VIRT_EXCEPTION = 20
    IV_SECURITY_EXCEPTION = 30


class GdbStub_x86_64(GdbStub):
    GDB_SIGNAL_DEFAULT = 7

    # Mapping is from GDB's gdb/i386-stubs.c
    GDB_SIGNAL_MAPPING = {
        ExceptionVectors.IV_DIVIDE_ERROR: 8,
        ExceptionVectors.IV_DEBUG: 5,
        ExceptionVectors.IV_BREAKPOINT: 5,
        ExceptionVectors.IV_OVERFLOW: 16,
        ExceptionVectors.IV_BOUND_RANGE: 16,
        ExceptionVectors.IV_INVALID_OPCODE: 4,
        ExceptionVectors.IV_DEVICE_NOT_AVAILABLE: 8,
        ExceptionVectors.IV_DOUBLE_FAULT: 7,
        ExceptionVectors.IV_COPROC_SEGMENT_OVERRUN: 11,
        ExceptionVectors.IV_INVALID_TSS: 11,
        ExceptionVectors.IV_SEGMENT_NOT_PRESENT: 11,
        ExceptionVectors.IV_STACK_FAULT: 11,
        ExceptionVectors.IV_GENERAL_PROTECTION: 11,
        ExceptionVectors.IV_PAGE_FAULT: 11,
        ExceptionVectors.IV_X87_FPU_FP_ERROR: 7,
    }

    GDB_G_PKT_NUM_REGS = 34

    GDB_32BIT_REGS = {
        RegNum.EFLAGS,
        RegNum.CS,
        RegNum.SS,
        RegNum.DS,
        RegNum.ES,
        RegNum.FS,
        RegNum.GS,
    }

    def __init__(self, logfile, elffile):
        super().__init__(logfile=logfile, elffile=elffile)
        self.registers = None
        self.exception_vector = None
        self.exception_code = None
        self.gdb_signal = self.GDB_SIGNAL_DEFAULT

        self.parse_arch_data_block()
        self.compute_signal()

    def parse_arch_data_block(self):
        arch_data_blk = self.logfile.get_arch_data()['data']

        arch_data_blk_struct = "<QQQQQQQQQQQQQQQQQQQQQQ"
        cfg_exception_debug = True
        if len(arch_data_blk) != struct.calcsize(arch_data_blk_struct):
            # There are fewer registers dumped
            # when CONFIG_EXCEPTION_DEBUG=n
            arch_data_blk_struct = "<QQQQQQQQQQQQQQQQQ"
            cfg_exception_debug = False

        tu = struct.unpack(arch_data_blk_struct, arch_data_blk)

        self.registers = dict()

        self.exception_vector = tu[0]
        self.exception_code = tu[1]

        self.registers[RegNum.RAX] = tu[2]
        self.registers[RegNum.RCX] = tu[3]
        self.registers[RegNum.RDX] = tu[4]
        self.registers[RegNum.RSI] = tu[5]
        self.registers[RegNum.RDI] = tu[6]
        self.registers[RegNum.RSP] = tu[7]
        self.registers[RegNum.R8 ] = tu[8]
        self.registers[RegNum.R9 ] = tu[9]
        self.registers[RegNum.R10] = tu[10]
        self.registers[RegNum.R11] = tu[11]
        self.registers[RegNum.RIP] = tu[12]
        self.registers[RegNum.EFLAGS] = tu[13]
        self.registers[RegNum.CS] = tu[14]
        self.registers[RegNum.SS] = tu[15]
        self.registers[RegNum.RBP] = tu[16]

        if cfg_exception_debug:
            self.registers[RegNum.RBX] = tu[17]
            self.registers[RegNum.R12] = tu[18]
            self.registers[RegNum.R13] = tu[19]
            self.registers[RegNum.R14] = tu[20]
            self.registers[RegNum.R15] = tu[21]

    def compute_signal(self):
        sig = self.GDB_SIGNAL_DEFAULT
        vector = self.exception_vector

        if vector is None:
            sig = self.GDB_SIGNAL_DEFAULT

        # Map vector number to GDB signal number
        if vector in self.GDB_SIGNAL_MAPPING:
            sig = self.GDB_SIGNAL_MAPPING[vector]

        self.gdb_signal = sig

    def handle_register_group_read_packet(self):
        idx = 0
        pkt = b''

        while idx < self.GDB_G_PKT_NUM_REGS:
            if idx in self.GDB_32BIT_REGS:
                reg_fmt = "<I"
                reg_bytes = 4
            else:
                reg_fmt = "<Q"
                reg_bytes = 8

            if idx in self.registers:
                bval = struct.pack(reg_fmt, self.registers[idx])
                pkt += binascii.hexlify(bval)
            else:
                # Register not in coredump -> unknown value
                # Send in "xxxxxxxx"
                pkt += b'x' * (reg_bytes * 2)

            idx += 1

        self.put_gdb_packet(pkt)

    def handle_register_single_read_packet(self, pkt):
        # Mark registers as "<unavailable>".
        # 'p' packets are usually used for registers
        # other than the general ones (e.g. eax, ebx)
        # so we can safely reply "xxxxxxxx" here.
        self.put_gdb_packet(b'x' * 16)
