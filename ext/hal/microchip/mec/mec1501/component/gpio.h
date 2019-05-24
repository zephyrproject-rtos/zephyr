/**
 *
 * Copyright (c) 2019 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the Licence at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * \asf_license_stop
 *
 */

/** @file gpio.h
 *MEC1501 GPIO definitions
 */
/** @defgroup MEC1501 Peripherals GPIO
 */

#ifndef _GPIO_H
#define _GPIO_H

#include <stdint.h>
#include <stddef.h>

#define NUM_MCHP_GPIO_PORTS	6u
#define MAX_NUM_MCHP_GPIO	(NUM_MCHP_GPIO_PORTS * 32u)

#define MCHP_GPIO_CTRL_BASE	0x40081000ul
#define MCHP_GPIO_PARIN_OFS	0x0300ul
#define MCHP_GPIO_PAROUT_OFS	0x0380ul
#define MCHP_GPIO_LOCK_OFS	0x03E8ul
#define MCHP_GPIO_CTRL2_OFS	0x0500ul

#define MCHP_GPIO_PARIN_BASE	(MCHP_GPIO_CTRL_BASE + MCHP_GPIO_PARIN_OFS)
#define MCHP_GPIO_PAROUT_BASE	(MCHP_GPIO_CTRL_BASE + MCHP_GPIO_PAROUT_OFS)
#define MCHP_GPIO_LOCK_BASE	(MCHP_GPIO_CTRL_BASE + MCHP_GPIO_LOCK_OFS)
#define MCHP_GPIO_CTRL2_BASE	(MCHP_GPIO_CTRL_BASE + MCHP_GPIO_CTRL2_OFS)

/*
 * !!! MEC15xx data sheets pin numbering is octal !!!
 * n = pin number in octal or the equivalent in decimal or hex
 * Example: GPIO135
 * n = 0135 or n = 0x5D or n = 93
 */
#define MCHP_GPIO_CTRL_ADDR(n) \
	((uintptr_t)(MCHP_GPIO_CTRL_BASE) + ((uintptr_t)(n) << 2))

#define MCHP_GPIO_CTRL2_ADDR(n) \
	((uintptr_t)(MCHP_GPIO_CTRL2_BASE + MCHP_GPIO_CTRL2_OFS) +\
	((uintptr_t)(n) << 2))

/*
 * GPIO Parallel In registers.
 * Each register contains 32 GPIO's
 * PARIN0 for GPIO_0000 - 0037
 * PARIN1 for GPIO_0040 - 0077
 * PARIN2 for GPIO_0100 - 0137
 * PARIN3 for GPIO_0140 - 0177
 * PARIN4 for GPIO_0200 - 0237
 * PARIN5 for GPIO_0240 - 0277
 */
#define MCHP_GPIO_PARIN_ADDR(n) ((uintptr_t)(MCHP_GPIO_BASE_ADDR) +\
	(uintptr_t)(MCHP_GPIO_PARIN_OFS) + ((n) << 2))

#define MCHP_GPIO_PARIN0_ADDR ((uintptr_t)(MCHP_GPIO_BASE_ADDR) +\
	(uintptr_t)(MCHP_GPIO_PARIN_OFS))

#define MCHP_GPIO_PARIN1_ADDR ((uintptr_t)(MCHP_GPIO_BASE_ADDR) +\
	(uintptr_t)(MCHP_GPIO_PARIN_OFS) + 0x04ul)

#define MCHP_GPIO_PARIN2_ADDR ((uintptr_t)(MCHP_GPIO_BASE_ADDR) +\
	(uintptr_t)(MCHP_GPIO_PARIN_OFS) + 0x08ul)

#define MCHP_GPIO_PARIN3_ADDR ((uintptr_t)(MCHP_GPIO_BASE_ADDR) +\
	(uintptr_t)(MCHP_GPIO_PARIN_OFS) + 0x0Cul)

#define MCHP_GPIO_PARIN4_ADDR ((uintptr_t)(MCHP_GPIO_BASE_ADDR) +\
	(uintptr_t)(MCHP_GPIO_PARIN_OFS) + 0x10ul)

#define MCHP_GPIO_PARIN5_ADDR ((uintptr_t)(MCHP_GPIO_BASE_ADDR) +\
	(uintptr_t)(MCHP_GPIO_PARIN_OFS) + 0x14ul)

/*
 * GPIO Parallel Out registers.
 * Each register contains 32 GPIO's
 * PAROUT0 for GPIO_0000 - 0037
 * PAROUT1 for GPIO_0040 - 0077
 * PAROUT2 for GPIO_0100 - 0137
 * PAROUT3 for GPIO_0140 - 0177
 * PAROUT4 for GPIO_0200 - 0237
 * PAROUT5 for GPIO_0240 - 0277
 */
#define MCHP_GPIO_PAROUT_ADDR(n) ((uintptr_t)(MCHP_GPIO_BASE_ADDR) +\
	(uintptr_t)(MCHP_GPIO_PAROUT_OFS) + ((n) << 2))

#define MCHP_GPIO_PAROUT0_ADDR ((uintptr_t)(MCHP_GPIO_BASE_ADDR) +\
	(uintptr_t)(MCHP_GPIO_PAROUT_OFS))

#define MCHP_GPIO_PAROUT1_ADDR ((uintptr_t)(MCHP_GPIO_BASE_ADDR) +\
	(uintptr_t)(MCHP_GPIO_PAROUT_OFS) + 0x04ul)

#define MCHP_GPIO_PAROUT2_ADDR ((uintptr_t)(MCHP_GPIO_BASE_ADDR) +\
	(uintptr_t)(MCHP_GPIO_PAROUT_OFS) + 0x08ul)

#define MCHP_GPIO_PAROUT3_ADDR ((uintptr_t)(MCHP_GPIO_BASE_ADDR) +\
	(uintptr_t)(MCHP_GPIO_PAROUT_OFS) + 0x0Cul)

#define MCHP_GPIO_PAROUT4_ADDR ((uintptr_t)(MCHP_GPIO_BASE_ADDR) +\
	(uintptr_t)(MCHP_GPIO_PAROUT_OFS) + 0x10ul)

#define MCHP_GPIO_PAROUT5_ADDR ((uintptr_t)(MCHP_GPIO_BASE_ADDR) +\
	(uintptr_t)(MCHP_GPIO_PAROUT_OFS) + 0x14ul)

/*
 * MEC1501H-B0-SZ (144-pin)
 */
#define MCHP_GPIO_PORT_A_BITMAP 0x7FFFFF9Dul	/* GPIO_0000 - 0036  GIRQ11 */
#define MCHP_GPIO_PORT_B_BITMAP 0x0FFFFFFDul	/* GPIO_0040 - 0076  GIRQ10 */
#define MCHP_GPIO_PORT_C_BITMAP 0x07FF3CF7ul	/* GPIO_0100 - 0136  GIRQ09 */
#define MCHP_GPIO_PORT_D_BITMAP 0x272EFFFFul	/* GPIO_0140 - 0176  GIRQ08 */
#define MCHP_GPIO_PORT_E_BITMAP 0x00DE00FFul	/* GPIO_0200 - 0236  GIRQ12 */
#define MCHP_GPIO_PORT_F_BITMAP 0x0000397Ful	/* GPIO_0240 - 0276  GIRQ26 */

#define MCHP_GPIO_PORT_A_DRVSTR_BITMAP	0x7FFFFF9Dul
#define MCHP_GPIO_PORT_B_DRVSTR_BITMAP	0x0FFFFFFDul
#define MCHP_GPIO_PORT_C_DRVSTR_BITMAP	0x07FF3CF7ul
#define MCHP_GPIO_PORT_D_DRVSTR_BITMAP	0x272EFFFFul
#define MCHP_GPIO_PORT_E_DRVSTR_BITMAP	0x00DE00FFul
#define MCHP_GPIO_PORT_F_DRVSTR_BITMAP	0x0000397Ful

/*
 * GPIO Port to ECIA GIRQ mapping
 */
#define MCHP_GPIO_PORT_A_GIRQ	11u
#define MCHP_GPIO_PORT_B_GIRQ	10u
#define MCHP_GPIO_PORT_C_GIRQ	9u
#define MCHP_GPIO_PORT_D_GIRQ	8u
#define MCHP_GPIO_PORT_E_GIRQ	12u
#define MCHP_GPIO_PORT_F_GIRQ	26u

