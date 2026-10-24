/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Hexagon architecture GDB stub definitions
 */

#ifndef ZEPHYR_INCLUDE_ARCH_HEXAGON_GDBSTUB_H_
#define ZEPHYR_INCLUDE_ARCH_HEXAGON_GDBSTUB_H_

/*
 * Hexagon GDB breakpoint instruction.
 *
 * trap0(#0xdb) is used as the software breakpoint instruction.  The encoding
 * 0x5400db0c places the immediate 0xdb in bits[15:8] of the instruction word.
 * The trap0 handler checks this immediate to distinguish GDB breakpoints from
 * syscall traps (trap0(#1)) and other uses.
 */
#define HEXAGON_BREAK_INSN 0x5400db0c

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>
#include <stdbool.h>

/* GDB register definitions for Hexagon */
#define GDB_HEXAGON_R0       0
#define GDB_HEXAGON_R1       1
#define GDB_HEXAGON_R2       2
#define GDB_HEXAGON_R3       3
#define GDB_HEXAGON_R4       4
#define GDB_HEXAGON_R5       5
#define GDB_HEXAGON_R6       6
#define GDB_HEXAGON_R7       7
#define GDB_HEXAGON_R8       8
#define GDB_HEXAGON_R9       9
#define GDB_HEXAGON_R10      10
#define GDB_HEXAGON_R11      11
#define GDB_HEXAGON_R12      12
#define GDB_HEXAGON_R13      13
#define GDB_HEXAGON_R14      14
#define GDB_HEXAGON_R15      15
#define GDB_HEXAGON_R16      16
#define GDB_HEXAGON_R17      17
#define GDB_HEXAGON_R18      18
#define GDB_HEXAGON_R19      19
#define GDB_HEXAGON_R20      20
#define GDB_HEXAGON_R21      21
#define GDB_HEXAGON_R22      22
#define GDB_HEXAGON_R23      23
#define GDB_HEXAGON_R24      24
#define GDB_HEXAGON_R25      25
#define GDB_HEXAGON_R26      26
#define GDB_HEXAGON_R27      27
#define GDB_HEXAGON_R28      28
#define GDB_HEXAGON_R29      29
#define GDB_HEXAGON_R30      30
#define GDB_HEXAGON_R31      31
#define GDB_HEXAGON_PC       32
#define GDB_HEXAGON_USR      33
#define GDB_HEXAGON_GP       34
#define GDB_HEXAGON_UGP      35
#define GDB_HEXAGON_LC0      36
#define GDB_HEXAGON_LC1      37
#define GDB_HEXAGON_SA0      38
#define GDB_HEXAGON_SA1      39
#define GDB_HEXAGON_P0       40
#define GDB_HEXAGON_P1       41
#define GDB_HEXAGON_P2       42
#define GDB_HEXAGON_P3       43
#define GDB_HEXAGON_NUM_REGS 44

/* Maximum number of breakpoints */
#ifndef GDB_MAX_BREAKPOINTS
#define GDB_MAX_BREAKPOINTS 4
#endif

/* GDB context structure - required by Zephyr's gdbstub subsystem */
struct gdb_ctx {
	unsigned int exception;              /* Exception reason */
	uint32_t regs[GDB_HEXAGON_NUM_REGS]; /* Register cache */
	bool stopped;                        /* Execution stopped flag */

	/* Breakpoint support */
	struct {
		uint32_t addr;
		uint32_t saved_insn;
		bool active;
	} breakpoints[GDB_MAX_BREAKPOINTS];
};

/*
 * Entry point from the exception/trap0 handlers (irq_manage.c) into the
 * debug stub.  ctx already holds the full register file: EVENT_ENTRY
 * (event_handlers.S) saves the callee-saved GPRs and GP/UGP into it
 * whenever CONFIG_GDBSTUB is enabled, on top of the volatile registers it
 * always saves -- see the field comment on struct event_context in
 * event_context.h for why that capture has to happen there and not here.
 */
struct event_context;
void z_hexagon_gdb_entry(struct event_context *ctx);

/**
 * @brief Check whether an instruction is the Hexagon GDB breakpoint.
 *
 * The address is checked against the debugger-readable memory map before it
 * is dereferenced, so fault handling can safely reject an invalid PC.
 *
 * @param addr Address of the candidate instruction.
 *
 * @retval true The instruction is a GDB breakpoint.
 * @retval false The address is invalid or contains another instruction.
 */
bool z_hexagon_is_gdb_breakpoint(uintptr_t addr);

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_HEXAGON_GDBSTUB_H_ */
