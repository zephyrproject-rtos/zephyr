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

#if __riscv_xlen == 32
 #define PR_REG "%08" PRIxPTR
 #define NO_REG "        "
#elif __riscv_xlen == 64
 #define PR_REG "%016" PRIxPTR
 #define NO_REG "                "
#endif

FUNC_NORETURN void z_riscv_fatal_error(unsigned int reason,
				       const z_arch_esf_t *esf)
{
	if (esf != NULL) {
		LOG_ERR("     a0: " PR_REG "    t0: " PR_REG, esf->a0, esf->t0);
		LOG_ERR("     a1: " PR_REG "    t1: " PR_REG, esf->a1, esf->t1);
		LOG_ERR("     a2: " PR_REG "    t2: " PR_REG, esf->a2, esf->t2);
		LOG_ERR("     a3: " PR_REG "    t3: " PR_REG, esf->a3, esf->t3);
		LOG_ERR("     a4: " PR_REG "    t4: " PR_REG, esf->a4, esf->t4);
		LOG_ERR("     a5: " PR_REG "    t5: " PR_REG, esf->a5, esf->t5);
		LOG_ERR("     a6: " PR_REG "    t6: " PR_REG, esf->a6, esf->t6);
		LOG_ERR("     a7: " PR_REG, esf->a7);
		LOG_ERR("         " NO_REG "    tp: " PR_REG, esf->tp);
		LOG_ERR("     ra: " PR_REG "    gp: " PR_REG, esf->ra, esf->gp);
		LOG_ERR("   mepc: " PR_REG, esf->mepc);
		LOG_ERR("mstatus: " PR_REG, esf->mstatus);
		LOG_ERR("");
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
	case 6:
		return "Store/AMO address misaligned";
	case 7:
		return "Store/AMO access fault";
	case 8:
		return "Environment call from U-mode";
	case 9:
		return "Environment call from S-mode";
	case 11:
		return "Environment call from M-mode";
	case 12:
		return "Instruction page fault";
	case 13:
		return "Load page fault";
	case 15:
		return "Store/AMO page fault";
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

#ifndef CONFIG_SOC_OPENISA_RV32M1_RISCV32
	ulong_t mtval;
	__asm__ volatile("csrr %0, mtval" : "=r" (mtval));
#endif

	mcause &= SOC_MCAUSE_EXP_MASK;
	LOG_ERR("");
	LOG_ERR(" mcause: %ld, %s", mcause, cause_str(mcause));
#ifndef CONFIG_SOC_OPENISA_RV32M1_RISCV32
	LOG_ERR("  mtval: %lx", mtval);
#endif

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
