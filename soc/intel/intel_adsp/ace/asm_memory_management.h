/* Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Macros for power gating memory banks specific for ACE 1.0
 */

#ifndef __ZEPHYR_ACE_LIB_ASM_MEMORY_MANAGEMENT_H__
#define __ZEPHYR_ACE_LIB_ASM_MEMORY_MANAGEMENT_H__

#include <zephyr/devicetree.h>

#ifdef _ASMLANGUAGE

.macro m_ace_lpsram_power_down_entire ax, ay, az, au
	/* Retrieve the LPSRAM bank count from the ACE_L2MCAP register */
	movi \az, DT_REG_ADDR(DT_NODELABEL(hsbcap))
	l32i \az, \az, 0
	/* Extract the 4-bit bank count field starting from bit 8 */
	extui \au, \az, 8, 4

	movi \ay, 1       /* Power down command */

	/* Get the address of the LPSRAM control register from the Devicetree */
	movi \az, DT_REG_ADDR(DT_NODELABEL(lsbpm))
2 :
	/* Issue the power down command to the current LPSRAM bank */
	s8i \ay, \az, 0
	memw
1 :
	/* Poll the status register to confirm the power down command has taken effect */
	l8ui \ax, \az, 4
	bne \ax, \ay, 1b

	/* Move to the next LPSRAM bank control register */
	addi \az, \az, DT_REG_SIZE(DT_NODELABEL(lsbpm))
	addi \au, \au, -1 /* Decrement bank count */
	bnez \au, 2b      /* If banks are left, continue loop */
.endm

.macro m_ace_hpsram_power_down_entire ax, ay, az, au
	/* Read the HPSRAM bank count from ACE_L2MCAP register */
	movi \au, DT_REG_ADDR(DT_NODELABEL(hsbcap))
	l32i \au, \au, 0
	extui \au, \au, 0, 8 /* Bank count is in the lower 8 bits */

	movi \ay, 1             /* Power down command */

	/* Calculate the address of the HSxPGCTL register */
	movi \az, DT_REG_ADDR(DT_NODELABEL(hsbpm))
2 :
	s8i \ay, \az, 0         /* HSxPGCTL.l2lmpge = 1 (power down) */
	memw
1 :
	l8ui \ax, \az, 4        /* ax = HSxPGISTS.l2lmpgis */
	bne \ax, \ay, 1b        /* wait till status == request */

	addi \az, \az, DT_REG_SIZE(DT_NODELABEL(hsbpm)) /* Move to next bank control register */
	addi \au, \au, -1       /* Decrement bank count */
	bnez \au, 2b            /* If banks are left, continue loop */
.endm
#endif /* _ASMLANGUAGE */
#endif /* __Z_ACE_LIB_ASM_MEMORY_MANAGEMENT_H__ */
