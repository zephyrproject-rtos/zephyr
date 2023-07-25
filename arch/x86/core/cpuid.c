/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cpuid.h> /* Header provided by the toolchain. */

#include <zephyr/kernel_structs.h>
#include <zephyr/arch/x86/cpuid.h>
#include <zephyr/kernel.h>

uint32_t z_x86_cpuid_extended_features(void)
{
	uint32_t eax, ebx, ecx = 0U, edx;

	if (__get_cpuid(CPUID_EXTENDED_FEATURES_LVL,
			&eax, &ebx, &ecx, &edx) == 0) {
		return 0;
	}

	return edx;
}

#define INITIAL_APIC_ID_SHIFT (24)
#define INITIAL_APIC_ID_MASK (0xFF)

uint8_t z_x86_cpuid_get_current_physical_apic_id(void)
{
	uint32_t eax, ebx, ecx, edx;

	if (IS_ENABLED(CONFIG_X2APIC)) {
		/* leaf 0x1F should be used first prior to using 0x0B */
		if (__get_cpuid(CPUID_EXTENDED_TOPOLOGY_ENUMERATION_V2,
				&eax, &ebx, &ecx, &edx) == 0) {
			if (__get_cpuid(CPUID_EXTENDED_TOPOLOGY_ENUMERATION,
					&eax, &ebx, &ecx, &edx) == 0) {
				return 0;
			}
		}
	} else {
		if (__get_cpuid(CPUID_BASIC_INFO_1,
				&eax, &ebx, &ecx, &edx) == 0) {
			return 0;
		}

		edx = (ebx >> INITIAL_APIC_ID_SHIFT);
	}

	return (uint8_t)(edx & INITIAL_APIC_ID_MASK);
}
