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

#define SCTLR_EL3_RES1		(BIT(29) | BIT(28) | BIT(23) | \
				BIT(22) | BIT(18) | BIT(16) | \
				BIT(11) | BIT(5) | BIT(4))

#define SCTLR_EL1_RES1		(BIT(29) | BIT(28) | BIT(23) | \
				BIT(22) | BIT(20) | BIT(11))
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

/*
 * TODO: ACTLR is of class implementation defined. All core implementations
 * in armv8a have the same implementation so far w.r.t few controls.
 * When there will be differences we have to create core specific headers.
 */
#define ACTLR_EL3_CPUACTLR	BIT(0)
#define ACTLR_EL3_CPUECTLR	BIT(1)
#define ACTLR_EL3_L2CTLR	BIT(4)
#define ACTLR_EL3_L2ECTLR	BIT(5)
#define ACTLR_EL3_L2ACTLR	BIT(6)

#define CPTR_EL3_RES_VAL	(0x0)
#define CPTR_EL3_EZ		BIT(8)
#define CPTR_EL3_TFP		BIT(9)
#define CPTR_EL3_TTA		BIT(20)
#define CPTR_EL3_TCPAC		BIT(31)

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
