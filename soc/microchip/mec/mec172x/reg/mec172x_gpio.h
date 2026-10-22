/*
 * Copyright (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MEC172X_GPIO_H
#define _MEC172X_GPIO_H

#include <stdint.h>
#include <stddef.h>

#if defined(CONFIG_SOC_MEC172X_NSZ)
#include "gpio_pkg_sz.h"
#elif defined(CONFIG_SOC_MEC172X_NLJ)
#include "gpio_pkg_lj.h"
#endif

#define NUM_MCHP_GPIO_PORTS	6u
#define MAX_NUM_MCHP_GPIO	(NUM_MCHP_GPIO_PORTS * 32u)

/* GPIO Control register field definitions. */

/* bits[1:0] internal pull up/down selection */
#define MCHP_GPIO_CTRL_PUD_POS		0
#define MCHP_GPIO_CTRL_PUD_MASK0	0x03u
#define MCHP_GPIO_CTRL_PUD_MASK		0x03u
#define MCHP_GPIO_CTRL_PUD_NONE		0x00u
#define MCHP_GPIO_CTRL_PUD_PU		0x01u
#define MCHP_GPIO_CTRL_PUD_PD		0x02u
/* Repeater(keeper) mode */
#define MCHP_GPIO_CTRL_PUD_RPT		0x03u

/* bits[3:2] power gating */
#define MCHP_GPIO_CTRL_PWRG_POS		2
#define MCHP_GPIO_CTRL_PWRG_MASK0	0x03u
#define MCHP_GPIO_CTRL_PWRG_VTR_IO	0
#define MCHP_GPIO_CTRL_PWRG_VCC_IO	SHLU32(1, MCHP_GPIO_CTRL_PWRG_POS)
#define MCHP_GPIO_CTRL_PWRG_OFF		SHLU32(2, MCHP_GPIO_CTRL_PWRG_POS)
#define MCHP_GPIO_CTRL_PWRG_RSVD	SHLU32(3, MCHP_GPIO_CTRL_PWRG_POS)
#define MCHP_GPIO_CTRL_PWRG_MASK	SHLU32(3, MCHP_GPIO_CTRL_PWRG_POS)

/* bits[7:4] interrupt detection mode */
#define MCHP_GPIO_CTRL_IDET_POS		4
#define MCHP_GPIO_CTRL_IDET_MASK0	0x0fu
#define MCHP_GPIO_CTRL_IDET_LVL_LO	0
#define MCHP_GPIO_CTRL_IDET_LVL_HI	SHLU32(1, MCHP_GPIO_CTRL_IDET_POS)
#define MCHP_GPIO_CTRL_IDET_DISABLE	SHLU32(4, MCHP_GPIO_CTRL_IDET_POS)
#define MCHP_GPIO_CTRL_IDET_REDGE	SHLU32(0xd, MCHP_GPIO_CTRL_IDET_POS)
#define MCHP_GPIO_CTRL_IDET_FEDGE	SHLU32(0xe, MCHP_GPIO_CTRL_IDET_POS)
#define MCHP_GPIO_CTRL_IDET_BEDGE	SHLU32(0xf, MCHP_GPIO_CTRL_IDET_POS)
#define MCHP_GPIO_CTRL_IDET_MASK	SHLU32(0xf, MCHP_GPIO_CTRL_IDET_POS)

/* bit[8] output buffer type: push-pull or open-drain */
#define MCHP_GPIO_CTRL_BUFT_POS		8
#define MCHP_GPIO_CTRL_BUFT_MASK	BIT(MCHP_GPIO_CTRL_BUFT_POS)
#define MCHP_GPIO_CTRL_BUFT_OPENDRAIN	BIT(MCHP_GPIO_CTRL_BUFT_POS)
#define MCHP_GPIO_CTRL_BUFT_PUSHPULL	0

/* bit[9] direction */
#define MCHP_GPIO_CTRL_DIR_POS		9
#define MCHP_GPIO_CTRL_DIR_MASK		BIT(MCHP_GPIO_CTRL_DIR_POS)
#define MCHP_GPIO_CTRL_DIR_OUTPUT	BIT(MCHP_GPIO_CTRL_DIR_POS)
#define MCHP_GPIO_CTRL_DIR_INPUT	0

