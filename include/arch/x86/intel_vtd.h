/*
 * Copyright (c) 2020 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_X86_INTEL_VTD_H
#define ZEPHYR_INCLUDE_ARCH_X86_INTEL_VTD_H

#ifndef _ASMLANGUAGE

/*************\
 * Registers *
\*************/

#define VTD_VER_REG		0x000 /* Version */
#define VTD_CAP_REG		0x008 /* Capability */
#define VTD_ECAP_REG		0x010 /* Extended Capability */
#define VTD_GCMD_REG		0x018 /* Global Command */
#define VTD_GSTS_REG		0x01C /* Global Status */
#define VTD_RTADDR_REG		0x020 /* Root Table Address */
#define VTD_CCMD_REG		0x028 /* Context Command */
#define VTD_FSTS_REG		0x034 /* Fault Status */
#define VTD_FECTL_REG		0x038 /* Fault Event Control Register*/
#define VTD_FEDATA_REG		0x03C /* Fault Event Data */
#define VTD_FEADDR_REG		0x040 /* Fault Event Address */
#define VTD_FEUADDR_REG		0x044 /* Fault Event Upper Address */
#define VTD_AFLOG_REG		0x058 /* Advanced Fault Log */
#define VTD_PMEN_REG		0x064 /* Protected Memory Enable */
#define VTD_PLMBASE_REG		0x068 /* Protected Low Memory Base */
#define VTD_PLMLIMIT_REG	0x06C /* Protected Low Memory Limit */
#define VTD_PHMBASE_REG		0x070 /* Protected High Memory Base */
#define VTD_PHMLIMIT_REG	0x078 /* Protected High Memory Limit */
#define VTD_IQH_REG		0x080 /* Invalidation Queue Head */
#define VTD_IQT_REG		0x088 /* Invalidation Queue Tail */
#define VTD_IQA_REG		0x090 /* Invalidation Queue Address */
#define VTD_ICS_REG		0x09C /* Invalidation Completion Status */
#define VTD_IECTL_REG		0x0A0 /* Invalidation Completion Event Control */
#define VTD_IEDATA_REG		0x0A4 /* Invalidation Completion Event Data */
#define VTD_IEADDR_REG		0x0A8 /* Invalidation Completion Event Address */
#define VTD_IEUADDR_REG		0x0AC /* Invalidation Completion Event Upper Address */
#define VTD_IQERCD_REG		0x0B0 /* Invalidation Queue Error Record */
#define VTD_IRTA_REG		0x0B8 /* Interrupt Remapping Table Address */
#define VTD_PQH_REG		0x0C0 /* Page Request Queue Head */
#define VTD_PQT_REG		0x0C8 /* Page Request Queue Tail */
#define VTD_PQA_REG		0x0D0 /* Page Request Queue Address */
#define VTD_PRS_REG		0x0DC /* Page Request Status */
#define VTD_PECTL_REG		0x0E0 /* Page Request Event Control */
#define VTD_PEDATA_REG		0x0E4 /* Page Request Event Data */
#define VTD_PEADDR_REG		0x0E8 /* Page Request Event Address */
#define VTD_PEUADDR_REG		0x0EC /* Page Request Event Upper Address */
#define VTD_MTRRCAP_REG		0x100 /* MTRR Capability */
#define VTD_MTRRDEF_REG		0x108 /* MTRR Default Type */
#define VTD_MTRR_FIX64K_00000_REG 0x120 /* Fixed-range MTRR for 64K_00000 */
#define VTD_MTRR_FIX16K_80000_REG 0x128 /* Fixed-range MTRR for 16K_80000 */
#define VTD_MTRR_FIX16K_A0000_REG 0x130 /* Fixed-range MTRR for 16K_A0000 */
#define VTD_MTRR_FIX4K_C0000_REG 0x138 /* Fixed-range MTRR for 4K_C0000 */
#define VTD_MTRR_FIX4K_C8000_REG 0x140 /* Fixed-range MTRR for 4K_C8000 */
#define VTD_MTRR_FIX4K_D0000_REG 0x148 /* Fixed-range MTRR for 4K_D0000 */
#define VTD_MTRR_FIX4K_D8000_REG 0x150 /* Fixed-range MTRR for 4K_D8000 */
#define VTD_MTRR_FIX4K_E0000_REG 0x158 /* Fixed-range MTRR for 4K_E0000 */
#define VTD_MTRR_FIX4K_E8000_REG 0x160 /* Fixed-range MTRR for 4K_E8000 */
#define VTD_MTRR_FIX4K_F0000_REG 0x168 /* Fixed-range MTRR for 4K_F0000 */
#define VTD_MTRR_FIX4K_F8000_REG 0x170 /* Fixed-range MTRR for 4K_F8000 */
#define VTD_MTRR_PHYSBASE0_REG	0x180 /* Variable-range MTRR Base0 */
#define VTD_MTRR_PHYSMASK0_REG	0x188 /* Variable-range MTRR Mask0 */
#define VTD_MTRR_PHYSBASE1_REG	0x190 /* Variable-range MTRR Base1 */
#define VTD_MTRR_PHYSMASK1_REG	0x198 /* Variable-range MTRR Mask1 */
#define VTD_MTRR_PHYSBASE2_REG	0x1A0 /* Variable-range MTRR Base2 */
#define VTD_MTRR_PHYSMASK2_REG	0x1A8 /* Variable-range MTRR Mask2 */
#define VTD_MTRR_PHYSBASE3_REG	0x1B0 /* Variable-range MTRR Base3 */
#define VTD_MTRR_PHYSMASK3_REG	0x1B8 /* Variable-range MTRR Mask3 */
#define VTD_MTRR_PHYSBASE4_REG	0x1C0 /* Variable-range MTRR Base4 */
#define VTD_MTRR_PHYSMASK4_REG	0x1C8 /* Variable-range MTRR Mask4 */
#define VTD_MTRR_PHYSBASE5_REG	0x1D0 /* Variable-range MTRR Base5 */
#define VTD_MTRR_PHYSMASK5_REG	0x1D8 /* Variable-range MTRR Mask5 */
#define VTD_MTRR_PHYSBASE6_REG	0x1E0 /* Variable-range MTRR Base6 */
#define VTD_MTRR_PHYSMASK6_REG	0x1E8 /* Variable-range MTRR Mask6 */
#define VTD_MTRR_PHYSBASE7_REG	0x1F0 /* Variable-range MTRR Base7 */
#define VTD_MTRR_PHYSMASK7_REG	0x1F8 /* Variable-range MTRR Mask7 */
#define VTD_MTRR_PHYSBASE8_REG	0x200 /* Variable-range MTRR Base8 */
#define VTD_MTRR_PHYSMASK8_REG	0x208 /* Variable-range MTRR Mask8 */
#define VTD_MTRR_PHYSBASE9_REG	0x210 /* Variable-range MTRR Base9 */
#define VTD_MTRR_PHYSMASK9_REG	0x218 /* Variable-range MTRR Mask9 */
#define VTD_VCCAP_REG		0xE00 /* Virtual Command Capability */
#define VTD_VCMD		0xE10 /* Virtual Command */
#define VTD_VCRSP		0xE20 /* Virtual Command Response */

