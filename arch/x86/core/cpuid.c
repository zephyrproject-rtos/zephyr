/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cpuid.h> /* Header provided by the toolchain. */

#include <kernel_structs.h>
#include <arch/x86/cpuid.h>
#include <kernel.h>

uint32_t z_x86_cpuid_extended_features(void)
{
	uint32_t eax, ebx, ecx = 0U, edx;

	if (__get_cpuid(CPUID_EXTENDED_FEATURES_LVL,
			&eax, &ebx, &ecx, &edx) == 0) {
		return 0;
	}

	return edx;
}
