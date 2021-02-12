/*
 * Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_AARCH64_LIB_HELPERS_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_AARCH64_LIB_HELPERS_H_

#ifndef _ASMLANGUAGE

#include <arch/arm/aarch64/cpu.h>
#include <stdint.h>

/* All the macros need a memory clobber */

#define read_sysreg(reg)						\
({									\
	uint64_t val;							\
	__asm__ volatile ("mrs %0, " STRINGIFY(reg)			\
			  : "=r" (val) :: "memory");			\
	val;								\
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

MAKE_REG_HELPER(cntfrq_el0);
MAKE_REG_HELPER(cnthctl_el2);
MAKE_REG_HELPER(cnthp_ctl_el2);
MAKE_REG_HELPER(cntv_ctl_el0)
MAKE_REG_HELPER(cntv_cval_el0)
MAKE_REG_HELPER(cntvct_el0);
MAKE_REG_HELPER(cntvoff_el2);
MAKE_REG_HELPER(currentel);
MAKE_REG_HELPER(daif)
MAKE_REG_HELPER(hcr_el2);
MAKE_REG_HELPER(id_aa64pfr0_el1);
MAKE_REG_HELPER(scr_el3);

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

#define dsb()	__asm__ volatile ("dsb sy" ::: "memory")
#define dmb()	__asm__ volatile ("dmb sy" ::: "memory")
#define isb()	__asm__ volatile ("isb" ::: "memory")

/* Zephyr needs these as well */
#define __ISB() isb()
#define __DMB() dmb()
#define __DSB() dsb()

static inline bool is_el_implemented(unsigned int el)
{
	unsigned int shift;

	if (el > 3) {
		return false;
	}

	shift = ID_AA64PFR0_EL1_SHIFT * el;

	return (((read_id_aa64pfr0_el1() >> shift) & ID_AA64PFR0_ELX_MASK) != 0U);
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

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_AARCH64_LIB_HELPERS_H_ */
