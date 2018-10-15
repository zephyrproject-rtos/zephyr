/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __HAL_BASE_H
#define __HAL_BASE_H

#ifdef __cplusplus
extern "C" {
#endif

#define BASE_IPI				0x40008000
#define BASE_WDG				0x40010000

/*
 * AON_AHB_APB
 */
#define BASE_INTC				0x40000000
#define BASE_IPI				0x40008000
#define BASE_EIC0				0x40028000
#define BASE_EIC1				0x40058000
#define BASE_GLB				0x40088000
#define BASE_AHB				0x40130000

#define BASE_AON_INTC			0x40800000
#define BASE_AON_GPIOP0			0x40804000
#define BASE_AON_GPIOP1         0x40808000
#define BASE_AON_GPIOP2         0x4080C000
#define BASE_AON_GPIOP3         0x40810000
#define BASE_AON_GPIOP4         0x40814000
#define BASE_AON_GPIOP5         0x40818000
#define BASE_AON_GPIOP6         0x4081C000
#define BASE_AON_GPIOP7         0x40820000
#define BASE_AON_GLB			0x4083C000
#define BASE_AON_PIN			0x40840000
#define BASE_AON_CLK_BASE		0x48844000
#define BASE_AON_CLK_RF			0x40844200
#define BASE_AON_SYST			0x40824000
#define BASE_AON_TMR			0x40828000
#define BASE_AON_EIC0			0x4082C000
#define BASE_AON_EIC1			0x40830000
#define BASE_AON_UART			0x40838000

#define BASE_AON_SFC_CFG        0x40890000
#define BASE_AON_SFC			0x42000000


#define __REG_SET_ADDR(reg)		(reg + 0x1000)
#define __REG_CLR_ADDR(reg)		(reg + 0x2000)

#define sci_write32(reg, val) \
	sys_write32(val, reg)

#define sci_read32 sys_read32

#define sci_reg_and(reg, val) \
	sci_write32(reg, (sci_read32(reg) & val))

#define sci_reg_or(reg, val) \
	sci_write32(reg, (sci_read32(reg) | val))

#define sci_glb_set(reg, val) \
	sys_write32(val, __REG_SET_ADDR(reg))

#define sci_glb_clr(reg, val) \
	sys_write32(val, __REG_CLR_ADDR(reg))

#ifdef __cplusplus
}
#endif

#endif
