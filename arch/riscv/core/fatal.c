/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <kernel_structs.h>
#include <inttypes.h>
#include <logging/log.h>
LOG_MODULE_DECLARE(os);

FUNC_NORETURN void z_riscv_fatal_error(unsigned int reason,
				       const z_arch_esf_t *esf)
{
	if (esf != NULL) {
		LOG_ERR("Faulting instruction address = 0x%08lx",
			esf->mepc);
		LOG_ERR("  ra: 0x%08lx  gp: 0x%08lx  tp: 0x%08lx  t0: 0x%08lx",
			esf->ra, esf->gp, esf->tp, esf->t0);
		LOG_ERR("  t1: 0x%08lx  t2: 0x%08lx  t3: 0x%08lx  t4: 0x%08lx",
			esf->t1, esf->t2, esf->t3, esf->t4);
		LOG_ERR("  t5: 0x%08lx  t6: 0x%08lx  a0: 0x%08lx  a1: 0x%08lx",
			esf->t5, esf->t6, esf->a0, esf->a1);
		LOG_ERR("  a2: 0x%08lx  a3: 0x%08lx  a4: 0x%08lx  a5: 0x%08lx",
			esf->a2, esf->a3, esf->a4, esf->a5);
		LOG_ERR("  a6: 0x%08lx  a7: 0x%08lx\n",
			esf->a6, esf->a7);
	}

	z_fatal_error(reason, esf);
	CODE_UNREACHABLE;
}

static char *cause_str(ulong_t cause)
{
	switch (cause) {
	case 0:
		return "Instruction address misaligned";
	case 1:
		return "Instruction Access fault";
	case 2:
		return "Illegal instruction";
	case 3:
		return "Breakpoint";
	case 4:
		return "Load address misaligned";
	case 5:
		return "Load access fault";
	default:
		return "unknown";
	}
}

FUNC_NORETURN void _Fault(const z_arch_esf_t *esf)
{
	ulong_t mcause;

	__asm__ volatile("csrr %0, mcause" : "=r" (mcause));

	mcause &= SOC_MCAUSE_EXP_MASK;
	LOG_ERR("Exception cause %s (%ld)", cause_str(mcause), mcause);

	z_riscv_fatal_error(K_ERR_CPU_EXCEPTION, esf);
}
