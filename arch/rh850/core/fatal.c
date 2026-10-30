/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/fatal.h>
#include <zephyr/logging/log.h>
#include <kernel_arch_func.h>

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

/* The high 16 bits of EIIC/FEIC hold a detail code */
#define RH850_CAUSE(code) ((code) & 0xffffU)

static const char *rh850_cause_str(uint32_t cause)
{
	uint32_t c = RH850_CAUSE(cause);

	if (c >= 0x10U && c <= 0x1fU) {
		return "SYSERR";
	}
	if (c >= 0x30U && c <= 0x3fU) {
		return "FETRAP";
	}
	if (c >= 0x40U && c <= 0x4fU) {
		return "TRAP0";
	}
	if (c >= 0x50U && c <= 0x5fU) {
		return "TRAP1";
	}
	if (c >= 0x80U && c <= 0x82U) {
		return "UCPOP";
	}
	if (c >= 0xf0U && c <= 0xffU) {
		return "FEINT";
	}

	switch (c) {
	case 0x60U:
		return "RIE";
	case 0x71U:
		return "FPE";
	case 0x72U:
		return "FPI";
	case 0x90U:
		return "MIP";
	case 0x91U:
	case 0x95U:
		return "MDP";
	case 0xa0U:
		return "PIE";
	case 0xc0U:
		return "MAE";
	case 0xe0U:
		return "FENMI";
	default:
		return "unknown";
	}
}

static void rh850_fatal_error(unsigned int reason, const struct arch_esf *esf)
{
#ifdef CONFIG_EXCEPTION_DEBUG
	if (esf != NULL) {
		uint32_t sp = POINTER_TO_UINT(esf) + sizeof(*esf);

		EXCEPTION_DUMP(" r1: %08x  r2: %08x  sp: %08x  r4: %08x", esf->r1, esf->r2, sp,
			       esf->r4);
		EXCEPTION_DUMP(" r5: %08x  r6: %08x  r7: %08x  r8: %08x", esf->r5, esf->r6, esf->r7,
			       esf->r8);
		EXCEPTION_DUMP(" r9: %08x r10: %08x r11: %08x r12: %08x", esf->r9, esf->r10,
			       esf->r11, esf->r12);
		EXCEPTION_DUMP("r13: %08x r14: %08x r15: %08x r16: %08x", esf->r13, esf->r14,
			       esf->r15, esf->r16);
		EXCEPTION_DUMP("r17: %08x r18: %08x r19: %08x r20: %08x", esf->r17, esf->r18,
			       esf->r19, esf->r20);
		EXCEPTION_DUMP("r21: %08x r22: %08x r23: %08x r24: %08x", esf->r21, esf->r22,
			       esf->r23, esf->r24);
		EXCEPTION_DUMP("r25: %08x r26: %08x r27: %08x r28: %08x", esf->r25, esf->r26,
			       esf->r27, esf->r28);
		EXCEPTION_DUMP("r29: %08x  ep: %08x  lp: %08x", esf->r29, esf->r30, esf->r31);
		EXCEPTION_DUMP(" pc: %08x psw: %08x fpsr: %08x fpepc: %08x", esf->eipc, esf->eipsw,
			       esf->fpsr, esf->fpepc);
		EXCEPTION_DUMP("ctpc: %08x ctpsw: %08x", esf->ctpc, esf->ctpsw);
	}
#endif /* CONFIG_EXCEPTION_DEBUG */

	z_fatal_error(reason, esf);

	if (arch_is_in_nested_exception(esf) && reason != K_ERR_STACK_CHK_FAIL) {
		k_fatal_halt(reason);
	}
}

void z_rh850_fault(struct arch_esf *esf, uint32_t cause, uint32_t level)
{
	bool fe = level == RH850_EXC_LEVEL_FE;

	EXCEPTION_DUMP("%s level exception: %s, %s %08x", fe ? "FE" : "EI", rh850_cause_str(cause),
		       fe ? "FEIC" : "EIIC", cause);

	if (RH850_CAUSE(cause) == 0xc0U || RH850_CAUSE(cause) == 0x91U) {
		EXCEPTION_DUMP("MEA %08x MEI %08x", rh850_sysreg_read(RH850_SR_MEA),
			       rh850_sysreg_read(RH850_SR_MEI));
	}

	rh850_fatal_error(K_ERR_CPU_EXCEPTION, esf);
}

void z_rh850_except(unsigned int reason, struct arch_esf *esf)
{
	rh850_fatal_error(reason, esf);
}
