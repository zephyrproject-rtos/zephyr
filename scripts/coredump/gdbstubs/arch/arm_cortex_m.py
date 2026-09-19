#!/usr/bin/env python3
#
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

import struct

from coredump_parser.elf_parser import ThreadInfoOffset

from gdbstubs.arch.arm_aarch32 import GdbStub_ARM_AArch32, RegNum


class GdbStub_ARM_CortexM(GdbStub_ARM_AArch32):
    def arch_supports_thread_operations(self):
        return True

    def handle_register_group_read_packet(self):
        if not self.elffile.has_kernel_thread_info():
            self.send_registers_packet(self.registers)
        else:
            self.handle_thread_register_group_read_packet()

    def handle_thread_register_group_read_packet(self):
        # For selected_thread 0, use the register data retrieved from the dump's arch section
        if self.selected_thread == 0:
            self.send_registers_packet(self.registers)
        else:
            thread_ptr = self.thread_ptrs[self.selected_thread]

            # Get stack pointer out of thread struct
            t_stack_ptr_offset = self.elffile.get_kernel_thread_info_offset(
                ThreadInfoOffset.THREAD_INFO_OFFSET_T_STACK_PTR
            )
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
                    t_arch_offset = self.elffile.get_kernel_thread_info_offset(
                        ThreadInfoOffset.THREAD_INFO_OFFSET_T_ARCH
                    )
                    t_exc_return_offset = self.elffile.get_kernel_thread_info_offset(
                        ThreadInfoOffset.THREAD_INFO_OFFSET_T_ARM_EXC_RETURN
                    )

                    # Value of 0xffffffff indicates THREAD_INFO_UNIMPLEMENTED
                    if t_exc_return_offset != 0xFFFFFFFF:
                        exc_return_bytes = self.get_memory(
                            thread_ptr + t_arch_offset + t_exc_return_offset, 1
                        )
                        exc_return = int.from_bytes(exc_return_bytes, "little")

                        # If the bit 4 is not set, the stack frame is extended for floating point
                        # data, adjust the SP accordingly
                        if (exc_return & (1 << 4)) == 0:
                            thread_registers[RegNum.SP] = thread_registers[RegNum.SP] + 72

                    # Set R7 to match the stack pointer in case the frame pointer is not omitted
                    thread_registers[RegNum.R7] = thread_registers[RegNum.SP]

                # Read callee-saved registers (r4-r11) from _callee_saved struct.
                if self.callee_saved_offset is not None:
                    callee_saved_bytes = self.get_memory(
                        thread_ptr + self.callee_saved_offset, size_t_size * 8
                    )
                    if callee_saved_bytes is not None:
                        callee_regs = struct.unpack("<IIIIIIII", callee_saved_bytes)
                        thread_registers[RegNum.R4] = callee_regs[0]
                        thread_registers[RegNum.R5] = callee_regs[1]
                        thread_registers[RegNum.R6] = callee_regs[2]
                        thread_registers[RegNum.R7] = callee_regs[3]
                        thread_registers[RegNum.R8] = callee_regs[4]
                        thread_registers[RegNum.R9] = callee_regs[5]
                        thread_registers[RegNum.R10] = callee_regs[6]
                        thread_registers[RegNum.R11] = callee_regs[7]

            self.send_registers_packet(thread_registers)