/*
 * bit[10] Alternate output disable. Default==0(alternate output enabled)
 * GPIO output value is controlled by bit[16] of this register.
 * Set bit[10]=1 if you wish to control pin output using the parallel
 * GPIO output register bit for this pin.
 */
#define MCHP_GPIO_CTRL_AOD_POS		10
#define MCHP_GPIO_CTRL_AOD_MASK		BIT(MCHP_GPIO_CTRL_AOD_POS)
#define MCHP_GPIO_CTRL_AOD_DIS		BIT(MCHP_GPIO_CTRL_AOD_POS)

/* bit[11] GPIO function output polarity */
#define MCHP_GPIO_CTRL_POL_POS		11
#define MCHP_GPIO_CTRL_POL_INVERT	BIT(MCHP_GPIO_CTRL_POL_POS)

/* bits[14:12] pin mux (function) */
#define MCHP_GPIO_CTRL_MUX_POS		12
#define MCHP_GPIO_CTRL_MUX_MASK0	0x07u
#define MCHP_GPIO_CTRL_MUX_MASK		SHLU32(7, MCHP_GPIO_CTRL_MUX_POS)
#define MCHP_GPIO_CTRL_MUX_F0		0
#define MCHP_GPIO_CTRL_MUX_GPIO		MCHP_GPIO_CTRL_MUX_F0
#define MCHP_GPIO_CTRL_MUX_F1		SHLU32(1, MCHP_GPIO_CTRL_MUX_POS)
#define MCHP_GPIO_CTRL_MUX_F2		SHLU32(2, MCHP_GPIO_CTRL_MUX_POS)
#define MCHP_GPIO_CTRL_MUX_F3		SHLU32(3, MCHP_GPIO_CTRL_MUX_POS)
#define MCHP_GPIO_CTRL_MUX_F4		SHLU32(4, MCHP_GPIO_CTRL_MUX_POS)
#define MCHP_GPIO_CTRL_MUX_F5		SHLU32(5, MCHP_GPIO_CTRL_MUX_POS)
#define MCHP_GPIO_CTRL_MUX_F6		SHLU32(6, MCHP_GPIO_CTRL_MUX_POS)
#define MCHP_GPIO_CTRL_MUX_F7		SHLU32(7, MCHP_GPIO_CTRL_MUX_POS)
#define MCHP_GPIO_CTRL_MUX(n) SHLU32(((n) & 0x7u), MCHP_GPIO_CTRL_MUX_POS)

#define MCHP_GPIO_CTRL_MUX_GET(x) (((uint32_t)(x) >> MCHP_GPIO_CTRL_MUX_POS)\
				& MCHP_GPIO_CTRL_MUX_MASK0)

/*
 * bit[15] Disables input pad leaving output pad enabled
 * Useful for reducing power consumption of output only pins.
 */
#define MCHP_GPIO_CTRL_INPAD_DIS_POS	15
#define MCHP_GPIO_CTRL_INPAD_DIS_MASK	BIT(MCHP_GPIO_CTRL_INPAD_DIS_POS)
#define MCHP_GPIO_CTRL_INPAD_DIS	BIT(MCHP_GPIO_CTRL_INPAD_DIS_POS)

/* bit[16]: Alternate output pin value. Enabled when bit[10]==0(default) */
#define MCHP_GPIO_CTRL_OUTVAL_POS	16
#define MCHP_GPIO_CTRL_OUTV_HI		BIT(MCHP_GPIO_CTRL_OUTVAL_POS)

/* bit[24] Input pad value. Always live unless input pad is powered down */
#define MCHP_GPIO_CTRL_INPAD_VAL_POS	24
#define MCHP_GPIO_CTRL_INPAD_VAL_HI	BIT(MCHP_GPIO_CTRL_INPAD_VAL_POS)