/*
 * GPIO Port GIRQ to NVIC external input
 * GPIO GIRQ's are always aggregated.
 */
#define MCHP_GPIO_PORT_A_NVIC	3u
#define MCHP_GPIO_PORT_B_NVIC	2u
#define MCHP_GPIO_PORT_C_NVIC	1u
#define MCHP_GPIO_PORT_D_NVIC	0u
#define MCHP_GPIO_PORT_E_NVIC	4u
#define MCHP_GPIO_PORT_F_NVIC	17u

/*
 * Control
 */
#define MCHP_GPIO_CTRL_MASK		0x0101BFFFul
/* bits[15:0] of Control register */
#define MCHP_GPIO_CTRL_CFG_MASK		0xBFFFul

/* Disable interrupt detect and pad */
#define MCHP_GPIO_CTRL_DIS_PIN		0x8040ul

#define MCHP_GPIO_CTRL_DFLT		0x8040ul
#define MCHP_GPIO_CTRL_DFLT_MASK	0xfffful

#define GPIO000_CTRL_DFLT	0x1040ul
#define GPIO161_CTRL_DFLT	0x1040ul
#define GPIO162_CTRL_DFLT	0x1040ul
#define GPIO163_CTRL_DFLT	0x1040ul
#define GPIO172_CTRL_DFLT	0x1040ul
#define GPIO062_CTRL_DFLT	0x8240ul
#define GPIO170_CTRL_DFLT	0x0041ul	/* Boot-ROM JTAG_STRAP_BS */
#define GPIO116_CTRL_DFLT	0x0041ul
#define GPIO250_CTRL_DFLT	0x1240ul

/*
 * GPIO Control register field definitions.
 */

/* bits[1:0] internal pull up/down selection */
#define MCHP_GPIO_CTRL_PUD_POS		0u
#define MCHP_GPIO_CTRL_PUD_MASK0	0x03ul
#define MCHP_GPIO_CTRL_PUD_MASK		(0x03ul << (MCHP_GPIO_CTRL_PUD_POS))
#define MCHP_GPIO_CTRL_PUD_NONE		0x00ul
#define MCHP_GPIO_CTRL_PUD_PU		0x01ul
#define MCHP_GPIO_CTRL_PUD_PD		0x02ul
/* Repeater(keeper) mode */
#define MCHP_GPIO_CTRL_PUD_RPT		0x03ul

#define MCHP_GPIO_CTRL_PUD_GET(x) (((uint32_t)(x) >> MCHP_GPIO_CTRL_PUD_POS)\
	& MCHP_GPIO_CTRL_PUD_MASK0)
#define MCHP_GPIO_CTRL_PUD_SET(x) (((uint32_t)(x) & MCHP_GPIO_CTRL_PUD_MASK0)\
	<< MCHP_GPIO_CTRL_PUD_POS)

/* bits[3:2] power gating */
#define MCHP_GPIO_CTRL_PWRG_POS		2u
#define MCHP_GPIO_CTRL_PWRG_MASK0	0x03ul
#define MCHP_GPIO_CTRL_PWRG_MASK	(0x03UL << (MCHP_GPIO_CTRL_PWRG_POS))
#define MCHP_GPIO_CTRL_PWRG_VTR_IO	(0x00UL << (MCHP_GPIO_CTRL_PWRG_POS))
#define MCHP_GPIO_CTRL_PWRG_VCC_IO	(0x01UL << (MCHP_GPIO_CTRL_PWRG_POS))
#define MCHP_GPIO_CTRL_PWRG_OFF		(0x02UL << (MCHP_GPIO_CTRL_PWRG_POS))
#define MCHP_GPIO_CTRL_PWRG_RSVD	(0x03UL << (MCHP_GPIO_CTRL_PWRG_POS))

#define MCHP_GPIO_CTRL_PWRG_GET(x) (((uint32_t)(x) >> MCHP_GPIO_CTRL_PWRG_POS)\
	& MCHP_GPIO_CTRL_PWRG_MASK0)
#define MCHP_GPIO_CTRL_PWRG_SET(x) (((uint32_t)(x) & MCHP_GPIO_CTRL_PWRG_MASK0)\
	<< MCHP_GPIO_CTRL_PWRG_POS)

/* bits[7:4] interrupt detection mode */
#define MCHP_GPIO_CTRL_IDET_POS		4u
#define MCHP_GPIO_CTRL_IDET_MASK0	0x0Ful
#define MCHP_GPIO_CTRL_IDET_MASK	(0x0FUL << (MCHP_GPIO_CTRL_IDET_POS))
#define MCHP_GPIO_CTRL_IDET_LVL_LO	(0x00UL << (MCHP_GPIO_CTRL_IDET_POS))
#define MCHP_GPIO_CTRL_IDET_LVL_HI	(0x01UL << (MCHP_GPIO_CTRL_IDET_POS))
#define MCHP_GPIO_CTRL_IDET_DISABLE	(0x04UL << (MCHP_GPIO_CTRL_IDET_POS))
#define MCHP_GPIO_CTRL_IDET_REDGE	(0x0DUL << (MCHP_GPIO_CTRL_IDET_POS))
#define MCHP_GPIO_CTRL_IDET_FEDGE	(0x0EUL << (MCHP_GPIO_CTRL_IDET_POS))
#define MCHP_GPIO_CTRL_IDET_BEDGE	(0x0FUL << (MCHP_GPIO_CTRL_IDET_POS))

#define MCHP_GPIO_CTRL_IDET_GET(x) (((uint32_t)(x) >> MCHP_GPIO_CTRL_IDET_POS)\
	& MCHP_GPIO_CTRL_IDET_MASK0)
#define MCHP_GPIO_CTRL_IDET_SET(x) (((uint32_t)(x) & MCHP_GPIO_CTRL_IDET_MASK0)\
	<< MCHP_GPIO_CTRL_IDET_POS)

/* bit[8] output buffer type: push-pull or open-drain */
#define MCHP_GPIO_CTRL_BUFT_POS		8u
#define MCHP_GPIO_CTRL_BUFT_MASK0	0x01ul
#define MCHP_GPIO_CTRL_BUFT_MASK	(1ul << (MCHP_GPIO_CTRL_BUFT_POS))
#define MCHP_GPIO_CTRL_BUFT_PUSHPULL	(0x00UL << (MCHP_GPIO_CTRL_BUFT_POS))
#define MCHP_GPIO_CTRL_BUFT_OPENDRAIN	(0x01UL << (MCHP_GPIO_CTRL_BUFT_POS))

#define MCHP_GPIO_CTRL_BUFT_GET(x) (((uint32_t)(x) >> MCHP_GPIO_CTRL_BUFT_POS)\
	& MCHP_GPIO_CTRL_BUFT_MASK0)
#define MCHP_GPIO_CTRL_BUFT_SET(x) (((uint32_t)(x) & MCHP_GPIO_CTRL_BUFT_MASK0)\
	<< MCHP_GPIO_CTRL_BUFT_POS)

/* bit[9] direction */
#define MCHP_GPIO_CTRL_DIR_POS		9u
#define MCHP_GPIO_CTRL_DIR_MASK0	0x01ul
#define MCHP_GPIO_CTRL_DIR_MASK		(0x01UL << (MCHP_GPIO_CTRL_DIR_POS))
#define MCHP_GPIO_CTRL_DIR_INPUT	(0x00UL << (MCHP_GPIO_CTRL_DIR_POS))
#define MCHP_GPIO_CTRL_DIR_OUTPUT	(0x01UL << (MCHP_GPIO_CTRL_DIR_POS))

#define MCHP_GPIO_CTRL_DIR_GET(x) (((uint32_t)(x) >> MCHP_GPIO_CTRL_DIR_POS)\
	& MCHP_GPIO_CTRL_DIR_MASK0)
#define MCHP_GPIO_CTRL_DIR_SET(x) (((uint32_t)(x) & MCHP_GPIO_CTRL_DIR_MASK0)\
<< MCHP_GPIO_CTRL_DIR_POS)

