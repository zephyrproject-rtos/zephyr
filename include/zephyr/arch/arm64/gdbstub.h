/*
 * Copyright (c) 2026 KylinSoft Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief AArch64 specific gdbstub interface header
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM64_GDBSTUB_H_
#define ZEPHYR_INCLUDE_ARCH_ARM64_GDBSTUB_H_

#include <zephyr/arch/exception.h>

#ifndef _ASMLANGUAGE

#include <stdint.h>

/*
 * GDB aarch64 register numbering (see GDB aarch64-tdep):
 *  0-30  : X0-X30
 *  31    : SP
 *  32    : PC
 *  33    : CPSR / PSTATE
 *  34-65 : V0-V31 (16 bytes each) - not implemented, reported as 'x'
 *  66    : FPSR
 *  67    : FPCR
 */

#define GDB_AARCH64_FP_REGNO     29
#define GDB_AARCH64_LR_REGNO     30
#define GDB_AARCH64_SP_REGNO     31
#define GDB_AARCH64_PC_REGNO     32
#define GDB_AARCH64_CPSR_REGNO   33
#define GDB_AARCH64_V0_REGNO     34
#define GDB_AARCH64_FPSR_REGNO   66
#define GDB_AARCH64_FPCR_REGNO   67
#define GDB_AARCH64_NUM_GDB_REGS 68

/*
 * Binary size of a full GDB g-packet for aarch64 default layout:
 *  31*8 (Xn) + 8 (SP) + 8 (PC) + 4 (CPSR) + 32*16 (Vn) + 4 (FPSR) + 4 (FPCR)
 *  = 788 bytes -> 1576 hex characters
 */
#define GDB_AARCH64_G_PACKET_BYTES  788
#define GDB_AARCH64_G_PACKET_HEXLEN (GDB_AARCH64_G_PACKET_BYTES * 2)

struct gdb_ctx {
	unsigned int exception;
	struct arch_esf *esf;
};

void z_gdb_entry(struct arch_esf *esf, unsigned int cause);

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM64_GDBSTUB_H_ */
