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

/* System register interface to GICv3 */
#define ICC_IGRPEN1_EL1		S3_0_C12_C12_7
#define ICC_SGI1R		S3_0_C12_C11_5
#define ICC_SRE_EL1		S3_0_C12_C12_5
#define ICC_SRE_EL2		S3_4_C12_C9_5
#define ICC_SRE_EL3		S3_6_C12_C12_5
#define ICC_CTLR_EL1		S3_0_C12_C12_4
#define ICC_CTLR_EL3		S3_6_C12_C12_4
#define ICC_PMR_EL1		S3_0_C4_C6_0
#define ICC_RPR_EL1		S3_0_C12_C11_3
#define ICC_IGRPEN1_EL3		S3_6_C12_C12_7
#define ICC_IGRPEN0_EL1		S3_0_C12_C12_6
#define ICC_HPPIR0_EL1		S3_0_C12_C8_2
#define ICC_HPPIR1_EL1		S3_0_C12_C12_2
#define ICC_IAR0_EL1		S3_0_C12_C8_0
#define ICC_IAR1_EL1		S3_0_C12_C12_0
#define ICC_EOIR0_EL1		S3_0_C12_C8_1
#define ICC_EOIR1_EL1		S3_0_C12_C12_1
#define ICC_SGI0R_EL1		S3_0_C12_C11_7

/* register constants */
#define ICC_SRE_ELx_SRE		BIT(0)
#define ICC_SRE_ELx_DFB		BIT(1)
#define ICC_SRE_ELx_DIB		BIT(2)
#define ICC_SRE_EL3_EN		BIT(3)

/* ICC SGI macros */
#define SGIR_TGT_MASK		0xffff
#define SGIR_AFF1_SHIFT		16
#define SGIR_INTID_SHIFT	24
#define SGIR_INTID_MASK		0xf
#define SGIR_AFF2_SHIFT		32
#define SGIR_IRM_SHIFT		40
#define SGIR_IRM_MASK		0x1
#define SGIR_AFF3_SHIFT		48
#define SGIR_AFF_MASK		0xf

#define SGIR_IRM_TO_AFF		0

#define GICV3_SGIR_VALUE(_aff3, _aff2, _aff1, _intid, _irm, _tgt)	\
	((((uint64_t) (_aff3) & SGIR_AFF_MASK) << SGIR_AFF3_SHIFT) |	\
	 (((uint64_t) (_irm) & SGIR_IRM_MASK) << SGIR_IRM_SHIFT) |	\
	 (((uint64_t) (_aff2) & SGIR_AFF_MASK) << SGIR_AFF2_SHIFT) |	\
	 (((_intid) & SGIR_INTID_MASK) << SGIR_INTID_SHIFT) |		\
	 (((_aff1) & SGIR_AFF_MASK) << SGIR_AFF1_SHIFT) |		\
	 ((_tgt) & SGIR_TGT_MASK))

/* Implementation defined register definations */
#if defined(CONFIG_CPU_CORTEX_A72)

#define CORTEX_A72_L2CTLR_EL1				S3_1_C11_C0_2
#define CORTEX_A72_L2CTLR_DATA_RAM_LATENCY_SHIFT	0
#define CORTEX_A72_L2CTLR_DATA_RAM_SETUP_SHIFT		5
#define CORTEX_A72_L2CTLR_TAG_RAM_LATENCY_SHIFT		6
#define CORTEX_A72_L2CTLR_TAG_RAM_SETUP_SHIFT		9

#define CORTEX_A72_L2_DATA_RAM_LATENCY_3_CYCLES		2
#define CORTEX_A72_L2_DATA_RAM_LATENCY_MASK		7
#define CORTEX_A72_L2_DATA_RAM_SETUP_1_CYCLE		1
#define CORTEX_A72_L2_TAG_RAM_LATENCY_2_CYCLES		1
#define CORTEX_A72_L2_TAG_RAM_LATENCY_3_CYCLES		2
#define CORTEX_A72_L2_TAG_RAM_LATENCY_MASK		7
#define CORTEX_A72_L2_TAG_RAM_SETUP_1_CYCLE		1


#define CORTEX_A72_L2ACTLR_EL1				S3_1_C15_C0_0

#define CORTEX_A72_L2ACTLR_DISABLE_ACE_SH_OR_CHI	(1 << 6)
#endif /* CONFIG_CPU_CORTEX_A72 */

#ifndef _ASMLANGUAGE
/* Core sysreg macros */
#define read_sysreg(reg) ({					\
	uint64_t val;						\
	__asm__ volatile("mrs %0, " STRINGIFY(reg) : "=r" (val));\
	val;							\
})

#define write_sysreg(val, reg) ({				\
	__asm__ volatile("msr " STRINGIFY(reg) ", %0" : : "r" (val));\
})

#endif  /* !_ASMLANGUAGE */

#define MODE_EL_SHIFT		(0x2)
#define MODE_EL_MASK		(0x3)

#define MODE_EL3		(0x3)
#define MODE_EL2		(0x2)
#define MODE_EL1		(0x1)
#define MODE_EL0		(0x0)

#define GET_EL(_mode)		(((_mode) >> MODE_EL_SHIFT) & MODE_EL_MASK)

/* mpidr */
#define MPIDR_AFFLVL_MASK	0xff

#define MPIDR_AFF0_SHIFT	0
#define MPIDR_AFF1_SHIFT	8
#define MPIDR_AFF2_SHIFT	16
#define MPIDR_AFF3_SHIFT	32

#define MPIDR_AFFLVL(mpidr, aff_level) \
		(((mpidr) >> MPIDR_AFF##aff_level##_SHIFT) & MPIDR_AFFLVL_MASK)

#define GET_MPIDR()		read_sysreg(mpidr_el1)
#define MPIDR_TO_CORE(mpidr)	MPIDR_AFFLVL(mpidr, 0)
#endif /* ZEPHYR_INCLUDE_ARCH_ARM_AARCH64_CPU_H_ */
