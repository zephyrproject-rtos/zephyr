/*
 * Copyright (c) 2022 IoT.bzh
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Armv8-R AArch32 architecture helpers.
 *
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_CORTEX_A_R_LIB_HELPERS_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_CORTEX_A_R_LIB_HELPERS_H_

#ifndef _ASMLANGUAGE

#include <stdint.h>

#define read_sysreg32(op1, CRn, CRm, op2)				\
({									\
	uint32_t val;							\
	__asm__ volatile ("mrc p15, " #op1 ", %0, c" #CRn ", c"		\
			  #CRm ", " #op2 : "=r" (val) :: "memory");	\
	val;								\
})

#define write_sysreg32(val, op1, CRn, CRm, op2)				\
({									\
	__asm__ volatile ("mcr p15, " #op1 ", %0, c" #CRn ", c"		\
			  #CRm ", " #op2 :: "r" (val) : "memory");	\
})

#define read_sysreg64(op1, CRm)						\
({									\
	uint64_t val;							\
	__asm__ volatile ("mrrc p15, " #op1 ", %Q0, %R0, c"		\
			  #CRm : "=r" (val) :: "memory");		\
	val;								\
})

#define write_sysreg64(val, op1, CRm)					\
({									\
	__asm__ volatile ("mcrr p15, " #op1 ", %Q0, %R0, c"		\
			  #CRm :: "r" (val) : "memory");		\
})

#define MAKE_REG_HELPER(reg, op1, CRn, CRm, op2)			\
	static ALWAYS_INLINE uint32_t read_##reg(void)			\
	{								\
		return read_sysreg32(op1, CRn, CRm, op2);		\
	}								\
	static ALWAYS_INLINE void write_##reg(uint32_t val)		\
	{								\
		write_sysreg32(val, op1, CRn, CRm, op2);		\
	}

#define MAKE_REG64_HELPER(reg, op1, CRm)				\
	static ALWAYS_INLINE uint64_t read_##reg(void)			\
	{								\
		return read_sysreg64(op1, CRm);				\
	}								\
	static ALWAYS_INLINE void write_##reg(uint64_t val)		\
	{								\
		write_sysreg64(val, op1, CRm);				\
	}

MAKE_REG_HELPER(mpuir,	     0, 0, 0, 4);
MAKE_REG_HELPER(mpidr,	     0, 0, 0, 5);
MAKE_REG_HELPER(sctlr,	     0, 1, 0, 0);
MAKE_REG_HELPER(prselr,	     0, 6, 2, 1);
MAKE_REG_HELPER(prbar,	     0, 6, 3, 0);
MAKE_REG_HELPER(prlar,	     0, 6, 3, 1);
MAKE_REG_HELPER(vbar,        0, 12, 0, 0);
MAKE_REG_HELPER(cntv_ctl,    0, 14,  3, 1);
MAKE_REG64_HELPER(ICC_SGI1R, 0, 12);
MAKE_REG64_HELPER(cntvct,    1, 14);
MAKE_REG64_HELPER(cntv_cval, 3, 14);

/*
 * GIC v3 compatibility macros:
 * ARMv8 AArch32 profile has no mention of
 * ELx in the register names.
 * We define them anyway to reuse the GICv3 driver
 * made for AArch64.
 */

/* ICC_PMR */
MAKE_REG_HELPER(ICC_PMR_EL1,     0, 4, 6, 0);
/* ICC_IAR1 */
MAKE_REG_HELPER(ICC_IAR1_EL1,    0, 12, 12, 0);
/* ICC_EOIR1 */
MAKE_REG_HELPER(ICC_EOIR1_EL1,   0, 12, 12, 1);
/* ICC_SRE */
MAKE_REG_HELPER(ICC_SRE_EL1,     0, 12, 12, 5);
/* ICC_IGRPEN1 */
MAKE_REG_HELPER(ICC_IGRPEN1_EL1, 0, 12, 12, 7);

#define write_sysreg(val, reg) write_##reg(val)
#define read_sysreg(reg) read_##reg()

#endif /* !_ASMLANGUAGE */
#endif /* ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_CORTEX_A_R_LIB_HELPERS_H_ */
