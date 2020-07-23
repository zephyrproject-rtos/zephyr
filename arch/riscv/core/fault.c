/*
 * Copyright (c) 2020 RISE Research Institutes of Sweden <www.ri.se>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <kernel_internal.h>
#include <kernel_structs.h>
#include <exc_handle.h>
#include <logging/log.h>
LOG_MODULE_DECLARE(os);

#ifdef CONFIG_USERSPACE
Z_EXC_DECLARE(z_riscv_user_string_nlen);
Z_EXC_DECLARE(z_riscv_is_user_context);

static const struct z_exc_handle exceptions[] = {
	Z_EXC_HANDLE(z_riscv_user_string_nlen),
	Z_EXC_HANDLE(z_riscv_is_user_context)
};

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
		return "Store address misaligned";
	case 7:
		return "Store access fault";
	default:
		return "unknown";
	}
}
#endif /* CONFIG USERSPACE */

void _Fault(z_arch_esf_t *esf)
{

#ifdef CONFIG_USERSPACE
	/*
	 * Check if the fault has an exception handler, as defined using the
	 * macros found in include/exc_handle.h.
	 */
	for (int i = 0; i < ARRAY_SIZE(exceptions); i++) {
		uint32_t start = (uint32_t)exceptions[i].start;
		uint32_t end = (uint32_t)exceptions[i].end;

		if (esf->mepc >= start && esf->mepc < end) {
			esf->mepc = (uint32_t)exceptions[i].fixup;
			return;
		}
	}

	ulong_t mcause, mstatus;

	__asm__ volatile("csrr %0, mcause" : "=r" (mcause));
	__asm__ volatile("csrr %0, mstatus" : "=r" (mstatus));

	/*
	 * mstatus[12:11] is the MPP (M-mode previous privilege) field on RV32.
	 * After a trap, MPP = 0x0 if the trap was triggered in U-mode or 0x3
	 * if the trap was triggered in M-mode.
	 */

	mstatus &= MSTATUS_MPP_M;
	if (!mstatus) {

		LOG_ERR("U-mode thread aborted: %s (%ld)",
				cause_str(mcause), mcause);

		k_thread_abort(k_current_get());
		CODE_UNREACHABLE;

	} else {

		mcause &= SOC_MCAUSE_EXP_MASK;
		LOG_ERR("Exception cause %s (%ld)",
				cause_str(mcause), mcause);

		z_riscv_fatal_error(K_ERR_CPU_EXCEPTION, esf);

	}

#else
	z_riscv_fatal_error(K_ERR_CPU_EXCEPTION, esf);
#endif /* !CONFIG_USERSPACE */
}
