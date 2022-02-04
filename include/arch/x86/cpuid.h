/*
 * Copyright (c) 2022 Intel Corp.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_X86_CPUID_H_
#define ZEPHYR_INCLUDE_ARCH_X86_CPUID_H_

#ifndef _ASMLANGUAGE
#ifdef __cplusplus
extern "C" {
#endif

#define CPUID_BASIC_INFO_1			0x01
#define CPUID_EXTENDED_FEATURES_LVL		0x07
#define CPUID_EXTENDED_TOPOLOGY_ENUMERATION	0x0B
#define CPUID_EXTENDED_TOPOLOGY_ENUMERATION_V2	0x1F

/* Bits to check in CPUID extended features */
#define CPUID_SPEC_CTRL_SSBD	BIT(31)
#define CPUID_SPEC_CTRL_IBRS	BIT(26)

uint32_t z_x86_cpuid_extended_features(void);

uint8_t z_x86_cpuid_get_current_physical_apic_id(void);

#ifdef __cplusplus
}
#endif
#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_X86_CPUID_H_ */
