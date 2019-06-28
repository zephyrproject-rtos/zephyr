/*
 * Copyright (c) 2019 Intel Corp.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_X86_MSR_H_
#define ZEPHYR_INCLUDE_ARCH_X86_MSR_H_

/*
 * Model specific registers (MSR).  Access with z_x86_msr_read/write().
 */

#define X86_SPEC_CTRL_MSR		0x0048
#define X86_SPEC_CTRL_MSR_IBRS		BIT(0)
#define X86_SPEC_CTRL_MSR_SSBD		BIT(2)

#define X86_APIC_BASE_MSR		0x001b
#define X86_APIC_BASE_MSR_X2APIC	BIT(10)

#define X86_MTRR_DEF_TYPE_MSR		0x02ff
#define X86_MTRR_DEF_TYPE_MSR_ENABLE	BIT(11)

#define X86_X2APIC_BASE_MSR		0x0800	/* 0x0800-0x0BFF -> x2APIC */

#ifndef _ASMLANGUAGE
#ifdef __cplusplus
extern "C" {
#endif

/*
 * z_x86_msr_write() is shared between 32- and 64-bit implementations, but
 * due to ABI differences with long return values, z_x86_msr_read() is not.
 */

static inline void z_x86_msr_write(unsigned int msr, u64_t data)
{
	u32_t high = data >> 32;
	u32_t low = data & 0xFFFFFFFF;

	__asm__ volatile ("wrmsr" : : "c"(msr), "a"(low), "d"(high));
}

#ifndef CONFIG_X86_LONGMODE

static inline u64_t z_x86_msr_read(unsigned int msr)
{
	u64_t ret;

	__asm__ volatile("rdmsr" : "=A" (ret) : "c" (msr));

	return ret;
}

#endif

#ifdef __cplusplus
}
#endif
#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_X86_MSR_H_ */
