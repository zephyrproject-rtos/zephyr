#!/usr/bin/env python3
#
# Copyright (c) 2022 Huawei Technologies SASU
#
# SPDX-License-Identifier: Apache-2.0

import binascii
import logging
import struct

from coredump_parser.elf_parser import ThreadInfoOffset

from gdbstubs.gdbstub import GdbStub

logger = logging.getLogger("gdbstub")


class RegNum:
    X0 = 0  # X0-X29 - 30 GP registers
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
    X29 = 29  # Frame pointer register
    LR = 30  # X30 Link Register(LR)
    SP_EL0 = 31  # Stack pointer EL0 (SP_EL0)
    PC = 32  # Program Counter (PC)


class GdbStub_ARM64(GdbStub):
    # v1: x0-x18, lr, spsr, elr  (22 regs, 176 bytes)
    ARCH_DATA_BLK_STRUCT_V1 = "<" + ("Q" * 22)
    # v2: v1 + fp, sp            (24 regs, 192 bytes)
    ARCH_DATA_BLK_STRUCT_V2 = "<" + ("Q" * 24)

    GDB_SIGNAL_DEFAULT = 7
    GDB_G_PKT_NUM_REGS = 33

    # struct _callee_saved (include/zephyr/arch/arm64/thread.h): 14 uint64_t
    # fields -- x19..x29, sp_el0, sp_elx, lr -- saved directly into the
    # k_thread struct by z_arm64_context_switch() (arch/arm64/core/switch.S)
    # for any thread that isn't currently running. Not individually exposed
    # via the generic thread-info offsets table (only sp_elx is, as
    # THREAD_INFO_OFFSET_T_STACK_PTR), so the struct base is derived from
    # that single known offset instead.
    CALLEE_SAVED_STRUCT = "<" + ("Q" * 14)
    CALLEE_SAVED_SIZE = struct.calcsize(CALLEE_SAVED_STRUCT)
    CALLEE_SAVED_SP_ELX_INDEX = 12  # 0-based index of sp_elx within the struct

    def __init__(self, logfile, elffile):
        super().__init__(logfile=logfile, elffile=elffile)
        self.registers = None
        self.gdb_signal = self.GDB_SIGNAL_DEFAULT

        self.registers = self._decode_arch_block(self.logfile.get_arch_data()['data'])

    @classmethod
    def _decode_arch_block(cls, arch_data_blk):
        """Decode one arm64_arch_block-layout payload (x0-x18, lr, spsr,
        elr, optionally fp/sp) into a {RegNum: value} dict. Shared by the
        panicking thread's own arch block and by any live per-CPU snapshot
        blocks (CONFIG_DEBUG_COREDUMP_SMP_FREEZE_CPUS) -- both use the same
        on-wire layout."""

        block_len = len(arch_data_blk)

        has_fp_sp = block_len == struct.calcsize(cls.ARCH_DATA_BLK_STRUCT_V2)

        if has_fp_sp:
            tu = struct.unpack(cls.ARCH_DATA_BLK_STRUCT_V2, arch_data_blk)
        else:
            tu = struct.unpack(cls.ARCH_DATA_BLK_STRUCT_V1, arch_data_blk)

        registers = dict()

        registers[RegNum.X0] = tu[0]
        registers[RegNum.X1] = tu[1]
        registers[RegNum.X2] = tu[2]
        registers[RegNum.X3] = tu[3]
        registers[RegNum.X4] = tu[4]
        registers[RegNum.X5] = tu[5]
        registers[RegNum.X6] = tu[6]
        registers[RegNum.X7] = tu[7]
        registers[RegNum.X8] = tu[8]
        registers[RegNum.X9] = tu[9]
        registers[RegNum.X10] = tu[10]
        registers[RegNum.X11] = tu[11]
        registers[RegNum.X12] = tu[12]
        registers[RegNum.X13] = tu[13]
        registers[RegNum.X14] = tu[14]
        registers[RegNum.X15] = tu[15]
        registers[RegNum.X16] = tu[16]
        registers[RegNum.X17] = tu[17]
        registers[RegNum.X18] = tu[18]

        registers[RegNum.LR] = tu[19]
        # tu[20] is SPSR - not a GDB GP register, skip it
        registers[RegNum.PC] = tu[21]  # ELR = faulting/live PC

        if has_fp_sp:
            registers[RegNum.X29] = tu[22]  # FP
            registers[RegNum.SP_EL0] = tu[23]  # SP
            logger.debug(
                "LR=0x%016x PC=0x%016x FP=0x%016x SP=0x%016x", tu[19], tu[21], tu[22], tu[23]
            )
        else:
            logger.debug("LR=0x%016x PC=0x%016x (no FP/SP)", tu[19], tu[21])

        return registers

    def send_registers_packet(self, registers):
        reg_fmt = "<Q"

        idx = 0
        pkt = b''

        while idx < self.GDB_G_PKT_NUM_REGS:
            if idx in registers:
                bval = struct.pack(reg_fmt, registers[idx])
                pkt += binascii.hexlify(bval)
            else:
                # Register not in coredump -> unknown value
                # Send in "xxxxxxxx"
                pkt += b'x' * 16

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
        self.put_gdb_packet(b'x' * 16)

    def arch_supports_thread_operations(self):
        return True

    def handle_thread_register_group_read_packet(self):
        # For selected_thread 0, use the register data retrieved from the
        # dump's arch section (the faulting/current thread's real ESF).
        if self.selected_thread == 0:
            self.send_registers_packet(self.registers)
            return

        thread_ptr = self.thread_ptrs[self.selected_thread]

        # If this thread was actively running on another CPU when the
        # system was frozen for the dump (CONFIG_DEBUG_COREDUMP_SMP_FREEZE_CPUS),
        # a live snapshot exists with its *exact* register state (real PC,
        # SP, x0-x18, x19-x29) -- prefer it over the callee_saved-derived
        # approximation below, which is only ever as fresh as that thread's
        # last voluntary context switch and would otherwise be stale here.
        for snapshot in self.logfile.get_cpu_snapshots():
            # A zero-length entry is the panicking-CPU marker (see
            # arch_coredump_cpu_snapshot_dump() in arch/arm64/core/coredump.c)
            # -- it records a thread pointer but carries no register payload,
            # since selected_thread == 0 above already covers that thread
            # using the real ESF-based data.
            if snapshot["thread_ptr"] == thread_ptr and len(snapshot["data"]) > 0:
                logger.debug(
                    "Using live CPU snapshot (cpu=%d) for thread 0x%x",
                    snapshot["cpu_id"],
                    thread_ptr,
                )
                self.send_registers_packet(self._decode_arch_block(snapshot["data"]))
                return

        # THREAD_INFO_OFFSET_T_STACK_PTR is offsetof(k_thread, callee_saved.sp_elx)
        # on ARM64 (see subsys/debug/thread_info.c) -- back out the base of
        # the callee_saved struct from it so the rest of its fields can be
        # read too.
        t_stack_ptr_offset = self.elffile.get_kernel_thread_info_offset(
            ThreadInfoOffset.THREAD_INFO_OFFSET_T_STACK_PTR
        )
        callee_saved_offset = t_stack_ptr_offset - (self.CALLEE_SAVED_SP_ELX_INDEX * 8)

        barray = self.get_memory(thread_ptr + callee_saved_offset, self.CALLEE_SAVED_SIZE)

        thread_registers = dict()

        if barray is not None:
            tu = struct.unpack(self.CALLEE_SAVED_STRUCT, barray)

            thread_registers[RegNum.X19] = tu[0]
            thread_registers[RegNum.X20] = tu[1]
            thread_registers[RegNum.X21] = tu[2]
            thread_registers[RegNum.X22] = tu[3]
            thread_registers[RegNum.X23] = tu[4]
            thread_registers[RegNum.X24] = tu[5]
            thread_registers[RegNum.X25] = tu[6]
            thread_registers[RegNum.X26] = tu[7]
            thread_registers[RegNum.X27] = tu[8]
            thread_registers[RegNum.X28] = tu[9]
            thread_registers[RegNum.X29] = tu[10]
            # tu[11] is sp_el0 (EL0/userspace stack, unused by kernel-only
            # threads) -- not mapped to a GDB register slot.
            sp_elx = tu[12]
            lr = tu[13]

            thread_registers[RegNum.SP_EL0] = sp_elx  # SP to unwind from
            thread_registers[RegNum.LR] = lr
            # A non-running thread resumes via `ret` inside
            # z_arm64_context_switch(), i.e. execution continues at the
            # saved LR -- use it as PC so GDB can unwind from where the
            # thread will resume.
            thread_registers[RegNum.PC] = lr

        self.send_registers_packet(thread_registers)
