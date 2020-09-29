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
    # Matches the enum i386_regnum in GDB
    EAX = 0
    ECX = 1
    EDX = 2
    EBX = 3
    ESP = 4
    EBP = 5
    ESI = 6
    EDI = 7
    EIP = 8
    EFLAGS = 9
    CS = 10
    SS = 11
    DS = 12
    ES = 13
    FS = 14
    GS = 15


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


class GdbStub_x86(GdbStub):
    ARCH_DATA_BLK_STRUCT = "<IIIIIIIIIIIII"

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

    GDB_G_PKT_NUM_REGS = 16

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
        tu = struct.unpack(self.ARCH_DATA_BLK_STRUCT, arch_data_blk)

        self.registers = dict()

        self.exception_vector = tu[0]
        self.exception_code = tu[1]

        self.registers[RegNum.EAX] = tu[2]
        self.registers[RegNum.ECX] = tu[3]
        self.registers[RegNum.EDX] = tu[4]
        self.registers[RegNum.EBX] = tu[5]
        self.registers[RegNum.ESP] = tu[6]
        self.registers[RegNum.EBP] = tu[7]
        self.registers[RegNum.ESI] = tu[8]
        self.registers[RegNum.EDI] = tu[9]
        self.registers[RegNum.EIP] = tu[10]
        self.registers[RegNum.EFLAGS] = tu[11]
        self.registers[RegNum.CS] = tu[12]

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
        reg_fmt = "<I"

        idx = 0
        pkt = b''

        while idx < self.GDB_G_PKT_NUM_REGS:
            if idx in self.registers:
                bval = struct.pack(reg_fmt, self.registers[idx])
                pkt += binascii.hexlify(bval)
            else:
                # Register not in coredump -> unknown value
                # Send in "xxxxxxxx"
                pkt += b'x' * 8

            idx += 1

        self.put_gdb_packet(pkt)

    def handle_register_single_read_packet(self, pkt):
        # Mark registers as "<unavailable>".
        # 'p' packets are usually used for registers
        # other than the general ones (e.g. eax, ebx)
        # so we can safely reply "xxxxxxxx" here.
        self.put_gdb_packet(b'x' * 8)
