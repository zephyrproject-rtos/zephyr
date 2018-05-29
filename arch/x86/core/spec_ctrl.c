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
#include <kernel.h>

#define CPUID_EXTENDED_FEATURES_LVL 7

#define CPUID_SPEC_CTRL BIT(26)
#define SPEC_CTRL_SSBD BIT(2)

static int
cpu_has_spec_ctrl(void)
{
	u32_t eax, ebx, ecx = 0, edx;

	if (!__get_cpuid(CPUID_EXTENDED_FEATURES_LVL,
			 &eax, &ebx, &ecx, &edx)) {
		return 0;
	}

	ARG_UNUSED(eax);
	ARG_UNUSED(ebx);
	ARG_UNUSED(ecx);

	return edx & CPUID_SPEC_CTRL;
}

static int
disable_ssbd_if_needed(struct device *dev)
{
	/* This is checked in runtime rather than compile time since
	 * IA32_SPEC_CTRL_MSR might be added in a microcode update.
	 */
	if (cpu_has_spec_ctrl()) {
		u64_t cur = _x86_msr_read(IA32_SPEC_CTRL_MSR);

		_x86_msr_write(IA32_SPEC_CTRL_MSR,
			       cur | SPEC_CTRL_SSBD);
	}

	ARG_UNUSED(dev);

	return 0;
}

SYS_INIT(disable_ssbd_if_needed, PRE_KERNEL_1, 0);