/*
 * bit[10] Alternate output disable. Default==0(alternate output enabled)
 * GPIO output value is controlled by bit[16] of this register.
 * Set bit[10]=1 if you wish to control pin output using the parallel
 * GPIO output register bit for this pin.
*/
#define MCHP_GPIO_CTRL_AOD_POS		10u
#define MCHP_GPIO_CTRL_AOD_MASK0	0x01ul
#define MCHP_GPIO_CTRL_AOD_MASK		(1ul << (MCHP_GPIO_CTRL_AOD_POS))
#define MCHP_GPIO_CTRL_AOD_DIS		(0x01UL << (MCHP_GPIO_CTRL_AOD_POS))
#define MCHP_GPIO_CTRL_AOD_EN		(0x00UL << (MCHP_GPIO_CTRL_AOD_POS))

#define MCHP_GPIO_CTRL_AOD_GET(x) (((uint32_t)(x) >> MCHP_GPIO_CTRL_AOD_POS)\
	& MCHP_GPIO_CTRL_AOD_MASK0)
#define MCHP_GPIO_CTRL_AOD_SET(x) (((uint32_t)(x) & MCHP_GPIO_CTRL_AOD_MASK0)\
	<< MCHP_GPIO_CTRL_AOD_POS)

/* bit[11] GPIO function output polarity */
#define MCHP_GPIO_CTRL_POL_POS		11u
#define MCHP_GPIO_CTRL_POL_MASK0	0x01ul
#define MCHP_GPIO_CTRL_POL_MASK		(1ul << (MCHP_GPIO_CTRL_POL_POS))
#define MCHP_GPIO_CTRL_POL_NON_INVERT	(0x00UL << (MCHP_GPIO_CTRL_POL_POS))
#define MCHP_GPIO_CTRL_POL_INVERT	(0x01UL << (MCHP_GPIO_CTRL_POL_POS))

#define MCHP_GPIO_CTRL_POL_GET(x) (((uint32_t)(x) >> MCHP_GPIO_CTRL_POL_POS)\
	& MCHP_GPIO_CTRL_POL_MASK0)
#define MCHP_GPIO_CTRL_POL_SET(x) (((uint32_t)(x) & MCHP_GPIO_CTRL_POL_MASK0)\
	<< MCHP_GPIO_CTRL_POL_POS)

/* bits[13:12] pin mux (fucntion) */
#define MCHP_GPIO_CTRL_MUX_POS		12u
#define MCHP_GPIO_CTRL_MUX_MASK0	0x03ul
#define MCHP_GPIO_CTRL_MUX_MASK		(0x0FUL << (MCHP_GPIO_CTRL_MUX_POS))
#define MCHP_GPIO_CTRL_MUX_F0		(0x00UL << (MCHP_GPIO_CTRL_MUX_POS))
#define MCHP_GPIO_CTRL_MUX_F1		(0x01UL << (MCHP_GPIO_CTRL_MUX_POS))
#define MCHP_GPIO_CTRL_MUX_F2		(0x02UL << (MCHP_GPIO_CTRL_MUX_POS))
#define MCHP_GPIO_CTRL_MUX_F3		(0x03UL << (MCHP_GPIO_CTRL_MUX_POS))

#define MCHP_GPIO_CTRL_MUX_GET(x) (((uint32_t)(x) >> MCHP_GPIO_CTRL_MUX_POS)\
	& MCHP_GPIO_CTRL_MUX_MASK0)
#define MCHP_GPIO_CTRL_MUX_SET(x) (((uint32_t)(x) & MCHP_GPIO_CTRL_MUX_MASK0)\
	<< MCHP_GPIO_CTRL_MUX_POS)

/* bit[14] = read-only 0 reserved */

/*
 * bit[15] Disables input pad leaving output pad enabled
 * Useful for reducing power consumption of output only pins.
 */
#define MCHP_GPIO_CTRL_INPAD_DIS_POS	15u
#define MCHP_GPIO_CTRL_INPAD_DIS_MASK0	0x01ul
#define MCHP_GPIO_CTRL_INPAD_DIS_MASK	(0x01ul << (MCHP_GPIO_CTRL_INPAD_DIS_POS))
#define MCHP_GPIO_CTRL_INPAD_DIS	(0x01ul << (MCHP_GPIO_CTRL_INPAD_DIS_POS))
#define MCHP_GPIO_CTRL_INPAD_EN		(0ul << (MCHP_GPIO_CTRL_INPAD_DIS_POS))

#define MCHP_GPIO_CTRL_INPAD_DIS_GET(x) (((uint32_t)(x) >> \
	MCHP_GPIO_CTRL_INPAD_DIS_POS) & MCHP_GPIO_CTRL_INPAD_DIS_MASK0)
#define MCHP_GPIO_CTRL_INPAD_DIS_SET(x) (((uint32_t)(x) & \
	MCHP_GPIO_CTRL_INPAD_DIS_MASK0) << MCHP_GPIO_CTRL_INPAD_DIS_POS)

/*
 * bit[16]: Alternate output pin value. Enabled when bit[10]==0(default)
 */
#define MCHP_GPIO_CTRL_OUTVAL_POS	16u
#define MCHP_GPIO_CTRL_OUTVAL_MASK0	0x01ul
#define MCHP_GPIO_CTRL_OUTVAL_MASK	(1ul << (MCHP_GPIO_CTRL_OUTVAL_POS))
#define MCHP_GPIO_CTRL_OUTV_LO		(0x00UL << (MCHP_GPIO_CTRL_OUTVAL_POS))
#define MCHP_GPIO_CTRL_OUTV_HI		(0x01UL << (MCHP_GPIO_CTRL_OUTVAL_POS))

#define MCHP_GPIO_CTRL_OUTVAL_GET(x) (((uint32_t)(x) >> \
	MCHP_GPIO_CTRL_OUTVAL_POS) & MCHP_GPIO_CTRL_OUTVAL_MASK0)
#define MCHP_GPIO_CTRL_OUTVAL_SET(x) (((uint32_t)(x) & \
	MCHP_GPIO_CTRL_OUTVAL_MASK0) << MCHP_GPIO_CTRL_OUTVAL_POS)

/* bit[24] Input pad value. Always live unless input pad is powered down */
#define MCHP_GPIO_CTRL_INPAD_VAL_POS	24u
#define MCHP_GPIO_CTRL_INPAD_VAL_BLEN	1u
#define MCHP_GPIO_CTRL_INPAD_VAL_MASK0	0x01ul
#define MCHP_GPIO_CTRL_INPAD_VAL_MASK	(0x01UL << (MCHP_GPIO_CTRL_INPAD_VAL_POS))
#define MCHP_GPIO_CTRL_INPAD_VAL_LO	(0x00UL << (MCHP_GPIO_CTRL_INPAD_VAL_POS))
#define MCHP_GPIO_CTRL_INPAD_VAL_HI	(0x01UL << (MCHP_GPIO_CTRL_INPAD_VAL_POS))

#define MCHP_GPIO_CTRL_INPAD_VAL_GET(x) (((uint32_t)(x) >> \
	MCHP_GPIO_CTRL_INPAD_VAL_POS) & MCHP_GPIO_CTRL_INPAD_VAL_MASK0)

#define MCHP_GPIO_CTRL_INPAD_VAL_SET(x) (((uint32_t)(x) & \
	MCHP_GPIO_CTRL_INPAD_VAL_MASK0) << MCHP_GPIO_CTRL_INPAD_VAL_POS)

#define MCHP_GPIO_CTRL_DRIVE_OD_HI \
	(MCHP_GPIO_CTRL_BUFT_OPENDRAIN + MCHP_GPIO_CTRL_DIR_OUTPUT +\
		MCHP_GPIO_CTRL_AOD_EN + MCHP_GPIO_CTRL_POL_NON_INVERT +\
		MCHP_GPIO_CTRL_MUX_GPIO + MCHP_GPIO_CTRL_OUTVAL_HI)

#define MCHP_GPIO_CTRL_DRIVE_OD_HI_MASK \
	(MCHP_GPIO_CTRL_BUFT_MASK + MCHP_GPIO_CTRL_DIR_MASK +\
		MCHP_GPIO_CTRL_AOD_MASK + MCHP_GPIO_CTRL_POL_MASK +\
		MCHP_GPIO_CTRL_MUX_MASK + MCHP_GPIO_CTRL_OUTVAL_MASK)

