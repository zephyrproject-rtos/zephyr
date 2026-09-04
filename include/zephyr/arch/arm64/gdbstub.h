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

#define GDB_AARCH64_X0_REGNO		0
#define GDB_AARCH64_SP_REGNO		31
#define GDB_AARCH64_PC_REGNO		32
#define GDB_AARCH64_CPSR_REGNO		33
#define GDB_AARCH64_V0_REGNO		34
#define GDB_AARCH64_FPSR_REGNO		66
#define GDB_AARCH64_FPCR_REGNO		67
#define GDB_AARCH64_NUM_GDB_REGS	68

/* Core GP registers we actually cache */
enum aarch64_gdb_reg {
	GDB_X0 = 0,
	GDB_X1,
	GDB_X2,
	GDB_X3,
	GDB_X4,
	GDB_X5,
	GDB_X6,
	GDB_X7,
	GDB_X8,
	GDB_X9,
	GDB_X10,
	GDB_X11,
	GDB_X12,
	GDB_X13,
	GDB_X14,
	GDB_X15,
	GDB_X16,
	GDB_X17,
	GDB_X18,
	GDB_X19,
	GDB_X20,
	GDB_X21,
	GDB_X22,
	GDB_X23,
	GDB_X24,
	GDB_X25,
	GDB_X26,
	GDB_X27,
	GDB_X28,
	GDB_X29,
	GDB_X30,
	GDB_SP,
	GDB_PC,
	GDB_CPSR,
	GDB_NUM_REGS
};

/*
 * Binary size of a full GDB g-packet for aarch64 default layout:
 *  31*8 (Xn) + 8 (SP) + 8 (PC) + 4 (CPSR) + 32*16 (Vn) + 4 (FPSR) + 4 (FPCR)
 *  = 788 bytes -> 1576 hex characters
 */
#define GDB_AARCH64_G_PACKET_BYTES	788
#define GDB_AARCH64_G_PACKET_HEXLEN	(GDB_AARCH64_G_PACKET_BYTES * 2)

/* Force BRK instruction encoding used by arch_gdb_init */
#define GDB_AARCH64_BRK_INSTR		0xD4200000U

struct gdb_ctx {
	unsigned int exception;
	uint64_t registers[GDB_NUM_REGS];
};

/*
 * GPRs not saved in arch_esf (x19–x29). Captured by the exception asm path
 * before entering C gdbstub so GDB can read/write the interrupted context.
 * Layout: x19,x20,x21,x22,x23,x24,x25,x26,x27,x28,x29.
 */
struct arm64_gdb_esf_extra_regs {
	uint64_t x19;
	uint64_t x20;
	uint64_t x21;
	uint64_t x22;
	uint64_t x23;
	uint64_t x24;
	uint64_t x25;
	uint64_t x26;
	uint64_t x27;
	uint64_t x28;
	uint64_t x29;
};

extern struct arm64_gdb_esf_extra_regs gdb_esf_extra_regs;

void z_gdb_entry(struct arch_esf *esf, unsigned int cause);

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM64_GDBSTUB_H_ */
