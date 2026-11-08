#!/usr/bin/env python3
#
# Copyright (c) 2026 Takayuki Goto
#
# SPDX-License-Identifier: Apache-2.0

import binascii
import logging
import struct

from coredump_parser.elf_parser import ThreadInfoOffset

from gdbstubs.gdbstub import GdbStub

logger = logging.getLogger("gdbstub")


class RegNum:
    R0 = 0
    R4 = 4
    R11 = 11
    R12 = 12
    SP = 13
    LR = 14
    PC = 15
    # GDB's default (non M-profile) ARM register file puts the FPA
    # registers f0-f7 (12 bytes each) and fps between pc and cpsr.
    CPSR = 25


class GdbStub_ARM_ARMv5(GdbStub):
    # r0-r12, sp, lr, pc, cpsr, flags, callee_saved_offset
    ARCH_DATA_BLK_STRUCT = "<" + "I" * 19

    CALLEE_VALID = 1 << 0

    # CPSR.M = system, CPSR.I set
    CPSR_SWITCHED_OUT = 0x9F

    GDB_SIGNAL_DEFAULT = 7

    def __init__(self, logfile, elffile):
        super().__init__(logfile=logfile, elffile=elffile)
        self.registers = None
        self.gdb_signal = self.GDB_SIGNAL_DEFAULT
        self.callee_saved_offset = None

        self.parse_arch_data_block()

    def parse_arch_data_block(self):
        arch_data_blk = self.logfile.get_arch_data()['data']

        tu = struct.unpack(self.ARCH_DATA_BLK_STRUCT, arch_data_blk)

        self.registers = dict()
        callee_valid = (tu[17] & self.CALLEE_VALID) != 0

        for reg in range(RegNum.R0, RegNum.R12 + 1):
            if RegNum.R4 <= reg <= RegNum.R11 and not callee_valid:
                continue
            self.registers[reg] = tu[reg]

        self.registers[RegNum.SP] = tu[13]
        self.registers[RegNum.LR] = tu[14]
        self.registers[RegNum.PC] = tu[15]
        self.registers[RegNum.CPSR] = tu[16]

        self.callee_saved_offset = tu[18]

    def send_registers_packet(self, registers):
        pkt = b''

        for idx in range(RegNum.CPSR + 1):
            # f0-f7 are 12 bytes wide, everything else 4 bytes
            size = 12 if RegNum.PC < idx < RegNum.CPSR - 1 else 4

            if idx in registers:
                pkt += binascii.hexlify(struct.pack("<I", registers[idx]))
            else:
                # Register not in the core dump: unknown value
                pkt += b'x' * (size * 2)

        self.put_gdb_packet(pkt)

    def handle_register_group_read_packet(self):
        if not self.elffile.has_kernel_thread_info():
            self.send_registers_packet(self.registers)
        else:
            self.handle_thread_register_group_read_packet()

    def handle_register_single_read_packet(self, pkt):
        self.put_gdb_packet(b'x' * 8)

    def arch_supports_thread_operations(self):
        return True

    def handle_thread_register_group_read_packet(self):
        # Thread 0 is the faulting context, described by the arch block
        if self.selected_thread == 0:
            self.send_registers_packet(self.registers)
            return

        thread_ptr = self.thread_ptrs[self.selected_thread]
        thread_registers = dict()

        # A switched out thread resumes at the lr it called
        # z_arm_context_switch() with, with r4-r11 and sp as saved there.
        size_t_size = self.elffile.get_kernel_thread_info_size_t_size()
        callee = self.get_memory(thread_ptr + self.callee_saved_offset, size_t_size * 10)

        if callee is not None:
            regs = struct.unpack("<" + "I" * 10, callee)

            for i in range(8):
                thread_registers[RegNum.R4 + i] = regs[i]
            thread_registers[RegNum.SP] = regs[8]
            thread_registers[RegNum.LR] = regs[9]
            thread_registers[RegNum.PC] = regs[9]
            # z_arm_context_switch() always runs in system mode, in ARM
            # state with IRQs masked.
            thread_registers[RegNum.CPSR] = self.CPSR_SWITCHED_OUT
        else:
            t_stack_ptr_offset = self.elffile.get_kernel_thread_info_offset(
                ThreadInfoOffset.THREAD_INFO_OFFSET_T_STACK_PTR
            )
            stack_ptr = self.get_memory(thread_ptr + t_stack_ptr_offset, size_t_size)
            if stack_ptr is not None:
                thread_registers[RegNum.SP] = int.from_bytes(stack_ptr, "little")

        self.send_registers_packet(thread_registers)
