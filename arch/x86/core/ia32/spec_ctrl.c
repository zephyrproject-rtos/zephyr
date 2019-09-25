/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cpuid.h> /* Header provided by the toolchain. */

#include <init.h>
#include <kernel_structs.h>
#include <kernel_arch_data.h>
#include <kernel_arch_func.h>
#include <arch/x86/msr.h>
#include <kernel.h>

/*
 * See:
 * https://software.intel.com/security-software-guidance/api-app/sites/default/files/336996-Speculative-Execution-Side-Channel-Mitigations.pdf
 */

#define CPUID_EXTENDED_FEATURES_LVL 7

/* Bits to check in CPUID extended features */
#define CPUID_SPEC_CTRL_SSBD	BIT(31)
#define CPUID_SPEC_CTRL_IBRS	BIT(26)

#if defined(CONFIG_DISABLE_SSBD) || defined(CONFIG_ENABLE_EXTENDED_IBRS)
static u32_t cpuid_extended_features(void)
{
	u32_t eax, ebx, ecx = 0U, edx;

	if (__get_cpuid(CPUID_EXTENDED_FEATURES_LVL,
			&eax, &ebx, &ecx, &edx) == 0) {
		return 0;
	}

	return edx;
}

static int spec_ctrl_init(struct device *dev)
{
	ARG_UNUSED(dev);

	u32_t enable_bits = 0U;
	u32_t cpuid7 = cpuid_extended_features();

#ifdef CONFIG_DISABLE_SSBD
	if ((cpuid7 & CPUID_SPEC_CTRL_SSBD) != 0U) {
		enable_bits |= X86_SPEC_CTRL_MSR_SSBD;
	}
#endif
#ifdef CONFIG_ENABLE_EXTENDED_IBRS
	if ((cpuid7 & CPUID_SPEC_CTRL_IBRS) != 0U) {
		enable_bits |= X86_SPEC_CTRL_MSR_IBRS;
	}
#endif
	if (enable_bits != 0U) {
		u64_t cur = z_x86_msr_read(X86_SPEC_CTRL_MSR);

		z_x86_msr_write(X86_SPEC_CTRL_MSR,
			       cur | enable_bits);
	}

	return 0;
}

SYS_INIT(spec_ctrl_init, PRE_KERNEL_1, 0);
#endif /* CONFIG_DISABLE_SSBD || CONFIG_ENABLE_EXTENDED_IBRS */
