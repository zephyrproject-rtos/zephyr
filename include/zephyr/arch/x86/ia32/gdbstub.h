/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief IA-32 specific gdbstub interface header
 */

#ifndef ZEPHYR_INCLUDE_ARCH_X86_GDBSTUB_SYS_H_
#define ZEPHYR_INCLUDE_ARCH_X86_GDBSTUB_SYS_H_

#ifndef _ASMLANGUAGE

#include <stdint.h>
#include <zephyr/toolchain.h>

/**
 * @brief Number of register used by gdbstub in IA-32
 */
#define GDB_STUB_NUM_REGISTERS 16

/**
 * @brief GDB interruption context
 *
 * The exception stack frame contents used by gdbstub. The contents
 * of this struct are used to display information about the current
 * cpu state.
 */

struct gdb_interrupt_ctx {
	uint32_t ss;
	uint32_t gs;
	uint32_t fs;
	uint32_t es;
	uint32_t ds;
	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t esp;
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;
	uint32_t vector;
	uint32_t error_code;
	uint32_t eip;
	uint32_t cs;
	uint32_t eflags;
} __packed;

/**
 * @brief IA-32 register used in gdbstub
 */

enum GDB_REGISTER {
	GDB_EAX,
	GDB_ECX,
	GDB_EDX,
	GDB_EBX,
	GDB_ESP,
	GDB_EBP,
	GDB_ESI,
	GDB_EDI,
	GDB_PC,
	GDB_EFLAGS,
	GDB_CS,
	GDB_SS,
	GDB_DS,
	GDB_ES,
	GDB_FS,
	GDB_GS,
	GDB_ORIG_EAX = 41,
};

struct gdb_ctx {
	unsigned int exception;
	unsigned int registers[GDB_STUB_NUM_REGISTERS];
};

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_X86_GDBSTUB_SYS_H_ */
