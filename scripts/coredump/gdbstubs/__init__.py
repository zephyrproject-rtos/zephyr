#!/usr/bin/env python3
#
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

from gdbstubs.arch.x86 import GdbStub_x86
from gdbstubs.arch.x86_64 import GdbStub_x86_64
from gdbstubs.arch.arm_cortex_m import GdbStub_ARM_CortexM
from gdbstubs.arch.risc_v import GdbStub_RISC_V
from gdbstubs.arch.xtensa import GdbStub_Xtensa
from gdbstubs.arch.arm64 import GdbStub_ARM64

class TgtCode:
    UNKNOWN = 0
    X86 = 1
    X86_64 = 2
    ARM_CORTEX_M = 3
    RISC_V = 4
    XTENSA = 5
    ARM64 = 6

def get_gdbstub(logfile, elffile):
    stub = None

    tgt_code = logfile.log_hdr['tgt_code']

    if tgt_code == TgtCode.X86:
        stub = GdbStub_x86(logfile=logfile, elffile=elffile)
    elif tgt_code == TgtCode.X86_64:
        stub = GdbStub_x86_64(logfile=logfile, elffile=elffile)
    elif tgt_code == TgtCode.ARM_CORTEX_M:
        stub = GdbStub_ARM_CortexM(logfile=logfile, elffile=elffile)
    elif tgt_code == TgtCode.RISC_V:
        stub = GdbStub_RISC_V(logfile=logfile, elffile=elffile)
    elif tgt_code == TgtCode.XTENSA:
        stub = GdbStub_Xtensa(logfile=logfile, elffile=elffile)
    elif tgt_code == TgtCode.ARM64:
        stub = GdbStub_ARM64(logfile=logfile, elffile=elffile)

    return stub
