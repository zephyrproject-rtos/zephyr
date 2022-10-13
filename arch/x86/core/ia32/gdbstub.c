/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <kernel_internal.h>
#include <ia32/exception.h>
#include <inttypes.h>
#include <zephyr/debug/gdbstub.h>


static struct gdb_ctx ctx;

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

	z_gdb_main_loop(&ctx);

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

size_t arch_gdb_reg_readall(struct gdb_ctx *ctx, uint8_t *buf, size_t buflen)
{
	size_t ret;

	if (buflen < (sizeof(ctx->registers) * 2)) {
		ret = 0;
	} else {
		ret = bin2hex((const uint8_t *)&(ctx->registers),
			      sizeof(ctx->registers), buf, buflen);
	}

	return ret;
}

size_t arch_gdb_reg_writeall(struct gdb_ctx *ctx, uint8_t *hex, size_t hexlen)
{
	size_t ret;

	if (hexlen != (sizeof(ctx->registers) * 2)) {
		ret = 0;
	} else {
		ret = hex2bin(hex, hexlen,
			      (uint8_t *)&(ctx->registers),
			      sizeof(ctx->registers));
	}

	return ret;
}

size_t arch_gdb_reg_readone(struct gdb_ctx *ctx, uint8_t *buf, size_t buflen,
			    uint32_t regno)
{
	size_t ret;

	if (buflen < (sizeof(unsigned int) * 2)) {
		/* Make sure there is enough space to write hex string */
		ret = 0;
	} else if (regno >= GDB_STUB_NUM_REGISTERS) {
		/* Return hex string "xx" to tell GDB that this register
		 * is not available. So GDB will continue probing other
		 * registers instead of stopping in the middle of
		 * "info registers all".
		 */
		if (buflen >= 2) {
			memcpy(buf, "xx", 2);
			ret = 2;
		} else {
			ret = 0;
		}
	} else {
		ret = bin2hex((const uint8_t *)&(ctx->registers[regno]),
			      sizeof(ctx->registers[regno]),
			      buf, buflen);
	}

	return ret;
}

size_t arch_gdb_reg_writeone(struct gdb_ctx *ctx, uint8_t *hex, size_t hexlen,
			     uint32_t regno)
{
	size_t ret;

	if (regno == GDB_ORIG_EAX) {
		/* GDB requires orig_eax that seems to be
		 * Linux specific. Unfortunately if we just
		 * return error, GDB will stop working.
		 * So just fake an OK response by saying
		 * that we have processed the hex string.
		 */
		ret = hexlen;
	} else if (regno >= GDB_STUB_NUM_REGISTERS) {
		ret = 0;
	} else if (hexlen != (sizeof(unsigned int) * 2)) {
		/* Make sure the input hex string matches register size */
		ret = 0;
	} else {
		ret = hex2bin(hex, hexlen,
			      (uint8_t *)&(ctx->registers[regno]),
			      sizeof(ctx->registers[regno]));
	}

	return ret;
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
	__asm__ volatile ("int3");
}

/* Hook current IDT. */
_EXCEPTION_CONNECT_NOCODE(z_gdb_debug_isr, IV_DEBUG, 3);
_EXCEPTION_CONNECT_NOCODE(z_gdb_break_isr, IV_BREAKPOINT, 3);
