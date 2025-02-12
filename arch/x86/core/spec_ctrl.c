/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <kernel_arch_data.h>
#include <kernel_arch_func.h>
#include <zephyr/arch/x86/msr.h>
#include <zephyr/arch/x86/cpuid.h>

/*
 * See:
 * https://software.intel.com/security-software-guidance/api-app/sites/default/files/336996-Speculative-Execution-Side-Channel-Mitigations.pdf
 */

#if defined(CONFIG_X86_DISABLE_SSBD) || defined(CONFIG_X86_ENABLE_EXTENDED_IBRS)
static int spec_ctrl_init(void)
{

	uint32_t enable_bits = 0U;
	uint32_t cpuid7 = z_x86_cpuid_extended_features();

#ifdef CONFIG_X86_DISABLE_SSBD
	if ((cpuid7 & CPUID_SPEC_CTRL_SSBD) != 0U) {
		enable_bits |= X86_SPEC_CTRL_MSR_SSBD;
	}
#endif
#ifdef CONFIG_X86_ENABLE_EXTENDED_IBRS
	if ((cpuid7 & CPUID_SPEC_CTRL_IBRS) != 0U) {
		enable_bits |= X86_SPEC_CTRL_MSR_IBRS;
	}
#endif
	if (enable_bits != 0U) {
		uint64_t cur = z_x86_msr_read(X86_SPEC_CTRL_MSR);

		z_x86_msr_write(X86_SPEC_CTRL_MSR,
			       cur | enable_bits);
	}

	return 0;
}

SYS_INIT(spec_ctrl_init, PRE_KERNEL_1, 0);
#endif /* CONFIG_X86_DISABLE_SSBD || CONFIG_X86_ENABLE_EXTENDED_IBRS */