/*
 * Each GPIO pin implements a second control register.
 * GPIO Control 2 register selects pin drive strength and slew rate.
 * bit[0] = slew rate: 0=slow, 1=fast
 * bits[5:4] = drive strength
 * 00b = 2mA (default)
 * 01b = 4mA
 * 10b = 8mA
 * 11b = 12mA
 */
#define MCHP_GPIO_CTRL2_OFFSET		0x0500ul
#define MCHP_GPIO_CTRL2_SLEW_POS	0ul
#define MCHP_GPIO_CTRL2_SLEW_MASK	(1ul << MCHP_GPIO_CTRL2_SLEW_POS)
#define MCHP_GPIO_CTRL2_SLEW_SLOW	(0ul << MCHP_GPIO_CTRL2_SLEW_POS)
#define MCHP_GPIO_CTRL2_SLEW_FAST	(1ul << MCHP_GPIO_CTRL2_SLEW_POS)
#define MCHP_GPIO_CTRL2_DRV_STR_POS	4ul
#define MCHP_GPIO_CTRL2_DRV_STR_MASK	(0x03ul << MCHP_GPIO_CTRL2_DRV_STR_POS)
#define MCHP_GPIO_CTRL2_DRV_STR_2MA	(0ul << MCHP_GPIO_CTRL2_DRV_STR_POS)
#define MCHP_GPIO_CTRL2_DRV_STR_4MA	(1ul << MCHP_GPIO_CTRL2_DRV_STR_POS)
#define MCHP_GPIO_CTRL2_DRV_STR_8MA	(2ul << MCHP_GPIO_CTRL2_DRV_STR_POS)
#define MCHP_GPIO_CTRL2_DRV_STR_12MA	(3ul << MCHP_GPIO_CTRL2_DRV_STR_POS)

/*
 * GPIO pin numbers
 */
enum mec_gpio_idx {
	MCHP_GPIO_0000_ID = 0u,	/* Port A bit[0] */
	MCHP_GPIO_0002_ID = 2u,
	MCHP_GPIO_0003_ID,
	MCHP_GPIO_0004_ID,
	MCHP_GPIO_0007_ID = 7u,
	MCHP_GPIO_0010_ID,
	MCHP_GPIO_0011_ID,
	MCHP_GPIO_0012_ID,
	MCHP_GPIO_0013_ID,
	MCHP_GPIO_0014_ID,
	MCHP_GPIO_0015_ID,
	MCHP_GPIO_0016_ID,
	MCHP_GPIO_0017_ID,
	MCHP_GPIO_0020_ID,
	MCHP_GPIO_0021_ID,
	MCHP_GPIO_0022_ID,
	MCHP_GPIO_0023_ID,
	MCHP_GPIO_0024_ID,
	MCHP_GPIO_0025_ID,
	MCHP_GPIO_0026_ID,
	MCHP_GPIO_0027_ID,
	MCHP_GPIO_0030_ID,
	MCHP_GPIO_0031_ID,
	MCHP_GPIO_0032_ID,
	MCHP_GPIO_0033_ID,
	MCHP_GPIO_0034_ID,
	MCHP_GPIO_0035_ID,
	MCHP_GPIO_0036_ID,	/* Port A bit[30] */
	MCHP_GPIO_0040_ID = 32u,	/* Port B bit[0] */
	MCHP_GPIO_0042_ID = 34u,
	MCHP_GPIO_0043_ID,
	MCHP_GPIO_0044_ID,
	MCHP_GPIO_0045_ID,
	MCHP_GPIO_0046_ID,
	MCHP_GPIO_0047_ID,
	MCHP_GPIO_0050_ID,
	MCHP_GPIO_0051_ID,
	MCHP_GPIO_0052_ID,
	MCHP_GPIO_0053_ID,
	MCHP_GPIO_0054_ID,
	MCHP_GPIO_0055_ID,
	MCHP_GPIO_0056_ID,
	MCHP_GPIO_0057_ID,
	MCHP_GPIO_0060_ID,
	MCHP_GPIO_0061_ID,
	MCHP_GPIO_0062_ID,
	MCHP_GPIO_0063_ID,
	MCHP_GPIO_0064_ID,
	MCHP_GPIO_0065_ID,
	MCHP_GPIO_0066_ID,
	MCHP_GPIO_0067_ID,
	MCHP_GPIO_0070_ID,
	MCHP_GPIO_0071_ID,
	MCHP_GPIO_0072_ID,
	MCHP_GPIO_0073_ID,	/* Port B bit[27] */
	MCHP_GPIO_0100_ID = 64u,	/* Port C bit[0] */
	MCHP_GPIO_0101_ID,
	MCHP_GPIO_0102_ID,
	MCHP_GPIO_0104_ID = 68u,
	MCHP_GPIO_0105_ID,
	MCHP_GPIO_0106_ID,
	MCHP_GPIO_0107_ID,
	MCHP_GPIO_0112_ID = 74u,
	MCHP_GPIO_0113_ID,
	MCHP_GPIO_0114_ID,
	MCHP_GPIO_0115_ID,
	MCHP_GPIO_0120_ID = 80u,
	MCHP_GPIO_0121_ID,
	MCHP_GPIO_0122_ID,
	MCHP_GPIO_0123_ID,
	MCHP_GPIO_0124_ID,
	MCHP_GPIO_0125_ID,
	MCHP_GPIO_0126_ID,
	MCHP_GPIO_0127_ID,
	MCHP_GPIO_0130_ID,
	MCHP_GPIO_0131_ID,
	MCHP_GPIO_0132_ID,	/* Port C bit[26] */
	MCHP_GPIO_0140_ID = 96u,	/* Port D bit[0] */
	MCHP_GPIO_0141_ID,
	MCHP_GPIO_0142_ID,
	MCHP_GPIO_0143_ID,
	MCHP_GPIO_0144_ID,
	MCHP_GPIO_0145_ID,
	MCHP_GPIO_0146_ID,
	MCHP_GPIO_0147_ID,
	MCHP_GPIO_0150_ID,
	MCHP_GPIO_0151_ID,
	MCHP_GPIO_0152_ID,
	MCHP_GPIO_0153_ID,
	MCHP_GPIO_0154_ID,
	MCHP_GPIO_0155_ID,
	MCHP_GPIO_0156_ID,
	MCHP_GPIO_0157_ID,
	MCHP_GPIO_0161_ID = 113u,
	MCHP_GPIO_0162_ID,
	MCHP_GPIO_0163_ID,
	MCHP_GPIO_0165_ID = 117u,
	MCHP_GPIO_0170_ID = 120u,
	MCHP_GPIO_0171_ID,
	MCHP_GPIO_0172_ID,
	MCHP_GPIO_0175_ID = 125u,	/* Port D bit[29] */
	MCHP_GPIO_0200_ID = 128u,	/* Port E bit[0] */
	MCHP_GPIO_0201_ID,
	MCHP_GPIO_0202_ID,
	MCHP_GPIO_0203_ID,
	MCHP_GPIO_0204_ID,
	MCHP_GPIO_0205_ID,
	MCHP_GPIO_0206_ID,
	MCHP_GPIO_0207_ID,
	MCHP_GPIO_0221_ID = 145u,
	MCHP_GPIO_0222_ID,
	MCHP_GPIO_0223_ID,
	MCHP_GPIO_0224_ID,
	MCHP_GPIO_0226_ID = 150u,
	MCHP_GPIO_0227_ID,	/* Port E bit[22] */
	MCHP_GPIO_0240_ID = 160u,	/* Port F bit[0] */
	MCHP_GPIO_0241_ID,
	MCHP_GPIO_0242_ID,
	MCHP_GPIO_0243_ID,
	MCHP_GPIO_0244_ID,
	MCHP_GPIO_0245_ID,
	MCHP_GPIO_0246_ID,
	MCHP_GPIO_0250_ID = 168u,
	MCHP_GPIO_0253_ID = 171u,
	MCHP_GPIO_0254_ID = 172u,
	MCHP_GPIO_0255_ID = 173u,	/* Port F bit[13] */
};

#define MCHP_GPIO_PIN2PORT(pin_id) ((uint32_t)(pin_id) >> 5)

/* =========================================================================*/
/* ================	       GPIO			   ================ */
/* =========================================================================*/

/**
  * @brief GPIO Control (GPIO)
  */
