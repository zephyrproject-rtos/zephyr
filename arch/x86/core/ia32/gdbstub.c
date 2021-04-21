/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <kernel_internal.h>
#include <ia32/exception.h>
#include <inttypes.h>
#include <debug/gdbstub.h>


static struct gdb_ctx ctx;
static bool start;

/**
 * Currently we just handle vectors 1 and 3 but lets keep it generic
 * to be able to notify other exceptions in the future
 */
static unsigned int get_exception(unsigned int vector)
{
	unsigned int exception;

	switch (vector) {
	case IV_DIVIDE_ERROR:
		exception = GDB_EXCEPTION_DIVIDE_ERROR;
		break;
	case IV_DEBUG:
		exception = GDB_EXCEPTION_BREAKPOINT;
		break;
	case IV_BREAKPOINT:
		exception = GDB_EXCEPTION_BREAKPOINT;
		break;
	case IV_OVERFLOW:
		exception = GDB_EXCEPTION_OVERFLOW;
		break;
	case IV_BOUND_RANGE:
		exception = GDB_EXCEPTION_OVERFLOW;
		break;
	case IV_INVALID_OPCODE:
		exception = GDB_EXCEPTION_INVALID_INSTRUCTION;
		break;
	case IV_DEVICE_NOT_AVAILABLE:
		exception = GDB_EXCEPTION_DIVIDE_ERROR;
		break;
	case IV_DOUBLE_FAULT:
		exception = GDB_EXCEPTION_MEMORY_FAULT;
		break;
	case IV_COPROC_SEGMENT_OVERRUN:
		exception = GDB_EXCEPTION_INVALID_MEMORY;
		break;
	case IV_INVALID_TSS:
		exception = GDB_EXCEPTION_INVALID_MEMORY;
		break;
	case IV_SEGMENT_NOT_PRESENT:
		exception = GDB_EXCEPTION_INVALID_MEMORY;
		break;
	case IV_STACK_FAULT:
		exception = GDB_EXCEPTION_INVALID_MEMORY;
		break;
	case IV_GENERAL_PROTECTION:
		exception = GDB_EXCEPTION_INVALID_MEMORY;
		break;
	case IV_PAGE_FAULT:
		exception = GDB_EXCEPTION_INVALID_MEMORY;
		break;
	case IV_X87_FPU_FP_ERROR:
		exception = GDB_EXCEPTION_MEMORY_FAULT;
		break;
	default:
		exception = GDB_EXCEPTION_MEMORY_FAULT;
		break;
	}

	return exception;
}

/*
 * Debug exception handler.
 */
static void z_gdb_interrupt(unsigned int vector, z_arch_esf_t *esf)
{
	ctx.exception = get_exception(vector);

	ctx.registers[GDB_EAX] = esf->eax;
	ctx.registers[GDB_ECX] = esf->ecx;
	ctx.registers[GDB_EDX] = esf->edx;
	ctx.registers[GDB_EBX] = esf->ebx;
	ctx.registers[GDB_ESP] = esf->esp;
	ctx.registers[GDB_EBP] = esf->ebp;
	ctx.registers[GDB_ESI] = esf->esi;
	ctx.registers[GDB_EDI] = esf->edi;
	ctx.registers[GDB_PC] = esf->eip;
	ctx.registers[GDB_CS] = esf->cs;
	ctx.registers[GDB_EFLAGS]  = esf->eflags;
	ctx.registers[GDB_SS] = esf->ss;
	ctx.registers[GDB_DS] = esf->ds;
	ctx.registers[GDB_ES] = esf->es;
	ctx.registers[GDB_FS] = esf->fs;
	ctx.registers[GDB_GS] = esf->gs;

	z_gdb_main_loop(&ctx, start);
	start = false;

	esf->eax = ctx.registers[GDB_EAX];
	esf->ecx = ctx.registers[GDB_ECX];
	esf->edx = ctx.registers[GDB_EDX];
	esf->ebx = ctx.registers[GDB_EBX];
	esf->esp = ctx.registers[GDB_ESP];
	esf->ebp = ctx.registers[GDB_EBP];
	esf->esi = ctx.registers[GDB_ESI];
	esf->edi = ctx.registers[GDB_EDI];
	esf->eip = ctx.registers[GDB_PC];
	esf->cs = ctx.registers[GDB_CS];
	esf->eflags = ctx.registers[GDB_EFLAGS];
	esf->ss = ctx.registers[GDB_SS];
	esf->ds = ctx.registers[GDB_DS];
	esf->es = ctx.registers[GDB_ES];
	esf->fs = ctx.registers[GDB_FS];
	esf->gs = ctx.registers[GDB_GS];
}

void arch_gdb_continue(void)
{
	/* Clear the TRAP FLAG bit */
	ctx.registers[GDB_EFLAGS] &= ~BIT(8);
}

void arch_gdb_step(void)
{
	/* Set the TRAP FLAG bit */
	ctx.registers[GDB_EFLAGS] |= BIT(8);
}

static __used void z_gdb_debug_isr(z_arch_esf_t *esf)
{
	z_gdb_interrupt(IV_DEBUG, esf);
}

static __used void z_gdb_break_isr(z_arch_esf_t *esf)
{
	z_gdb_interrupt(IV_BREAKPOINT, esf);
}

void arch_gdb_init(void)
{
	start = true;
	__asm__ volatile ("int3");
}

/* Hook current IDT. */
_EXCEPTION_CONNECT_NOCODE(z_gdb_debug_isr, IV_DEBUG, 3);
_EXCEPTION_CONNECT_NOCODE(z_gdb_break_isr, IV_BREAKPOINT, 3);
