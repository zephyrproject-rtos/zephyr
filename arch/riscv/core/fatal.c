/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <kernel_structs.h>
#include <inttypes.h>
#include <exc_handle.h>
#include <logging/log.h>
LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

#ifdef CONFIG_USERSPACE
Z_EXC_DECLARE(z_riscv_user_string_nlen);

static const struct z_exc_handle exceptions[] = {
	Z_EXC_HANDLE(z_riscv_user_string_nlen),
};
#endif /* CONFIG_USERSPACE */

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

void _Fault(z_arch_esf_t *esf)
{
#ifdef CONFIG_USERSPACE
	/*
	 * Perform an assessment whether an PMP fault shall be
	 * treated as recoverable.
	 */
	for (int i = 0; i < ARRAY_SIZE(exceptions); i++) {
		uint32_t start = (uint32_t)exceptions[i].start;
		uint32_t end = (uint32_t)exceptions[i].end;

		if (esf->mepc >= start && esf->mepc < end) {
			esf->mepc = (uint32_t)exceptions[i].fixup;
			return;
		}
	}
#endif /* CONFIG_USERSPACE */
	ulong_t mcause;

	__asm__ volatile("csrr %0, mcause" : "=r" (mcause));

	mcause &= SOC_MCAUSE_EXP_MASK;
	LOG_ERR("Exception cause %s (%ld)", cause_str(mcause), mcause);

	z_riscv_fatal_error(K_ERR_CPU_EXCEPTION, esf);
}

#ifdef CONFIG_USERSPACE
FUNC_NORETURN void arch_syscall_oops(void *ssf_ptr)
{
	user_fault(K_ERR_KERNEL_OOPS);
	CODE_UNREACHABLE;
}

void z_impl_user_fault(unsigned int reason)
{
	z_arch_esf_t *oops_esf = _current->syscall_frame;

	if (((_current->base.user_options & K_USER) != 0) &&
		reason != K_ERR_STACK_CHK_FAIL) {
		reason = K_ERR_KERNEL_OOPS;
	}
	z_riscv_fatal_error(reason, oops_esf);
}

static void z_vrfy_user_fault(unsigned int reason)
{
	z_impl_user_fault(reason);
}

#include <syscalls/user_fault_mrsh.c>

#endif /* CONFIG_USERSPACE */
