/*
 * Copyright (c) 2019-2020 Cobham Gaisler AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

/*
 * EXAMPLE OUTPUT
 *
 * ---------------------------------------------------------------------
 *
 *  tt = 0x02, illegal_instruction
 *
 *        INS        LOCALS     OUTS       GLOBALS
 *    0:  00000000   f3900fc0   40007c50   00000000
 *    1:  00000000   40004bf0   40008d30   40008c00
 *    2:  00000000   40004bf4   40008000   00000003
 *    3:  40009158   00000000   40009000   00000002
 *    4:  40008fa8   40003c00   40008fa8   00000008
 *    5:  40009000   f3400fc0   00000000   00000080
 *    6:  4000a1f8   40000050   4000a190   00000000
 *    7:  40002308   00000000   40001fb8   000000c1
 *
 *  psr: f30000c7   wim: 00000008   tbr: 40000020   y: 00000000
 *   pc: 4000a1f4   npc: 4000a1f8
 *
 *        pc         sp
 *   #0   4000a1f4   4000a190
 *   #1   40002308   4000a1f8
 *   #2   40003b24   4000a258
 *
 * ---------------------------------------------------------------------
 *
 *
 * INTERPRETATION
 *
 * INS, LOCALS, OUTS and GLOBALS represent the %i, %l, %o and %g
 * registers before the trap was taken.
 *
 * wim, y, pc and npc are the values before the trap was taken.
 * tbr has the tbr.tt field (bits 11..4) filled in by hardware
 * representing the current trap type. psr is read immediately
 * after the trap was taken so it will have the new CWP and ET=0.
 *
 * The "#i pc sp" rows is the stack backtrace. All register
 * windows are flushed to the stack prior to printing. First row
 * is the trapping pc and sp (o6).
 *
 *
 * HOW TO USE
 *
 * When investigating a crashed program, the first things to look
 * at is typically the tt, pc and sp (o6). You can lookup the pc
 * in the assembly list file or use addr2line. In the listing, the
 * register values in the table above can be used. The linker map
 * file will give a hint on which stack is active and if it has
 * overflowed.
 *
 * psr bits 11..8 is the processor interrupt (priority) level. 0
 * is lowest priority level (all can be taken), and 0xf is the
 * highest level where only non-maskable interrupts are taken.
 *
 * g0 is always zero. g5, g6 are never accessed by the compiler.
 * g7 is the TLS pointer if enabled. A SAVE instruction decreases
 * the current window pointer (psr bits 4..0) which results in %o
 * registers becoming %i registers and a new set of %l registers
 * appear. RESTORE does the opposite.
 */


/*
 * The SPARC V8 ABI guarantees that the stack pointer register
 * (o6) points to an area organized as "struct savearea" below at
 * all times when traps are enabled. This is the register save
 * area where register window registers can be flushed to the
 * stack.
 *
 * We flushed registers to this space in the fault trap entry
 * handler. Note that the space is allocated by the ABI (compiler)
 * for each stack frame.
 *
 * When printing the registers, we get the "local" and "in"
 * registers from the ABI stack save area, while the "out" and
 * "global" registers are taken from the exception stack frame
 * generated in the fault trap entry.
 */
struct savearea {
	uint32_t local[8];
	uint32_t in[8];
};

#if CONFIG_EXCEPTION_DEBUG
/*
 * Exception trap type (tt) values according to The SPARC V8
 * manual, Table 7-1.
 */