#define MCHP_GPIO_CTRL_BEGIN	0ul
#define MCHP_GPIO_CTRL_END	0x2C4ul
#define MCHP_GPIO_PARIN_BEGIN	0x300ul
#define MCHP_GPIO_PARIN_END	0x318ul
#define MCHP_GPIO_PAROUT_BEGIN	0x380ul
#define MCHP_GPIO_PAROUT_END	0x398ul
#define MCHP_GPIO_LOCK_BEGIN	0x3E8ul
#define MCHP_GPIO_LOCK_END	0x400ul
#define MCHP_GPIO_CTRL2_BEGIN	0x500ul
#define MCHP_GPIO_CTRL2_END	0x7B4ul

#define MAX_MCHP_GPIO_PIN	((MCHP_GPIO_CTRL_END) / 4ul)
#define MAX_MCHP_GPIO_BANK	6ul
#define MCHP_GPIO_LOCK5_IDX	0ul
#define MCHP_GPIO_LOCK4_IDX	1ul
#define MCHP_GPIO_LOCK3_IDX	2ul
#define MCHP_GPIO_LOCK2_IDX	3ul
#define MCHP_GPIO_LOCK1_IDX	4ul
#define MCHP_GPIO_LOCK0_IDX	5ul
#define MCHP_GPIO_LOCK_MAX_IDX	6ul

typedef struct gpio_ctrl_regs
{
	__IOM uint32_t CTRL_0000;	/*!< (@ 0x0000) GPIO_0000 Control */
	uint8_t RSVD1[4];
	__IOM uint32_t CTRL_0002;	/*!< (@ 0x0008) GPIO_0002 Control */
	__IOM uint32_t CTRL_0003;	/*!< (@ 0x000C) GPIO_0003 Control */
	__IOM uint32_t CTRL_0004;	/*!< (@ 0x0010) GPIO_0004 Control */
	uint8_t RSVD2[8];
	__IOM uint32_t CTRL_0007;	/*!< (@ 0x001C) GPIO_0007 Control */
	__IOM uint32_t CTRL_0010;	/*!< (@ 0x0020) GPIO_0010 Control */
	__IOM uint32_t CTRL_0011;
	__IOM uint32_t CTRL_0012;
	__IOM uint32_t CTRL_0013;
	__IOM uint32_t CTRL_0014;	/*!< (@ 0x0030) GPIO_0014 Control */
	__IOM uint32_t CTRL_0015;
	__IOM uint32_t CTRL_0016;
	__IOM uint32_t CTRL_0017;
	__IOM uint32_t CTRL_0020;	/*!< (@ 0x0040) GPIO_0020 Control */
	__IOM uint32_t CTRL_0021;
	__IOM uint32_t CTRL_0022;
	__IOM uint32_t CTRL_0023;
	__IOM uint32_t CTRL_0024;	/*!< (@ 0x0050) GPIO_0024 Control */
	__IOM uint32_t CTRL_0025;
	__IOM uint32_t CTRL_0026;
	__IOM uint32_t CTRL_0027;
	__IOM uint32_t CTRL_0030;	/*!< (@ 0x0060) GPIO_0030 Control */
	__IOM uint32_t CTRL_0031;
	__IOM uint32_t CTRL_0032;
	__IOM uint32_t CTRL_0033;
	__IOM uint32_t CTRL_0034;
	__IOM uint32_t CTRL_0035;
	__IOM uint32_t CTRL_0036;	/*!< (@ 0x0078) GPIO_0036 Control */
	uint8_t RSVD3[4];
	__IOM uint32_t CTRL_0040;	/*!< (@ 0x0080) GPIO_0040 Control */
	uint8_t RSVD4[4];
	__IOM uint32_t CTRL_0042;	/*!< (@ 0x0088) GPIO_0042 Control */
	__IOM uint32_t CTRL_0043;	/*!< (@ 0x008C) GPIO_0043 Control */
	__IOM uint32_t CTRL_0044;	/*!< (@ 0x0090) GPIO_0044 Control */
	__IOM uint32_t CTRL_0045;
	__IOM uint32_t CTRL_0046;
	__IOM uint32_t CTRL_0047;
	__IOM uint32_t CTRL_0050;	/*!< (@ 0x00A0) GPIO_0050 Control */
	__IOM uint32_t CTRL_0051;
	__IOM uint32_t CTRL_0052;
	__IOM uint32_t CTRL_0053;
	__IOM uint32_t CTRL_0054;	/*!< (@ 0x00B0) GPIO_0054 Control */
	__IOM uint32_t CTRL_0055;
	__IOM uint32_t CTRL_0056;
	__IOM uint32_t CTRL_0057;
	__IOM uint32_t CTRL_0060;	/*!< (@ 0x00C0) GPIO_0060 Control */
	__IOM uint32_t CTRL_0061;
	__IOM uint32_t CTRL_0062;
	__IOM uint32_t CTRL_0063;
	__IOM uint32_t CTRL_0064;	/*!< (@ 0x00D0) GPIO_0064 Control */
	__IOM uint32_t CTRL_0065;
	__IOM uint32_t CTRL_0066;
	__IOM uint32_t CTRL_0067;
	__IOM uint32_t CTRL_0070;	/*!< (@ 0x00E0) GPIO_0070 Control */
	__IOM uint32_t CTRL_0071;
	__IOM uint32_t CTRL_0072;
	__IOM uint32_t CTRL_0073;	/*!< (@ 0x00EC) GPIO_0073 Control */
	uint8_t RSVD5[16];
	__IOM uint32_t CTRL_0100;	/*!< (@ 0x0100) GPIO_0100 Control */
	__IOM uint32_t CTRL_0101;
	__IOM uint32_t CTRL_0102;
	uint8_t RSVD6[4];
	__IOM uint32_t CTRL_0104;	/*!< (@ 0x0110) GPIO_0104 Control */
	__IOM uint32_t CTRL_0105;
	__IOM uint32_t CTRL_0106;
	__IOM uint32_t CTRL_0107;	/*!< (@ 0x011C) GPIO_0107 Control */
	uint8_t RSVD7[8];
	__IOM uint32_t CTRL_0112;	/*!< (@ 0x0128) GPIO_0112 Control */
	__IOM uint32_t CTRL_0113;
	__IOM uint32_t CTRL_0114;
	__IOM uint32_t CTRL_0115;	/*!< (@ 0x0134) GPIO_0115 Control */
	uint8_t RSVD8[8];
	__IOM uint32_t CTRL_0120;	/*!< (@ 0x0140) GPIO_0120 Control */
	__IOM uint32_t CTRL_0121;
	__IOM uint32_t CTRL_0122;
	__IOM uint32_t CTRL_0123;
	__IOM uint32_t CTRL_0124;	/*!< (@ 0x0150) GPIO_0124 Control */
	__IOM uint32_t CTRL_0125;
	__IOM uint32_t CTRL_0126;
	__IOM uint32_t CTRL_0127;	/*!< (@ 0x015C) GPIO_0127 Control */
	__IOM uint32_t CTRL_0130;	/*!< (@ 0x0160) GPIO_0130 Control */
	__IOM uint32_t CTRL_0131;	/*!< (@ 0x0164) GPIO_0131 Control */
	__IOM uint32_t CTRL_0132;	/*!< (@ 0x0168) GPIO_0132 Control */
	uint8_t RSVD9[20];
	__IOM uint32_t CTRL_0140;	/*!< (@ 0x0180) GPIO_0140 Control */
	__IOM uint32_t CTRL_0141;
	__IOM uint32_t CTRL_0142;
	__IOM uint32_t CTRL_0143;
	__IOM uint32_t CTRL_0144;	/*!< (@ 0x0190) GPIO_0144 Control */
	__IOM uint32_t CTRL_0145;
	__IOM uint32_t CTRL_0146;
	__IOM uint32_t CTRL_0147;	/*!< (@ 0x019C) GPIO_0147 Control */
	__IOM uint32_t CTRL_0150;	/*!< (@ 0x01A0) GPIO_0150 Control */
	__IOM uint32_t CTRL_0151;
	__IOM uint32_t CTRL_0152;
	__IOM uint32_t CTRL_0153;
	__IOM uint32_t CTRL_0154;	/*!< (@ 0x01B0) GPIO_0154 Control */
	__IOM uint32_t CTRL_0155;
	__IOM uint32_t CTRL_0156;
	__IOM uint32_t CTRL_0157;	/*!< (@ 0x01BC) GPIO_0157 Control */
	uint8_t RSVD10[4];
	__IOM uint32_t CTRL_0161;	/*!< (@ 0x01C4) GPIO_0161 Control */
	__IOM uint32_t CTRL_0162;
	__IOM uint32_t CTRL_0163;
	uint8_t RSVD11[4];
	__IOM uint32_t CTRL_0165;	/*!< (@ 0x01D4) GPIO_0165 Control */
	uint8_t RSVD12[8];
	__IOM uint32_t CTRL_0170;	/*!< (@ 0x01E0) GPIO_0170 Control */
	__IOM uint32_t CTRL_0171;	/*!< (@ 0x01E4) GPIO_0171 Control */
	__IOM uint32_t CTRL_0172;	/*!< (@ 0x01E8) GPIO_0172 Control */
	uint8_t RSVD13[8];
	__IOM uint32_t CTRL_0175;	/*!< (@ 0x01F4) GPIO_0175 Control */
	uint8_t RSVD14[8];
	__IOM uint32_t CTRL_0200;	/*!< (@ 0x0200) GPIO_0200 Control */
	__IOM uint32_t CTRL_0201;
	__IOM uint32_t CTRL_0202;
	__IOM uint32_t CTRL_0203;
	__IOM uint32_t CTRL_0204;	/*!< (@ 0x0210) GPIO_0204 Control */
	__IOM uint32_t CTRL_0205;
	__IOM uint32_t CTRL_0206;
	__IOM uint32_t CTRL_0207;	/*!< (@ 0x021C) GPIO_0207 Control */
	uint8_t RSVD15[36];
	__IOM uint32_t CTRL_0221;	/*!< (@ 0x0244) GPIO_0221 Control */
	__IOM uint32_t CTRL_0222;
	__IOM uint32_t CTRL_0223;
	__IOM uint32_t CTRL_0224;	/*!< (@ 0x0250) GPIO_0224 Control */
	uint8_t RSVD16[4];
	__IOM uint32_t CTRL_0226;
	__IOM uint32_t CTRL_0227;	/*!< (@ 0x025C) GPIO_0227 Control */
	uint8_t RSVD17[32];
	__IOM uint32_t CTRL_0240;	/*!< (@ 0x0280) GPIO_0240 Control */
	__IOM uint32_t CTRL_0241;
	__IOM uint32_t CTRL_0242;
	__IOM uint32_t CTRL_0243;	/*!< (@ 0x028C) GPIO_0243 Control */
	__IOM uint32_t CTRL_0244;	/*!< (@ 0x0290) GPIO_0244 Control */
	__IOM uint32_t CTRL_0245;	/*!< (@ 0x0294) GPIO_0245 Control */
	__IOM uint32_t CTRL_0246;	/*!< (@ 0x0298) GPIO_0246 Control */
	uint8_t RSVD18[4];
	__IOM uint32_t CTRL_0250;	/*!< (@ 0x02A0) GPIO_0250 Control */
	uint8_t RSVD19[8];
	__IOM uint32_t CTRL_0253;	/*!< (@ 0x02AC) GPIO_0253 Control */
	__IOM uint32_t CTRL_0254;	/*!< (@ 0x02B0) GPIO_0254 Control */
	__IOM uint32_t CTRL_0255;	/*!< (@ 0x02B4) GPIO_0255 Control */
} GPIO_CTRL_Type;