/* Global Command Register details */
#define VTD_GCMD_CFI		23
#define VTD_GCMD_SIRTP		24
#define VTD_GCMD_IRE		25
#define VTD_GCMD_QIE		26
#define VTD_GCMD_WBF		27
#define VTD_GCMD_EAFL		28
#define VTD_GCMD_SFL		29
#define VTD_GCMD_SRTP		30
#define VTD_GCMD_TE		31

/* Global Status Register details */
#define VTD_GSTS_CFIS		23
#define VTD_GSTS_SIRTPS		24
#define VTD_GSTS_IRES		25
#define VTD_GSTS_QIES		26
#define VTD_GSTS_WBFS		27
#define VTD_GSTS_EAFLS		28
#define VTD_GSTS_SFLS		29
#define VTD_GSTS_SRTPS		30
#define VTD_GSTS_TES		31

/* Interrupt Remapping Table Address Register details */
#define VTD_IRTA_SIZE_MASK	0x00000000000000FF
#define VTD_IRTA_EIME		11
#define VTD_IRTA_ADDR_SHIFT	12

#define VTD_IRTA_REG_GEN_CONTENT(addr, size, mode) \
	(0 |					   \
	 (addr << VTD_IRTA_ADDR_SHIFT) |	   \
	 (mode) | (size & VTD_IRTA_SIZE_MASK))

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_X86_INTEL_VTD_H */