static const struct {
	int tt;
	const char *desc;
} TTDESC[] = {
	{ .tt = 0x02, .desc = "illegal_instruction", },
	{ .tt = 0x07, .desc = "mem_address_not_aligned", },
	{ .tt = 0x2B, .desc = "data_store_error", },
	{ .tt = 0x29, .desc = "data_access_error", },
	{ .tt = 0x09, .desc = "data_access_exception", },
	{ .tt = 0x21, .desc = "instruction_access_error", },
	{ .tt = 0x01, .desc = "instruction_access_exception", },
	{ .tt = 0x04, .desc = "fp_disabled", },
	{ .tt = 0x08, .desc = "fp_exception", },
	{ .tt = 0x2A, .desc = "division_by_zero", },
	{ .tt = 0x03, .desc = "privileged_instruction", },
	{ .tt = 0x20, .desc = "r_register_access_error", },
	{ .tt = 0x0B, .desc = "watchpoint_detected", },
	{ .tt = 0x2C, .desc = "data_access_MMU_miss", },
	{ .tt = 0x3C, .desc = "instruction_access_MMU_miss", },
	{ .tt = 0x05, .desc = "window_overflow", },
	{ .tt = 0x06, .desc = "window_underflow", },
	{ .tt = 0x0A, .desc = "tag_overflow", },
};

static void print_trap_type(const z_arch_esf_t *esf)
{
	const int tt = (esf->tbr & TBR_TT) >> TBR_TT_BIT;
	const char *desc = "unknown";

	if (tt & 0x80) {
		desc = "trap_instruction";
	} else if (tt >= 0x11 && tt <= 0x1F) {
		desc = "interrupt";
	} else {
		for (int i = 0; i < ARRAY_SIZE(TTDESC); i++) {
			if (TTDESC[i].tt == tt) {
				desc = TTDESC[i].desc;
				break;
			}
		}
	}
	LOG_ERR("tt = 0x%02X, %s", tt, desc);
}

static void print_integer_registers(const z_arch_esf_t *esf)
{
	const struct savearea *flushed = (struct savearea *) esf->out[6];

	LOG_ERR("      INS        LOCALS     OUTS       GLOBALS");
	for (int i = 0; i < 8; i++) {
		LOG_ERR(
			"  %d:  %08x   %08x   %08x   %08x",
			i,
			flushed ? flushed->in[i] : 0,
			flushed ? flushed->local[i] : 0,
			esf->out[i],
			esf->global[i]
		);
	}
}

static void print_special_registers(const z_arch_esf_t *esf)
{
	LOG_ERR(
		"psr: %08x   wim: %08x   tbr: %08x   y: %08x",
		esf->psr, esf->wim, esf->tbr, esf->y
	);
	LOG_ERR(" pc: %08x   npc: %08x", esf->pc, esf->npc);
}

static void print_backtrace(const z_arch_esf_t *esf)
{
	const int MAX_LOGLINES = 40;
	const struct savearea *s = (struct savearea *) esf->out[6];

	LOG_ERR("      pc         sp");
	LOG_ERR(" #0   %08x   %08x", esf->pc, (unsigned int) s);
	for (int i = 1; s && i < MAX_LOGLINES; i++) {
		const uint32_t pc = s->in[7];
		const uint32_t sp = s->in[6];

		if (sp == 0U && pc == 0U) {
			break;
		}
		LOG_ERR(" #%-2d  %08x   %08x", i, pc, sp);
		if (sp == 0U || sp & 7U) {
			break;
		}
		s = (const struct savearea *) sp;
	}
}

static void print_all(const z_arch_esf_t *esf)
{
	LOG_ERR("");
	print_trap_type(esf);
	LOG_ERR("");
	print_integer_registers(esf);
	LOG_ERR("");
	print_special_registers(esf);
	LOG_ERR("");
	print_backtrace(esf);
	LOG_ERR("");
}
#endif /* CONFIG_EXCEPTION_DEBUG */

FUNC_NORETURN void z_sparc_fatal_error(unsigned int reason,
				       const z_arch_esf_t *esf)
{
#if CONFIG_EXCEPTION_DEBUG
	if (esf != NULL) {
		if (IS_ENABLED(CONFIG_EXTRA_EXCEPTION_INFO)) {
			print_all(esf);
		} else {
			print_special_registers(esf);
		}
	}
#endif /* CONFIG_EXCEPTION_DEBUG */

	z_fatal_error(reason, esf);
	CODE_UNREACHABLE;
}
