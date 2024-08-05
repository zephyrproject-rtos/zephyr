/*
 * Copyright (c) 2018 Lexmark International, Inc.
 * Copyright 2024 Arm Limited and/or its affiliates <open-source-office@arm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_A_R_CPU_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_A_R_CPU_H_

#if defined(CONFIG_ARM_MPU)
#include <zephyr/arch/arm/cortex_a_r/mpu.h>
#endif

/*
 * SCTLR register bit assignments
 */
#define SCTLR_MPU_ENABLE	(1 << 0)

#define MODE_USR	0x10
#define MODE_FIQ	0x11
#define MODE_IRQ	0x12
#define MODE_SVC	0x13
#define MODE_ABT	0x17
#define MODE_HYP	0x1a
#define MODE_UND	0x1b
#define MODE_SYS	0x1f
#define MODE_MASK	0x1f

#define E_BIT	(1 << 9)
#define A_BIT	(1 << 8)
#define I_BIT	(1 << 7)
#define F_BIT	(1 << 6)
#define T_BIT	(1 << 5)

#define HIVECS	(1 << 13)

#define CPACR_NA	(0U)
#define CPACR_FA	(3U)

#define CPACR_CP10(r)	(r << 20)
#define CPACR_CP11(r)	(r << 22)

#define FPEXC_EN	(1 << 30)

#define DFSR_DOMAIN_SHIFT	(4)
#define DFSR_DOMAIN_MASK	(0xf)
#define DFSR_FAULT_4_MASK	(1 << 10)
#define DFSR_WRITE_MASK		(1 << 11)
#define DFSR_AXI_SLAVE_MASK	(1 << 12)

/* Armv8-R AArch32 architecture profile */
#define VBAR_MASK		(0xFFFFFFE0U)
#define SCTLR_M_BIT		BIT(0)
#define SCTLR_A_BIT		BIT(1)
#define SCTLR_C_BIT		BIT(2)
#define SCTLR_I_BIT		BIT(12)

/* Hyp System Control Register */
#define HSCTLR_RES1		(BIT(29) | BIT(28) | BIT(23) | \
				 BIT(22) | BIT(18) | BIT(16) | \
				 BIT(11) | BIT(4)  | BIT(3))

/* Hyp Auxiliary Control Register */
#define HACTLR_CPUACTLR         BIT(0)
#define HACTLR_CDBGDCI          BIT(1)
#define HACTLR_FLASHIFREGIONR   BIT(7)
#define HACTLR_PERIPHPREGIONR   BIT(8)
#define HACTLR_QOSR_BIT         BIT(9)
#define HACTLR_BUSTIMEOUTR_BIT  BIT(10)
#define HACTLR_INTMONR_BIT      BIT(12)
#define HACTLR_ERR_BIT          BIT(13)

#define HACTLR_INIT (HACTLR_ERR_BIT | HACTLR_INTMONR_BIT | \
		     HACTLR_BUSTIMEOUTR_BIT | HACTLR_QOSR_BIT | \
		     HACTLR_PERIPHPREGIONR | HACTLR_FLASHIFREGIONR | \
		     HACTLR_CDBGDCI | HACTLR_CPUACTLR)
/* ARMv8 Timer */
#define CNTV_CTL_ENABLE_BIT	BIT(0)
#define CNTV_CTL_IMASK_BIT	BIT(1)

/* Interrupt Controller System Register Enable Register */
#define ICC_SRE_ELx_SRE_BIT	BIT(0)
#define ICC_SRE_ELx_DFB_BIT	BIT(1)
#define ICC_SRE_ELx_DIB_BIT	BIT(2)
#define ICC_SRE_EL3_EN_BIT	BIT(3)

/* MPIDR mask to extract Aff0, Aff1, and Aff2 */
#define MPIDR_AFFLVL_MASK (0xffffff)

#define MPIDR_AFF0_SHIFT	(0)
#define MPIDR_AFF1_SHIFT	(8)
#define MPIDR_AFF2_SHIFT	(16)

#define MPIDR_AFFLVL(mpidr, aff_level) \
		(((mpidr) >> MPIDR_AFF##aff_level##_SHIFT) & MPIDR_AFFLVL_MASK)

#define GET_MPIDR()		read_sysreg(mpidr)
#define MPIDR_TO_CORE(mpidr)	MPIDR_AFFLVL(mpidr, 0)

/* ICC SGI macros */
#define SGIR_TGT_MASK		(0xffff)
#define SGIR_AFF1_SHIFT		(16)
#define SGIR_AFF2_SHIFT		(32)
#define SGIR_AFF3_SHIFT		(48)
#define SGIR_AFF_MASK		(0xff)
#define SGIR_INTID_SHIFT	(24)
#define SGIR_INTID_MASK		(0xf)
#define SGIR_IRM_SHIFT		(40)
#define SGIR_IRM_MASK		(0x1)
#define SGIR_IRM_TO_AFF		(0)

#define GICV3_SGIR_VALUE(_aff3, _aff2, _aff1, _intid, _irm, _tgt)	\
	((((uint64_t) (_aff3) & SGIR_AFF_MASK) << SGIR_AFF3_SHIFT) |	\
	 (((uint64_t) (_irm) & SGIR_IRM_MASK) << SGIR_IRM_SHIFT) |	\
	 (((uint64_t) (_aff2) & SGIR_AFF_MASK) << SGIR_AFF2_SHIFT) |	\
	 (((_intid) & SGIR_INTID_MASK) << SGIR_INTID_SHIFT) |		\
	 (((_aff1) & SGIR_AFF_MASK) << SGIR_AFF1_SHIFT) |		\
	 ((_tgt) & SGIR_TGT_MASK))

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_A_R_CPU_H_ */
