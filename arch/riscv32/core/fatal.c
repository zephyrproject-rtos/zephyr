/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <kernel_structs.h>
#include <inttypes.h>
#include <sys/printk.h>
#include <logging/log_ctrl.h>

const NANO_ESF _default_esf = {
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
#if defined(CONFIG_RISCV_SOC_CONTEXT_SAVE)
	{
		SOC_ESF_INIT,
	},
#endif
};

FUNC_NORETURN void z_riscv32_fatal_error(unsigned int reason,
					 const NANO_ESF *esf)
{
	printk("Faulting instruction address = 0x%x\n"
	       "  ra: 0x%x  gp: 0x%x  tp: 0x%x  t0: 0x%x\n"
	       "  t1: 0x%x  t2: 0x%x  t3: 0x%x  t4: 0x%x\n"
	       "  t5: 0x%x  t6: 0x%x  a0: 0x%x  a1: 0x%x\n"
	       "  a2: 0x%x  a3: 0x%x  a4: 0x%x  a5: 0x%x\n"
	       "  a6: 0x%x  a7: 0x%x\n",
	       (esf->mepc == 0xdeadbaad) ? 0xdeadbaad : esf->mepc,
	       esf->ra, esf->gp, esf->tp, esf->t0,
	       esf->t1, esf->t2, esf->t3, esf->t4,
	       esf->t5, esf->t6, esf->a0, esf->a1,
	       esf->a2, esf->a3, esf->a4, esf->a5,
	       esf->a6, esf->a7);

	z_fatal_error(reason, esf);
	CODE_UNREACHABLE;
}

static char *cause_str(u32_t cause)
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

FUNC_NORETURN void _Fault(const NANO_ESF *esf)
{
	u32_t mcause;

	__asm__ volatile("csrr %0, mcause" : "=r" (mcause));

	mcause &= SOC_MCAUSE_EXP_MASK;
	printk("Exception cause %s (%d)\n", cause_str(mcause), (int)mcause);

	z_riscv32_fatal_error(K_ERR_CPU_EXCEPTION, esf);
}
