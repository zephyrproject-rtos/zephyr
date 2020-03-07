/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_AARCH64_CPU_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_AARCH64_CPU_H_

#include <sys/util.h>

#define DAIFSET_FIQ		BIT(0)
#define DAIFSET_IRQ		BIT(1)
#define DAIFSET_ABT		BIT(2)
#define DAIFSET_DBG		BIT(3)

#define DAIF_FIQ		BIT(6)
#define DAIF_IRQ		BIT(7)
#define DAIF_ABT		BIT(8)
#define DAIF_DBG		BIT(9)
#define DAIF_MASK		(0xf << 6)

#define SPSR_MODE_EL1H		(0x5)

#define SCTLR_M_BIT		BIT(0)
#define SCTLR_A_BIT		BIT(1)
#define SCTLR_C_BIT		BIT(2)
#define SCTLR_SA_BIT		BIT(3)
#define SCTLR_I_BIT		BIT(12)

#define CPACR_EL1_FPEN_NOTRAP	(0x3 << 20)

#define SCR_EL3_NS		BIT(0)
#define SCR_EL3_IRQ		BIT(1)
#define SCR_EL3_FIQ		BIT(2)
#define SCR_EL3_EA		BIT(3)
#define SCR_EL3_RW		BIT(10)

#define HCR_EL2_FMO		BIT(3)
#define HCR_EL2_IMO		BIT(4)
#define HCR_EL2_AMO		BIT(5)

#define SPSR_EL3_h		BIT(0)
#define SPSR_EL3_TO_EL1		(0x2 << 1)

#define __ISB()			__asm__ volatile ("isb sy" : : : "memory")
#define __DMB()			__asm__ volatile ("dmb sy" : : : "memory")

#define MODE_EL_SHIFT		(0x2)
#define MODE_EL_MASK		(0x3)

#define MODE_EL3		(0x3)
#define MODE_EL2		(0x2)
#define MODE_EL1		(0x1)
#define MODE_EL0		(0x0)

#define GET_EL(_mode)		(((_mode) >> MODE_EL_SHIFT) & MODE_EL_MASK)

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_AARCH64_CPU_H_ */
