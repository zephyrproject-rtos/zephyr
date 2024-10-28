#!/usr/bin/env python3
#
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

import binascii
import logging
import struct

from coredump_parser.elf_parser import ThreadInfoOffset
from gdbstubs.gdbstub import GdbStub


logger = logging.getLogger("gdbstub")


class RegNum():
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


class GdbStub_ARM_CortexM(GdbStub):
    ARCH_DATA_BLK_STRUCT    = "<IIIIIIIII"
    ARCH_DATA_BLK_STRUCT_V2 = "<IIIIIIIIIIIIIIIII"

    GDB_SIGNAL_DEFAULT = 7

    GDB_G_PKT_NUM_REGS = 17

    def __init__(self, logfile, elffile):
        super().__init__(logfile=logfile, elffile=elffile)
        self.registers = None
        self.gdb_signal = self.GDB_SIGNAL_DEFAULT

        self.parse_arch_data_block()

    def parse_arch_data_block(self):
        arch_data_blk = self.logfile.get_arch_data()['data']
        arch_data_ver = self.logfile.get_arch_data()['hdr_ver']

        if arch_data_ver == 1:
            tu = struct.unpack(self.ARCH_DATA_BLK_STRUCT, arch_data_blk)
        elif arch_data_ver == 2:
            tu = struct.unpack(self.ARCH_DATA_BLK_STRUCT_V2, arch_data_blk)

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
            self.registers[RegNum.R4]  = tu[9]
            self.registers[RegNum.R5]  = tu[10]
            self.registers[RegNum.R6]  = tu[11]
            self.registers[RegNum.R7]  = tu[12]
            self.registers[RegNum.R8]  = tu[13]
            self.registers[RegNum.R9]  = tu[14]
            self.registers[RegNum.R10] = tu[15]
            self.registers[RegNum.R11] = tu[16]

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
        if not self.elffile.has_kernel_thread_info():
            self.send_registers_packet(self.registers)
        else:
            self.handle_thread_register_group_read_packet()

    def handle_register_single_read_packet(self, pkt):
        # Mark registers as "<unavailable>".
        # 'p' packets are usually used for registers
        # other than the general ones (e.g. eax, ebx)
        # so we can safely reply "xxxxxxxx" here.
        self.put_gdb_packet(b'x' * 8)

    def handle_register_single_write_packet(self, pkt):
        pkt_str = pkt.decode("ascii")
        reg = int(pkt_str[1:pkt_str.index('=')], 16)
        self.registers[reg] = int.from_bytes(binascii.unhexlify(pkt[3:]), byteorder = 'little')
        self.put_gdb_packet(b'+')

    def arch_supports_thread_operations(self):
        return True

    def handle_thread_register_group_read_packet(self):
        # For selected_thread 0, use the register data retrieved from the dump's arch section
        if self.selected_thread == 0:
            self.send_registers_packet(self.registers)
        else:
            thread_ptr = self.thread_ptrs[self.selected_thread]

            # Get stack pointer out of thread struct
            t_stack_ptr_offset = self.elffile.get_kernel_thread_info_offset(ThreadInfoOffset.THREAD_INFO_OFFSET_T_STACK_PTR)
            size_t_size = self.elffile.get_kernel_thread_info_size_t_size()
            stack_ptr_bytes = self.get_memory(thread_ptr + t_stack_ptr_offset, size_t_size)

            thread_registers = dict()

            if stack_ptr_bytes is not None:
                # Read registers stored at top of stack
                stack_ptr = int.from_bytes(stack_ptr_bytes, "little")
                barray = self.get_memory(stack_ptr, (size_t_size * 8))

                if barray is not None:
                    tu = struct.unpack("<IIIIIIII", barray)
                    thread_registers[RegNum.R0] = tu[0]
                    thread_registers[RegNum.R1] = tu[1]
                    thread_registers[RegNum.R2] = tu[2]
                    thread_registers[RegNum.R3] = tu[3]
                    thread_registers[RegNum.R12] = tu[4]
                    thread_registers[RegNum.LR] = tu[5]
                    thread_registers[RegNum.PC] = tu[6]
                    thread_registers[RegNum.XPSR] = tu[7]

                    # Set SP to point to stack just after these registers
                    thread_registers[RegNum.SP] = stack_ptr + 32

                    # Read the exc_return value from the thread's arch struct
                    t_arch_offset = self.elffile.get_kernel_thread_info_offset(ThreadInfoOffset.THREAD_INFO_OFFSET_T_ARCH)
                    t_exc_return_offset = self.elffile.get_kernel_thread_info_offset(ThreadInfoOffset.THREAD_INFO_OFFSET_T_ARM_EXC_RETURN)

                    # Value of 0xffffffff indicates THREAD_INFO_UNIMPLEMENTED
                    if t_exc_return_offset != 0xffffffff:
                        exc_return_bytes = self.get_memory(thread_ptr + t_arch_offset + t_exc_return_offset, 1)
                        exc_return = int.from_bytes(exc_return_bytes, "little")

                        # If the bit 4 is not set, the stack frame is extended for floating point data, adjust the SP accordingly
                        if (exc_return & (1 << 4)) == 0:
                            thread_registers[RegNum.SP] = thread_registers[RegNum.SP] + 72

                    # Set R7 to match the stack pointer in case the frame pointer is not omitted
                    thread_registers[RegNum.R7] = thread_registers[RegNum.SP]

            self.send_registers_packet(thread_registers)