typedef struct gpio_ctrl2_regs {
	__IOM uint32_t CTRL2_0000;	/*!< (@ 0x0000) GPIO_0000 Control */
	uint8_t RSVD1[4];
	__IOM uint32_t CTRL2_0002;	/*!< (@ 0x0008) GPIO_0002 Control */
	__IOM uint32_t CTRL2_0003;	/*!< (@ 0x000C) GPIO_0003 Control */
	__IOM uint32_t CTRL2_0004;	/*!< (@ 0x0010) GPIO_0004 Control */
	uint8_t RSVD2[8];
	__IOM uint32_t CTRL2_0007;	/*!< (@ 0x001C) GPIO_0007 Control */
	__IOM uint32_t CTRL2_0010;	/*!< (@ 0x0020) GPIO_0010 Control */
	__IOM uint32_t CTRL2_0011;
	__IOM uint32_t CTRL2_0012;
	__IOM uint32_t CTRL2_0013;
	__IOM uint32_t CTRL2_0014;	/*!< (@ 0x0030) GPIO_0014 Control */
	__IOM uint32_t CTRL2_0015;
	__IOM uint32_t CTRL2_0016;
	__IOM uint32_t CTRL2_0017;
	__IOM uint32_t CTRL2_0020;	/*!< (@ 0x0040) GPIO_0020 Control */
	__IOM uint32_t CTRL2_0021;
	__IOM uint32_t CTRL2_0022;
	__IOM uint32_t CTRL2_0023;
	__IOM uint32_t CTRL2_0024;	/*!< (@ 0x0050) GPIO_0024 Control */
	__IOM uint32_t CTRL2_0025;
	__IOM uint32_t CTRL2_0026;
	__IOM uint32_t CTRL2_0027;
	__IOM uint32_t CTRL2_0030;	/*!< (@ 0x0060) GPIO_0030 Control */
	__IOM uint32_t CTRL2_0031;
	__IOM uint32_t CTRL2_0032;
	__IOM uint32_t CTRL2_0033;
	__IOM uint32_t CTRL2_0034;
	__IOM uint32_t CTRL2_0035;
	__IOM uint32_t CTRL2_0036;	/*!< (@ 0x0078) GPIO_0036 Control */
	uint8_t RSVD3[4];
	__IOM uint32_t CTRL2_0040;	/*!< (@ 0x0080) GPIO_0040 Control */
	uint8_t RSVD4[4];
	__IOM uint32_t CTRL2_0042;	/*!< (@ 0x0088) GPIO_0042 Control */
	__IOM uint32_t CTRL2_0043;	/*!< (@ 0x008C) GPIO_0043 Control */
	__IOM uint32_t CTRL2_0044;	/*!< (@ 0x0090) GPIO_0044 Control */
	__IOM uint32_t CTRL2_0045;
	__IOM uint32_t CTRL2_0046;
	__IOM uint32_t CTRL2_0047;
	__IOM uint32_t CTRL2_0050;	/*!< (@ 0x00A0) GPIO_0050 Control */
	__IOM uint32_t CTRL2_0051;
	__IOM uint32_t CTRL2_0052;
	__IOM uint32_t CTRL2_0053;
	__IOM uint32_t CTRL2_0054;	/*!< (@ 0x00B0) GPIO_0054 Control */
	__IOM uint32_t CTRL2_0055;
	__IOM uint32_t CTRL2_0056;
	__IOM uint32_t CTRL2_0057;
	__IOM uint32_t CTRL2_0060;	/*!< (@ 0x00C0) GPIO_0060 Control */
	__IOM uint32_t CTRL2_0061;
	__IOM uint32_t CTRL2_0062;
	__IOM uint32_t CTRL2_0063;
	__IOM uint32_t CTRL2_0064;	/*!< (@ 0x00D0) GPIO_0064 Control */
	__IOM uint32_t CTRL2_0065;
	__IOM uint32_t CTRL2_0066;
	__IOM uint32_t CTRL2_0067;
	__IOM uint32_t CTRL2_0070;	/*!< (@ 0x00E0) GPIO_0070 Control */
	__IOM uint32_t CTRL2_0071;
	__IOM uint32_t CTRL2_0072;
	__IOM uint32_t CTRL2_0073;	/*!< (@ 0x00EC) GPIO_0073 Control */
	uint8_t RSVD5[16];
	__IOM uint32_t CTRL2_0100;	/*!< (@ 0x0100) GPIO_0100 Control */
	__IOM uint32_t CTRL2_0101;
	__IOM uint32_t CTRL2_0102;
	uint8_t RSVD6[4];
	__IOM uint32_t CTRL2_0104;	/*!< (@ 0x0110) GPIO_0104 Control */
	__IOM uint32_t CTRL2_0105;
	__IOM uint32_t CTRL2_0106;
	__IOM uint32_t CTRL2_0107;	/*!< (@ 0x011C) GPIO_0107 Control */
	uint8_t RSVD7[8];
	__IOM uint32_t CTRL2_0112;	/*!< (@ 0x0128) GPIO_0112 Control */
	__IOM uint32_t CTRL2_0113;
	__IOM uint32_t CTRL2_0114;
	__IOM uint32_t CTRL2_0115;	/*!< (@ 0x0134) GPIO_0115 Control */
	uint8_t RSVD8[8];
	__IOM uint32_t CTRL2_0120;	/*!< (@ 0x0140) GPIO_0120 Control */
	__IOM uint32_t CTRL2_0121;
	__IOM uint32_t CTRL2_0122;
	__IOM uint32_t CTRL2_0123;
	__IOM uint32_t CTRL2_0124;	/*!< (@ 0x0150) GPIO_0124 Control */
	__IOM uint32_t CTRL2_0125;
	__IOM uint32_t CTRL2_0126;
	__IOM uint32_t CTRL2_0127;	/*!< (@ 0x015C) GPIO_0127 Control */
	__IOM uint32_t CTRL2_0130;	/*!< (@ 0x0160) GPIO_0130 Control */
	__IOM uint32_t CTRL2_0131;	/*!< (@ 0x0164) GPIO_0131 Control */
	__IOM uint32_t CTRL2_0132;	/*!< (@ 0x0168) GPIO_0132 Control */
	uint8_t RSVD9[20];
	__IOM uint32_t CTRL2_0140;	/*!< (@ 0x0180) GPIO_0140 Control */
	__IOM uint32_t CTRL2_0141;
	__IOM uint32_t CTRL2_0142;
	__IOM uint32_t CTRL2_0143;
	__IOM uint32_t CTRL2_0144;	/*!< (@ 0x0190) GPIO_0144 Control */
	__IOM uint32_t CTRL2_0145;
	__IOM uint32_t CTRL2_0146;
	__IOM uint32_t CTRL2_0147;	/*!< (@ 0x019C) GPIO_0147 Control */
	__IOM uint32_t CTRL2_0150;	/*!< (@ 0x01A0) GPIO_0150 Control */
	__IOM uint32_t CTRL2_0151;
	__IOM uint32_t CTRL2_0152;
	__IOM uint32_t CTRL2_0153;
	__IOM uint32_t CTRL2_0154;	/*!< (@ 0x01B0) GPIO_0154 Control */
	__IOM uint32_t CTRL2_0155;
	__IOM uint32_t CTRL2_0156;
	__IOM uint32_t CTRL2_0157;	/*!< (@ 0x01BC) GPIO_0157 Control */
	uint8_t RSVD10[4];
	__IOM uint32_t CTRL2_0161;	/*!< (@ 0x01C4) GPIO_0161 Control */
	__IOM uint32_t CTRL2_0162;
	__IOM uint32_t CTRL2_0163;
	uint8_t RSVD11[4];
	__IOM uint32_t CTRL2_0165;	/*!< (@ 0x01D4) GPIO_0165 Control */
	uint8_t RSVD12[8];
	__IOM uint32_t CTRL2_0170;	/*!< (@ 0x01E0) GPIO_0170 Control */
	__IOM uint32_t CTRL2_0171;	/*!< (@ 0x01E4) GPIO_0171 Control */
	__IOM uint32_t CTRL2_0172;	/*!< (@ 0x01E8) GPIO_0172 Control */
	uint8_t RSVD13[8];
	__IOM uint32_t CTRL2_0175;	/*!< (@ 0x01F4) GPIO_0175 Control */
	uint8_t RSVD14[8];
	__IOM uint32_t CTRL2_0200;	/*!< (@ 0x0200) GPIO_0200 Control */
	__IOM uint32_t CTRL2_0201;
	__IOM uint32_t CTRL2_0202;
	__IOM uint32_t CTRL2_0203;
	__IOM uint32_t CTRL2_0204;	/*!< (@ 0x0210) GPIO_0204 Control */
	__IOM uint32_t CTRL2_0205;
	__IOM uint32_t CTRL2_0206;
	__IOM uint32_t CTRL2_0207;	/*!< (@ 0x021C) GPIO_0207 Control */
	uint8_t RSVD15[36];
	__IOM uint32_t CTRL2_0221;	/*!< (@ 0x0244) GPIO_0221 Control */
	__IOM uint32_t CTRL2_0222;
	__IOM uint32_t CTRL2_0223;
	__IOM uint32_t CTRL2_0224;	/*!< (@ 0x0250) GPIO_0224 Control */
	uint8_t RSVD16[4];
	__IOM uint32_t CTRL2_0226;
	__IOM uint32_t CTRL2_0227;	/*!< (@ 0x025C) GPIO_0227 Control */
	uint8_t RSVD17[32];
	__IOM uint32_t CTRL2_0240;	/*!< (@ 0x0280) GPIO_0240 Control */
	__IOM uint32_t CTRL2_0241;
	__IOM uint32_t CTRL2_0242;
	__IOM uint32_t CTRL2_0243;	/*!< (@ 0x028C) GPIO_0243 Control */
	__IOM uint32_t CTRL2_0244;	/*!< (@ 0x0290) GPIO_0244 Control */
	__IOM uint32_t CTRL2_0245;	/*!< (@ 0x0294) GPIO_0245 Control */
	__IOM uint32_t CTRL2_0246;	/*!< (@ 0x0298) GPIO_0246 Control */
	uint8_t RSVD18[4];
	__IOM uint32_t CTRL2_0250;	/*!< (@ 0x02A0) GPIO_0250 Control */
	uint8_t RSVD19[8];
	__IOM uint32_t CTRL2_0253;	/*!< (@ 0x02AC) GPIO_0253 Control */
	__IOM uint32_t CTRL2_0254;	/*!< (@ 0x02B0) GPIO_0254 Control */
	__IOM uint32_t CTRL2_0255;	/*!< (@ 0x02B4) GPIO_0255 Control */
} GPIO_CTRL2_Type;

