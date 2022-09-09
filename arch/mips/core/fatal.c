/*
 * Copyright (c) 2020 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

FUNC_NORETURN void z_mips_fatal_error(unsigned int reason,
					  const z_arch_esf_t *esf)
{
	if (esf != NULL) {
		printk("$ 0   :         (ze) %08lx(at) %08lx(v0) %08lx(v1)\n",
			esf->at, esf->v0, esf->v1);
		printk("$ 4   : %08lx(a0) %08lx(a1) %08lx(a2) %08lx(a3)\n",
			esf->a0, esf->a1, esf->a2, esf->a3);
		printk("$ 8   : %08lx(t0) %08lx(t1) %08lx(t2) %08lx(t3)\n",
			esf->t0, esf->t1, esf->t2, esf->t3);
		printk("$12   : %08lx(t4) %08lx(t5) %08lx(t6) %08lx(t7)\n",
			esf->t4, esf->t5, esf->t6, esf->t7);
		printk("...\n");
		printk("$24   : %08lx(t8) %08lx(t9)\n",
			esf->t8, esf->t9);
		printk("$28   : %08lx(gp)         (sp)         (s8) %08lx(ra)\n",
			esf->gp, esf->ra);

		printk("EPC   : %08lx\n", esf->epc);

		printk("Status: %08lx\n", esf->status);
		printk("Cause : %08lx\n", esf->cause);
		printk("BadVA : %08lx\n", esf->badvaddr);
	}

	z_fatal_error(reason, esf);
	CODE_UNREACHABLE;
}

static char *cause_str(unsigned long cause)
{
	switch (cause) {
	case 0:
		return "interrupt pending";
	case 1:
		return "TLB modified";
	case 2:
		return "TLB miss on load or ifetch";
	case 3:
		return "TLB miss on store";
	case 4:
		return "address error on load or ifetch";
	case 5:
		return "address error on store";
	case 6:
		return "bus error on ifetch";
	case 7:
		return "bus error on load or store";
	case 8:
		return "system call";
	case 9:
		return "breakpoint";
	case 10:
		return "reserved instruction";
	case 11:
		return "coprocessor unusable";
	case 12:
		return "arithmetic overflow";
	case 13:
		return "trap instruction";
	case 14:
		return "virtual coherency instruction";
	case 15:
		return "floating point";
	case 16:
		return "iwatch";
	case 23:
		return "dwatch";
	case 31:
		return "virtual coherency data";
	default:
		return "unknown";
	}
}

void _Fault(z_arch_esf_t *esf)
{
	unsigned long cause;

	cause = (read_c0_cause() & CAUSE_EXP_MASK) >> CAUSE_EXP_SHIFT;

	LOG_ERR("");
	LOG_ERR(" cause: %ld, %s", cause, cause_str(cause));

	z_mips_fatal_error(K_ERR_CPU_EXCEPTION, esf);
}
