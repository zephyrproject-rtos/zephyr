/* Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Macros for power gating memory banks specific for ACE 1.0
 */

#ifndef __ZEPHYR_ACE_LIB_ASM_MEMORY_MANAGEMENT_H__
#define __ZEPHYR_ACE_LIB_ASM_MEMORY_MANAGEMENT_H__

#ifdef _ASMLANGUAGE

/* These definitions should be placed elsewhere, but I can't find a good place for them. */
#define LSPGCTL						0x71D80
#define MAX_MEMORY_SEGMENTS			1
#define EBB_SEGMENT_SIZE			32
#define PLATFORM_HPSRAM_EBB_COUNT	22

.macro m_ace_hpsram_power_change segment_index, mask, ax, ay, az, au, aw
	.if \segment_index == 0
		.if EBB_SEGMENT_SIZE > PLATFORM_HPSRAM_EBB_COUNT
			.set i_end, PLATFORM_HPSRAM_EBB_COUNT
		.else
			.set i_end, EBB_SEGMENT_SIZE
		.endif
	.elseif PLATFORM_HPSRAM_EBB_COUNT >= EBB_SEGMENT_SIZE
		.set i_end, PLATFORM_HPSRAM_EBB_COUNT - EBB_SEGMENT_SIZE
	.else
		.err
	.endif
	.set ebb_index, \segment_index << 5
	.set i, 0               /* i = bank bit in segment */

	rsr.sar \aw             /* store old sar value */

	movi \az, (0x17A800 + 0x0008 * ebb_index)/* SHIM_HSPGCTL(ebb_index) */
	movi \au, i_end - 1     /* au = banks count in segment */
2 :
	/* au = current bank in segment */
	mov \ax, \mask          /* ax = mask */
	ssr \au
	srl \ax, \ax            /* ax >>= current bank */
	extui \ax, \ax, 0, 1    /* ax &= BIT(1) */
	s8i \ax, \az, 0         /* HSxPGCTL.l2lmpge = ax */
	memw
	1 :
		l8ui \ay, \az, 4    /* ax=HSxPGISTS.l2lmpgis */
		bne \ax, \ay, 1b    /* wait till status==request */

	addi \az, \az, 8
	addi \au, \au, -1
	bnez \au, 2b

	wsr.sar \aw
.endm

.macro m_ace_lpsram_power_down_entire ax, ay, az, au
	movi \au, 8 /* LPSRAM_EBB_QUANTITY */
	movi \az, LSPGCTL

	movi \ay, 1
2 :
	s8i \ay, \az, 0
	memw

1 :
	l8ui \ax, \az, 4
	bne \ax, \ay, 1b

	addi \az, \az, 8
	addi \au, \au, -1
	bnez \au, 2b
.endm
#endif /* _ASMLANGUAGE */
#endif /* __Z_ACE_LIB_ASM_MEMORY_MANAGEMENT_H__ */
