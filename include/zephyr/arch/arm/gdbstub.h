/*
 * Copyright (c) 2023 Marek Vedral <vedrama5@fel.cvut.cz>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_GDBSTUB_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_GDBSTUB_H_

#include <zephyr/arch/arm/exc.h>

#ifndef _ASMLANGUAGE

#define DBGDSCR_MONITOR_MODE_EN 0x8000

#define SPSR_ISETSTATE_ARM     0x0
#define SPSR_ISETSTATE_JAZELLE 0x2
#define SPSR_J                 24
#define SPSR_T                 5

/* Debug Breakpoint Control Register constants */
#define DBGDBCR_MEANING_MASK          0x7
#define DBGDBCR_MEANING_SHIFT         20
#define DBGDBCR_MEANING_ADDR_MISMATCH 0x4
#define DBGDBCR_BYTE_ADDR_MASK        0xF
#define DBGDBCR_BYTE_ADDR_SHIFT       5
#define DBGDBCR_BRK_EN_MASK           0x1

/* Regno of the SPSR */
#define SPSR_REG_IDX    25
/* Minimal size of the packet - SPSR is the last, 42-nd byte, see packet_pos array */
#define GDB_READALL_PACKET_SIZE (42 * 8)

#define IFSR_DEBUG_EVENT 0x2

enum AARCH32_GDB_REG {
	R0 = 0,
	R1,
	R2,
	R3,
	/* READONLY registers (R4 - R13) except R12 */
	R4,
	R5,
	R6,
	R7,
	R8,
	R9,
	R10,
	R11,
	R12,
	/* Stack pointer - READONLY */
	R13,
	LR,
	PC,
	/* Saved program status register */
	SPSR,
	GDB_NUM_REGS
};

/* required structure */
struct gdb_ctx {
	/* cause of the exception */
	unsigned int exception;
	unsigned int registers[GDB_NUM_REGS];
};

void z_gdb_entry(z_arch_esf_t *esf, unsigned int exc_cause);

#endif

#endif