typedef struct gpio_parin_regs {
	__IOM uint32_t PARIN0;	/*!< (@ 0x0000) GPIO Parallel Input [0000:0036] */
	__IOM uint32_t PARIN1;	/*!< (@ 0x0004) GPIO Parallel Input [0040:0076] */
	__IOM uint32_t PARIN2;	/*!< (@ 0x0008) GPIO Parallel Input [0100:0136] */
	__IOM uint32_t PARIN3;	/*!< (@ 0x000C) GPIO Parallel Input [0140:0176] */
	__IOM uint32_t PARIN4;	/*!< (@ 0x0010) GPIO Parallel Input [0200:0236] */
	__IOM uint32_t PARIN5;	/*!< (@ 0x0014) GPIO Parallel Input [0240:0276] */
} GPIO_PARIN_Type;

typedef struct gpio_parout_regs {
	__IOM uint32_t PAROUT0;	/*!< (@ 0x0000) GPIO Parallel Output [0000:0036] */
	__IOM uint32_t PAROUT1;	/*!< (@ 0x0004) GPIO Parallel Output [0040:0076] */
	__IOM uint32_t PAROUT2;	/*!< (@ 0x0008) GPIO Parallel Output [0100:0136] */
	__IOM uint32_t PAROUT3;	/*!< (@ 0x000C) GPIO Parallel Output [0140:0176] */
	__IOM uint32_t PAROUT4;	/*!< (@ 0x0010) GPIO Parallel Output [0200:0236] */
	__IOM uint32_t PAROUT5;	/*!< (@ 0x0014) GPIO Parallel Output [0240:0276] */
} GPIO_PAROUT_Type;

typedef struct gpio_lock_regs {
	__IOM uint32_t LOCK5;	/*!< (@ 0x0000) GPIO Lock 5 */
	__IOM uint32_t LOCK4;	/*!< (@ 0x0004) GPIO Lock 4 */
	__IOM uint32_t LOCK3;	/*!< (@ 0x0008) GPIO Lock 3 */
	__IOM uint32_t LOCK2;	/*!< (@ 0x000C) GPIO Lock 2 */
	__IOM uint32_t LOCK1;	/*!< (@ 0x0010) GPIO Lock 1 */
	__IOM uint32_t LOCK0;	/*!< (@ 0x0014) GPIO Lock 0 */
} GPIO_LOCK_Type;