#define MCHP_GPIO_CTRL_DRIVE_OD_HI				     \
	(MCHP_GPIO_CTRL_BUFT_OPENDRAIN + MCHP_GPIO_CTRL_DIR_OUTPUT + \
	 MCHP_GPIO_CTRL_MUX_GPIO + MCHP_GPIO_CTRL_OUTV_HI)

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
#define MCHP_GPIO_CTRL2_SLEW_POS	0
#define MCHP_GPIO_CTRL2_SLEW_MASK	0x01u
#define MCHP_GPIO_CTRL2_SLEW_SLOW	0
#define MCHP_GPIO_CTRL2_SLEW_FAST	BIT(MCHP_GPIO_CTRL2_SLEW_POS)
#define MCHP_GPIO_CTRL2_DRV_STR_POS	4
#define MCHP_GPIO_CTRL2_DRV_STR_MASK	0x30u
#define MCHP_GPIO_CTRL2_DRV_STR_2MA	0
#define MCHP_GPIO_CTRL2_DRV_STR_4MA	0x10u
#define MCHP_GPIO_CTRL2_DRV_STR_8MA	0x20u
#define MCHP_GPIO_CTRL2_DRV_STR_12MA	0x30u

/* Interfaces to any C modules */
#ifdef __cplusplus
extern "C" {
#endif

#define MCHP_GPIO_PIN2PORT(pin_id) ((uint32_t)(pin_id) >> 5)

/* Helper functions */
enum mchp_gpio_pud {
	MCHP_GPIO_NO_PUD = 0,
	MCHP_GPIO_PU_EN,
	MCHP_GPIO_PD_EN,
	MCHP_GPIO_RPT_EN,
};

enum mchp_gpio_pwrgate {
	MCHP_GPIO_PWRGT_VTR = 0,
	MCHP_GPIO_PWRGT_VCC,
	MCHP_GPIO_PWRGD_OFF,
};

enum mchp_gpio_idet {
	MCHP_GPIO_IDET_LO_LVL		= 0u,
	MCHP_GPIO_IDET_HI_LVL		= 0x01u,
	MCHP_GPIO_IDET_DIS		= 0x04u,
	MCHP_GPIO_IDET_RISING_EDGE	= 0x0du,
	MCHP_GPIO_IDET_FALLING_EDGE	= 0x0eu,
	MCHP_GPIO_IDET_BOTH_EDGES	= 0x0fu
};

enum mchp_gpio_outbuf {
	MCHP_GPIO_PUSH_PULL = 0,
	MCHP_GPIO_OPEN_DRAIN,
};

enum mchp_gpio_dir {
	MCHP_GPIO_DIR_IN = 0,
	MCHP_GPIO_DIR_OUT,
};

enum mchp_gpio_parout_en {
	MCHP_GPIO_PAROUT_DIS = 0,
	MCHP_GPIO_PAROUT_EN,
};

enum mchp_gpio_pol {
	MCHP_GPIO_POL_NORM = 0,
	MCHP_GPIO_POL_INV,
};

enum mchp_gpio_mux {
	MCHP_GPIO_MUX_GPIO = 0u,
	MCHP_GPIO_MUX_FUNC1,
	MCHP_GPIO_MUX_FUNC2,
	MCHP_GPIO_MUX_FUNC3,
	MCHP_GPIO_MUX_FUNC4,
	MCHP_GPIO_MUX_FUNC5,
	MCHP_GPIO_MUX_FUNC6,
	MCHP_GPIO_MUX_FUNC7,
	MCHP_GPIO_MUX_MAX
};

enum mchp_gpio_inpad_ctrl {
	MCHP_GPIO_INPAD_CTRL_EN = 0,
	MCHP_GPIO_INPAD_CTRL_DIS,
};

enum mchp_gpio_alt_out {
	MCHP_GPIO_ALT_OUT_LO = 0,
	MCHP_GPIO_ALT_OUT_HI,
};

enum mchp_gpio_slew {
	MCHP_GPIO_SLEW_SLOW = 0,
	MCHP_GPIO_SLEW_FAST,
};

enum mchp_gpio_drv_str {
	MCHP_GPIO_DRV_STR_2MA = 0,
	MCHP_GPIO_DRV_STR_4MA,
	MCHP_GPIO_DRV_STR_8MA,
	MCHP_GPIO_DRV_STR_12MA,
};

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _MEC172X_GPIO_H */
