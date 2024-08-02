/*
 * Copyright (c) 2019 Intel Corp.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_X86_MSR_H_
#define ZEPHYR_INCLUDE_ARCH_X86_MSR_H_

/*
 * Model specific registers (MSR).  Access with z_x86_msr_read/write().
 */

#define X86_TIME_STAMP_COUNTER_MSR	0x00000010

#define X86_SPEC_CTRL_MSR		0x00000048
#define X86_SPEC_CTRL_MSR_IBRS		BIT(0)
#define X86_SPEC_CTRL_MSR_SSBD		BIT(2)

#define X86_APIC_BASE_MSR		0x0000001b
#define X86_APIC_BASE_MSR_X2APIC	BIT(10)

#define X86_MTRR_DEF_TYPE_MSR		0x000002ff
#define X86_MTRR_DEF_TYPE_MSR_ENABLE	BIT(11)

#define X86_X2APIC_BASE_MSR		0x00000800 /* .. thru 0x00000BFF */

#define X86_EFER_MSR			0xC0000080U
#define X86_EFER_MSR_SCE		BIT(0)
#define X86_EFER_MSR_LME		BIT(8)
#define X86_EFER_MSR_NXE		BIT(11)

/* STAR 31:0   Unused in long mode
 *      47:32  Kernel CS (SS = CS+8)
 *      63:48  User CS (SS = CS+8)
 */
#define X86_STAR_MSR			0xC0000081U

/* Location for system call entry point */
#define X86_LSTAR_MSR			0xC0000082U

/* Low 32 bits in this MSR are the SYSCALL mask applied to EFLAGS */
#define X86_FMASK_MSR			0xC0000084U

#define X86_FS_BASE			0xC0000100U
#define X86_GS_BASE			0xC0000101U
#define X86_KERNEL_GS_BASE		0xC0000102U

#ifndef _ASMLANGUAGE
#ifdef __cplusplus
extern "C" {
#endif

/*
 * z_x86_msr_write() is shared between 32- and 64-bit implementations, but
 * due to ABI differences with long return values, z_x86_msr_read() is not.
 */

static inline void z_x86_msr_write(unsigned int msr, uint64_t data)
{
	uint32_t high = data >> 32;
	uint32_t low = data & 0xFFFFFFFFU;

	__asm__ volatile ("wrmsr" : : "c"(msr), "a"(low), "d"(high));
}

#ifdef CONFIG_X86_64

static inline uint64_t z_x86_msr_read(unsigned int msr)
{
	union {
		struct {
			uint32_t lo;
			uint32_t hi;
		};
		uint64_t value;
	} rv;

	__asm__ volatile ("rdmsr" : "=a" (rv.lo), "=d" (rv.hi) : "c" (msr));

	return rv.value;
}

#else

static inline uint64_t z_x86_msr_read(unsigned int msr)
{
	uint64_t ret;

	__asm__ volatile("rdmsr" : "=A" (ret) : "c" (msr));

	return ret;
}

#endif

#ifdef __cplusplus
}
#endif
#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_X86_MSR_H_ */
