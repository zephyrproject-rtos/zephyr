/*
 * Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM64_LIB_HELPERS_H_
#define ZEPHYR_INCLUDE_ARCH_ARM64_LIB_HELPERS_H_

#ifndef _ASMLANGUAGE

#include <zephyr/arch/arm64/cpu.h>
#include <stdint.h>

/* All the macros need a memory clobber */

#define read_sysreg(reg)						\
({									\
	uint64_t reg_val;						\
	__asm__ volatile ("mrs %0, " STRINGIFY(reg)			\
			  : "=r" (reg_val) :: "memory");		\
	reg_val;							\
})

#define write_sysreg(val, reg)						\
({									\
	__asm__ volatile ("msr " STRINGIFY(reg) ", %0"			\
			  :: "r" (val) : "memory");			\
})

#define zero_sysreg(reg)						\
({									\
	__asm__ volatile ("msr " STRINGIFY(reg) ", xzr"			\
			  ::: "memory");				\
})

#define MAKE_REG_HELPER(reg)						\
	static ALWAYS_INLINE uint64_t read_##reg(void)			\
	{								\
		return read_sysreg(reg);				\
	}								\
	static ALWAYS_INLINE void write_##reg(uint64_t val)		\
	{								\
		write_sysreg(val, reg);					\
	}								\
	static ALWAYS_INLINE void zero_##reg(void)			\
	{								\
		zero_sysreg(reg);					\
	}

#define MAKE_REG_HELPER_EL123(reg) \
	MAKE_REG_HELPER(reg##_el1) \
	MAKE_REG_HELPER(reg##_el2) \
	MAKE_REG_HELPER(reg##_el3)

MAKE_REG_HELPER(ccsidr_el1);
MAKE_REG_HELPER(clidr_el1);
MAKE_REG_HELPER(cntfrq_el0);
MAKE_REG_HELPER(cnthctl_el2);
MAKE_REG_HELPER(cnthp_ctl_el2);
MAKE_REG_HELPER(cnthps_ctl_el2);
MAKE_REG_HELPER(cntv_ctl_el0)
MAKE_REG_HELPER(cntv_cval_el0)
MAKE_REG_HELPER(cntvct_el0);
MAKE_REG_HELPER(cntvoff_el2);
MAKE_REG_HELPER(currentel);
MAKE_REG_HELPER(csselr_el1);
MAKE_REG_HELPER(daif)
MAKE_REG_HELPER(hcr_el2);
MAKE_REG_HELPER(id_aa64pfr0_el1);
MAKE_REG_HELPER(id_aa64mmfr0_el1);
MAKE_REG_HELPER(mpidr_el1);
MAKE_REG_HELPER(par_el1);
#if !defined(CONFIG_ARMV8_R)
MAKE_REG_HELPER(scr_el3);
#endif /* CONFIG_ARMV8_R */
MAKE_REG_HELPER(tpidrro_el0);
MAKE_REG_HELPER(vmpidr_el2);
MAKE_REG_HELPER(sp_el0);

MAKE_REG_HELPER_EL123(actlr)
MAKE_REG_HELPER_EL123(cpacr)
MAKE_REG_HELPER_EL123(cptr)
MAKE_REG_HELPER_EL123(elr)
MAKE_REG_HELPER_EL123(esr)
MAKE_REG_HELPER_EL123(far)
MAKE_REG_HELPER_EL123(mair)
MAKE_REG_HELPER_EL123(sctlr)
MAKE_REG_HELPER_EL123(spsr)
MAKE_REG_HELPER_EL123(tcr)
MAKE_REG_HELPER_EL123(ttbr0)
MAKE_REG_HELPER_EL123(vbar)

#if defined(CONFIG_ARM_MPU)
/* Armv8-R aarch64 mpu registers */
#define mpuir_el1	S3_0_c0_c0_4
#define prselr_el1	S3_0_c6_c2_1
#define prbar_el1	S3_0_c6_c8_0
#define prlar_el1	S3_0_c6_c8_1

MAKE_REG_HELPER(mpuir_el1);
MAKE_REG_HELPER(prselr_el1);
MAKE_REG_HELPER(prbar_el1);
MAKE_REG_HELPER(prlar_el1);
#endif

static ALWAYS_INLINE void enable_debug_exceptions(void)
{
	__asm__ volatile ("msr DAIFClr, %0"
			  :: "i" (DAIFCLR_DBG_BIT) : "memory");
}

static ALWAYS_INLINE void disable_debug_exceptions(void)
{
	__asm__ volatile ("msr DAIFSet, %0"
			  :: "i" (DAIFSET_DBG_BIT) : "memory");
}

static ALWAYS_INLINE void enable_serror_exceptions(void)
{
	__asm__ volatile ("msr DAIFClr, %0"
			  :: "i" (DAIFCLR_ABT_BIT) : "memory");
}

static ALWAYS_INLINE void disable_serror_exceptions(void)
{
	__asm__ volatile ("msr DAIFSet, %0"
			  :: "i" (DAIFSET_ABT_BIT) : "memory");
}

static ALWAYS_INLINE void enable_irq(void)
{
	__asm__ volatile ("msr DAIFClr, %0"
			  :: "i" (DAIFCLR_IRQ_BIT) : "memory");
}

static ALWAYS_INLINE void disable_irq(void)
{
	__asm__ volatile ("msr DAIFSet, %0"
			  :: "i" (DAIFSET_IRQ_BIT) : "memory");
}

static ALWAYS_INLINE void enable_fiq(void)
{
	__asm__ volatile ("msr DAIFClr, %0"
			  :: "i" (DAIFCLR_FIQ_BIT) : "memory");
}

static ALWAYS_INLINE void disable_fiq(void)
{
	__asm__ volatile ("msr DAIFSet, %0"
			  :: "i" (DAIFSET_FIQ_BIT) : "memory");
}

#define sev()	__asm__ volatile("sev" : : : "memory")
#define wfe()	__asm__ volatile("wfe" : : : "memory")
#define wfi()	__asm__ volatile("wfi" : : : "memory")

static inline bool is_el_implemented(unsigned int el)
{
	unsigned int shift;

	if (el > 3) {
		return false;
	}

	shift = ID_AA64PFR0_EL1_SHIFT * el;

	return (((read_id_aa64pfr0_el1() >> shift) & ID_AA64PFR0_ELX_MASK) != 0U);
}

static inline bool is_el_highest_implemented(void)
{
	uint32_t el_highest;
	uint32_t curr_el;

	el_highest = read_id_aa64pfr0_el1() & 0xFFFF;
	el_highest = (31U - __builtin_clz(el_highest)) / 4;

	curr_el = GET_EL(read_currentel());

	if (curr_el < el_highest)
		return false;

	return true;
}

static inline bool is_el2_sec_supported(void)
{
	return (((read_id_aa64pfr0_el1() >> ID_AA64PFR0_SEL2_SHIFT) &
		ID_AA64PFR0_SEL2_MASK) != 0U);
}

static inline bool is_in_secure_state(void)
{
	/* We cannot read SCR_EL3 from EL2 or EL1 */
	return !IS_ENABLED(CONFIG_ARMV8_A_NS);
}

#endif /* !_ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM64_LIB_HELPERS_H_ */
