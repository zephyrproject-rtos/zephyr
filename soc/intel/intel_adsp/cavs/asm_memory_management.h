/* Copyright (c) 2023 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ZEPHYR_CAVS_LIB_ASM_MEMORY_MANAGEMENT_H__
#define __ZEPHYR_CAVS_LIB_ASM_MEMORY_MANAGEMENT_H__

#ifdef _ASMLANGUAGE

#define HSPGCTL0		0x71D10
#define HSRMCTL0		0x71D14
#define HSPGISTS0		0x71D18

#define LSPGCTL			0x71D50
#define LSRMCTL			0x71D54
#define LSPGISTS		0x71D58

#define SHIM_HSPGCTL(x)		(HSPGCTL0 + 0x10 * (x))
#define SHIM_HSPGISTS(x)	(HSPGISTS0 + 0x10 * (x))

#define LPSRAM_MASK	0x1
/**
 * Macro powers down entire HPSRAM. On entry literals and code for section from
 * where this code is executed need to be placed in memory which is not
 * HPSRAM (in case when this code is located in HPSRAM, lock memory in L1$ or
 * L1 SRAM)
 */
.macro m_cavs_hpsram_power_down_entire ax, ay, az
	/* SEGMENT #0 */
	movi \az, SHIM_HSPGCTL(0)
	movi \ax, SHIM_HSPGISTS(0)
	movi \ay, 0x1FFFFFFF /* HPSRAM_MASK(0) */
	s32i \ay, \ax, 0
	memw
1 :
	l32i \ax, \az, 0
	bne \ax, \ay, 1b

	/* SEGMENT #1 */
	movi \az, SHIM_HSPGCTL(1)
	movi \ax, SHIM_HSPGISTS(1)
	movi \ay, 0x0FFFFFFF /* HPSRAM_MASK(1) */
	s32i \ay, \ax, 0
	memw
1 :
	l32i \ax, \az, 0
	bne \ax, \ay, 1b
.endm

.macro m_cavs_hpsram_power_change segment_index, mask, ax, ay, az
	movi \ax, SHIM_HSPGCTL(\segment_index)
	movi \ay, SHIM_HSPGISTS(\segment_index)
	s32i \mask, \ax, 0
	memw
	/* assumed that HDA shared dma buffer will be in LPSRAM */
1 :
	l32i \ax, \ay, 0
	bne \ax, \mask, 1b
.endm

.macro m_cavs_lpsram_power_down_entire ax, ay, az, loop_cnt_addr
	movi \az, LSPGISTS
	movi \ax, LSPGCTL
	movi \ay, LPSRAM_MASK
	s32i \ay, \ax, 0
	memw
	/* assumed that HDA shared dma buffer will be in LPSRAM */
	movi \ax, \loop_cnt_addr
	l32i \ax, \ax, 0
1 :
	addi \ax, \ax, -1
	bnez \ax, 1b
.endm

#endif
#endif