/*
 * Helper functions
 */
enum mchp_gpio_pud {
	MCHP_GPIO_NO_PUD = 0ul,
	MCHP_GPIO_PU_EN = 1ul,
	MCHP_GPIO_PD_EN = 2ul,
	MCHP_GPIO_RPT_EN = 3ul,
};

enum mchp_gpio_pwrgate {
	MCHP_GPIO_PWRGT_VTR = 0ul,
	MCHP_GPIO_PWRGT_VCC = 1ul,
	MCHP_GPIO_PWRGD_OFF = 2ul,
};

enum mchp_gpio_idet {
	MCHP_GPIO_IDET_LO_LVL = 0x00ul,
	MCHP_GPIO_IDET_HI_LVL = 0x01ul,
	MCHP_GPIO_IDET_DIS = 0x04ul,
	MCHP_GPIO_IDET_RISING_EDGE = 0x0Dul,
	MCHP_GPIO_IDET_FALLING_EDGE = 0x0Eul,
	MCHP_GPIO_IDET_BOTH_EDGES = 0x0Ful
};

enum mchp_gpio_outbuf {
	MCHP_GPIO_PUSH_PULL = 0ul,
	MCHP_GPIO_OPEN_DRAIN = 1ul,
};

enum mchp_gpio_dir {
	MCHP_GPIO_DIR_IN = 0ul,
	MCHP_GPIO_DIR_OUT = 1ul,
};

enum mchp_gpio_parout_en {
	MCHP_GPIO_PAROUT_DIS = 0ul,
	MCHP_GPIO_PAROUT_EN = 1ul,
};

enum mchp_gpio_pol {
	MCHP_GPIO_POL_NORM = 0ul,
	MCHP_GPIO_POL_INV = 1ul,
};

enum mchp_gpio_mux {
	MCHP_GPIO_MUX_GPIO = 0u,
	MCHP_GPIO_MUX_FUNC1,
	MCHP_GPIO_MUX_FUNC2,
	MCHP_GPIO_MUX_FUNC3,
	MCHP_GPIO_MUX_MAX
};

enum mchp_gpio_inpad_ctrl {
	MCHP_GPIO_INPAD_CTRL_EN = 0ul,
	MCHP_GPIO_INPAD_CTRL_DIS = 1ul,
};

enum mchp_gpio_alt_out {
	MCHP_GPIO_ALT_OUT_LO = 0ul,
	MCHP_GPIO_ALT_OUT_HI = 1ul,
};

enum mchp_gpio_slew {
	MCHP_GPIO_SLEW_SLOW = 0ul,
	MCHP_GPIO_SLEW_FAST = 1ul,
};

enum mchp_gpio_drv_str {
	MCHP_GPIO_DRV_STR_2MA = 0ul,
	MCHP_GPIO_DRV_STR_4MA = 1ul,
	MCHP_GPIO_DRV_STR_8MA = 2ul,
	MCHP_GPIO_DRV_STR_12MA = 3ul,
};

static __attribute__ ((always_inline)) inline void
mchp_gpio_pud_set(uintptr_t gp_ctrl_addr, enum mchp_gpio_pud pud)
{
	REG32(gp_ctrl_addr) =
		(REG32(gp_ctrl_addr) & ~(MCHP_GPIO_CTRL_PUD_MASK))
		| (((uint32_t) pud << MCHP_GPIO_CTRL_PUD_POS)
		& MCHP_GPIO_CTRL_PUD_MASK);
}

static __attribute__ ((always_inline)) inline void
mchp_gpio_pwrgt_set(uintptr_t gp_ctrl_addr, enum mchp_gpio_pwrgate pwrgt)
{
	REG32(gp_ctrl_addr) =
		(REG32(gp_ctrl_addr) & ~(MCHP_GPIO_CTRL_PWRG_MASK))
		| (((uint32_t) pwrgt << MCHP_GPIO_CTRL_PWRG_POS)
		& MCHP_GPIO_CTRL_PWRG_MASK);
}

static __attribute__ ((always_inline)) inline void
mchp_gpio_idet_set(uintptr_t gp_ctrl_addr, enum mchp_gpio_idet idet)
{
	REG32(gp_ctrl_addr) =
		(REG32(gp_ctrl_addr) & ~(MCHP_GPIO_CTRL_IDET_MASK))
		| (((uint32_t) idet << MCHP_GPIO_CTRL_IDET_POS)
		& MCHP_GPIO_CTRL_IDET_MASK);
}

static __attribute__ ((always_inline)) inline void
mchp_gpio_outbuf_set(uintptr_t gp_ctrl_addr, enum mchp_gpio_outbuf outbuf)
{
	REG32(gp_ctrl_addr) =
		(REG32(gp_ctrl_addr) & ~(MCHP_GPIO_CTRL_BUFT_MASK))
		| (((uint32_t) outbuf << MCHP_GPIO_CTRL_BUFT_POS)
		& MCHP_GPIO_CTRL_BUFT_MASK);
}

static __attribute__ ((always_inline)) inline void
mchp_gpio_dir_set(uintptr_t gp_ctrl_addr, enum mchp_gpio_dir dir)
{
	REG32(gp_ctrl_addr) =
		(REG32(gp_ctrl_addr) & ~(MCHP_GPIO_CTRL_DIR_MASK))
		| (((uint32_t) dir << MCHP_GPIO_CTRL_DIR_POS)
		& MCHP_GPIO_CTRL_DIR_MASK);
}

static __attribute__ ((always_inline)) inline void
mchp_gpio_parout_en_set(uintptr_t gp_ctrl_addr,
			enum mchp_gpio_parout_en parout_en)
{
	REG32(gp_ctrl_addr) =
		(REG32(gp_ctrl_addr) & ~(MCHP_GPIO_CTRL_AOD_MASK))
		| (((uint32_t) parout_en << MCHP_GPIO_CTRL_AOD_POS)
		& MCHP_GPIO_CTRL_AOD_MASK);
}

static __attribute__ ((always_inline)) inline void
mchp_gpio_pol_set(uintptr_t gp_ctrl_addr, enum mchp_gpio_pol pol)
{
	REG32(gp_ctrl_addr) =
		(REG32(gp_ctrl_addr) & ~(MCHP_GPIO_CTRL_POL_MASK))
		| (((uint32_t) pol << MCHP_GPIO_CTRL_POL_POS)
		& MCHP_GPIO_CTRL_POL_MASK);
}

static __attribute__ ((always_inline)) inline void
mchp_gpio_mux_set(uintptr_t gp_ctrl_addr, enum mchp_gpio_mux mux)
{
	REG32(gp_ctrl_addr) =
		(REG32(gp_ctrl_addr) & ~(MCHP_GPIO_CTRL_MUX_MASK))
		| (((uint32_t) mux << MCHP_GPIO_CTRL_MUX_POS)
		& MCHP_GPIO_CTRL_MUX_MASK);
}

static __attribute__ ((always_inline)) inline void
mchp_gpio_inpad_ctrl_set(uintptr_t gp_ctrl_addr,
			enum mchp_gpio_inpad_ctrl inpad_ctrl)
{
	REG32(gp_ctrl_addr) =
		(REG32(gp_ctrl_addr) & ~(MCHP_GPIO_CTRL_INPAD_DIS_MASK))
		| (((uint32_t) inpad_ctrl << MCHP_GPIO_CTRL_INPAD_DIS_POS)
		& MCHP_GPIO_CTRL_INPAD_DIS_MASK);
}

static __attribute__ ((always_inline)) inline void
mchp_gpio_alt_out_set(uintptr_t gp_ctrl_addr, enum mchp_gpio_alt_out aout_state)
{
	REG8(gp_ctrl_addr + 2ul) =
		(uint8_t) aout_state & MCHP_GPIO_CTRL_OUTVAL_MASK0;
}

static __attribute__ ((always_inline)) inline uint8_t
mchp_gpio_inpad_val_get(uintptr_t gp_ctrl_addr, enum mchp_gpio_alt_out aout_state)
{
	return REG8(gp_ctrl_addr + 3ul) & MCHP_GPIO_CTRL_INPAD_VAL_MASK0;
}

#endif				/* #ifndef _GPIO_H */
/* end gpio.h */
/**   @}
 */
