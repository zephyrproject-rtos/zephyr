/*
 * Copyright (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MEC172X_GPIO_H
#define _MEC172X_GPIO_H

#include <stdint.h>
#include <stddef.h>

#define NUM_MCHP_GPIO_PORTS	6u
#define MAX_NUM_MCHP_GPIO	(NUM_MCHP_GPIO_PORTS * 32u)

#define MCHP_GPIO_CTRL_OFS	0u
#define MCHP_GPIO_PARIN_OFS	0x0300u
#define MCHP_GPIO_PAROUT_OFS	0x0380u
#define MCHP_GPIO_LOCK_OFS	0x03e8u
#define MCHP_GPIO_CTRL2_OFS	0x0500u

/* MEC172XH-B0-SZ (144-pin) */
#define MCHP_GPIO_PORT_A_BITMAP 0x7FFFFF9Du /* GPIO_0000 - 0036  GIRQ11 */
#define MCHP_GPIO_PORT_B_BITMAP 0x7FFFFFFDu /* GPIO_0040 - 0076  GIRQ10 */
#define MCHP_GPIO_PORT_C_BITMAP 0x07FFFCF7u /* GPIO_0100 - 0136  GIRQ09 */
#define MCHP_GPIO_PORT_D_BITMAP 0x272EFFFFu /* GPIO_0140 - 0176  GIRQ08 */
#define MCHP_GPIO_PORT_E_BITMAP 0x00DE00FFu /* GPIO_0200 - 0236  GIRQ12 */
#define MCHP_GPIO_PORT_F_BITMAP 0x0000397Fu /* GPIO_0240 - 0276  GIRQ26 */

#define MCHP_GPIO_PORT_A_DRVSTR_BITMAP	0x7FFFFF9Du
#define MCHP_GPIO_PORT_B_DRVSTR_BITMAP	0x7FFFFFFDu
#define MCHP_GPIO_PORT_C_DRVSTR_BITMAP	0x07FFFCF7u
#define MCHP_GPIO_PORT_D_DRVSTR_BITMAP	0x272EFFFFu
#define MCHP_GPIO_PORT_E_DRVSTR_BITMAP	0x00DE00FFu
#define MCHP_GPIO_PORT_F_DRVSTR_BITMAP	0x0000397Fu

/* 32-bit Control register */
#define MCHP_GPIO_CTRL_MASK		0x0101ffffu
/* bits[15:0] of Control register */
#define MCHP_GPIO_CTRL_CFG_MASK		0xffffu

/* Disable interrupt detect and pad */
#define MCHP_GPIO_CTRL_DIS_PIN		0x8040u

#define MCHP_GPIO_CTRL_DFLT		0x8040u
#define MCHP_GPIO_CTRL_DFLT_MASK	0xffffu

#define GPIO000_CTRL_DFLT	0x1040u
#define GPIO062_CTRL_DFLT	0x8240u
#define GPIO116_CTRL_DFLT	0x0041u
#define GPIO161_CTRL_DFLT	0x1040u
#define GPIO162_CTRL_DFLT	0x1040u
#define GPIO170_CTRL_DFLT	0x0041u
#define GPIO234_CTRL_DFLT	0x1040u

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
#define MCHP_GPIO_CTRL2_OFFSET		0x0500u
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

/* GPIO pin numbers SZ (144-pin) package */
enum mec_gpio_idx {
	MCHP_GPIO_0000_ID = 0,
	MCHP_GPIO_0002_ID = 2,
	MCHP_GPIO_0003_ID,
	MCHP_GPIO_0004_ID,
	MCHP_GPIO_0007_ID = 7,
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
	MCHP_GPIO_0036_ID,
	MCHP_GPIO_0040_ID = 32,
	MCHP_GPIO_0042_ID = 34,
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
	MCHP_GPIO_0073_ID,
	MCHP_GPIO_0074_ID,
	MCHP_GPIO_0075_ID,
	MCHP_GPIO_0076_ID,
	MCHP_GPIO_0100_ID = 64,
	MCHP_GPIO_0101_ID,
	MCHP_GPIO_0102_ID,
	MCHP_GPIO_0104_ID = 68,
	MCHP_GPIO_0105_ID,
	MCHP_GPIO_0106_ID,
	MCHP_GPIO_0107_ID,
	MCHP_GPIO_0112_ID = 74,
	MCHP_GPIO_0113_ID,
	MCHP_GPIO_0114_ID,
	MCHP_GPIO_0115_ID,
	MCHP_GPIO_0116_ID,
	MCHP_GPIO_0117_ID,
	MCHP_GPIO_0120_ID = 80,
	MCHP_GPIO_0121_ID,
	MCHP_GPIO_0122_ID,
	MCHP_GPIO_0123_ID,
	MCHP_GPIO_0124_ID,
	MCHP_GPIO_0125_ID,
	MCHP_GPIO_0126_ID,
	MCHP_GPIO_0127_ID,
	MCHP_GPIO_0130_ID,
	MCHP_GPIO_0131_ID,
	MCHP_GPIO_0132_ID,
	MCHP_GPIO_0140_ID = 96,
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
	MCHP_GPIO_0161_ID = 113,
	MCHP_GPIO_0162_ID,
	MCHP_GPIO_0165_ID = 117,
	MCHP_GPIO_0170_ID = 120,
	MCHP_GPIO_0171_ID,
	MCHP_GPIO_0175_ID = 125,
	MCHP_GPIO_0200_ID = 128,
	MCHP_GPIO_0201_ID,
	MCHP_GPIO_0202_ID,
	MCHP_GPIO_0203_ID,
	MCHP_GPIO_0204_ID,
	MCHP_GPIO_0205_ID,
	MCHP_GPIO_0206_ID,
	MCHP_GPIO_0207_ID,
	MCHP_GPIO_0221_ID = 145,
	MCHP_GPIO_0222_ID,
	MCHP_GPIO_0223_ID,
	MCHP_GPIO_0224_ID,
	MCHP_GPIO_0226_ID = 150,
	MCHP_GPIO_0227_ID,
	MCHP_GPIO_0240_ID = 160,
	MCHP_GPIO_0241_ID,
	MCHP_GPIO_0242_ID,
	MCHP_GPIO_0243_ID,
	MCHP_GPIO_0244_ID,
	MCHP_GPIO_0245_ID,
	MCHP_GPIO_0246_ID,
	MCHP_GPIO_0254_ID = 172,
	MCHP_GPIO_0255_ID,
	MCHP_GPIO_MAX_ID
};

#define MCHP_GPIO_PIN2PORT(pin_id) ((uint32_t)(pin_id) >> 5)

#define MAX_MCHP_GPIO_BANK	6u
#define MCHP_GPIO_LOCK5_IDX	0u
#define MCHP_GPIO_LOCK4_IDX	1u
#define MCHP_GPIO_LOCK3_IDX	2u
#define MCHP_GPIO_LOCK2_IDX	3u
#define MCHP_GPIO_LOCK1_IDX	4u
#define MCHP_GPIO_LOCK0_IDX	5u
#define MCHP_GPIO_LOCK_MAX_IDX	6u

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

/** @brief GPIO control registers by pin name */
struct gpio_ctrl_regs {
	volatile uint32_t CTRL_0000;
	uint32_t RSVD1[1];
	volatile uint32_t CTRL_0002;
	volatile uint32_t CTRL_0003;
	volatile uint32_t CTRL_0004;
	uint32_t RSVD2[2];
	volatile uint32_t CTRL_0007;
	volatile uint32_t CTRL_0010;
	volatile uint32_t CTRL_0011;
	volatile uint32_t CTRL_0012;
	volatile uint32_t CTRL_0013;
	volatile uint32_t CTRL_0014;
	volatile uint32_t CTRL_0015;
	volatile uint32_t CTRL_0016;
	volatile uint32_t CTRL_0017;
	volatile uint32_t CTRL_0020;
	volatile uint32_t CTRL_0021;
	volatile uint32_t CTRL_0022;
	volatile uint32_t CTRL_0023;
	volatile uint32_t CTRL_0024;
	volatile uint32_t CTRL_0025;
	volatile uint32_t CTRL_0026;
	volatile uint32_t CTRL_0027;
	volatile uint32_t CTRL_0030;
	volatile uint32_t CTRL_0031;
	volatile uint32_t CTRL_0032;
	volatile uint32_t CTRL_0033;
	volatile uint32_t CTRL_0034;
	volatile uint32_t CTRL_0035;
	volatile uint32_t CTRL_0036;
	uint32_t RSVD3[1];
	volatile uint32_t CTRL_0040;
	uint32_t RSVD4[1];
	volatile uint32_t CTRL_0042;
	volatile uint32_t CTRL_0043;
	volatile uint32_t CTRL_0044;
	volatile uint32_t CTRL_0045;
	volatile uint32_t CTRL_0046;
	volatile uint32_t CTRL_0047;
	volatile uint32_t CTRL_0050;
	volatile uint32_t CTRL_0051;
	volatile uint32_t CTRL_0052;
	volatile uint32_t CTRL_0053;
	volatile uint32_t CTRL_0054;
	volatile uint32_t CTRL_0055;
	volatile uint32_t CTRL_0056;
	volatile uint32_t CTRL_0057;
	volatile uint32_t CTRL_0060;
	volatile uint32_t CTRL_0061;
	volatile uint32_t CTRL_0062;
	volatile uint32_t CTRL_0063;
	volatile uint32_t CTRL_0064;
	volatile uint32_t CTRL_0065;
	volatile uint32_t CTRL_0066;
	volatile uint32_t CTRL_0067;
	volatile uint32_t CTRL_0070;
	volatile uint32_t CTRL_0071;
	volatile uint32_t CTRL_0072;
	volatile uint32_t CTRL_0073;
	volatile uint32_t CTRL_0074;
	volatile uint32_t CTRL_0075;
	volatile uint32_t CTRL_0076;
	uint32_t RSVD5[1];
	volatile uint32_t CTRL_0100;
	volatile uint32_t CTRL_0101;
	volatile uint32_t CTRL_0102;
	uint32_t RSVD6[1];
	volatile uint32_t CTRL_0104;
	volatile uint32_t CTRL_0105;
	volatile uint32_t CTRL_0106;
	volatile uint32_t CTRL_0107;
	uint32_t RSVD7[2];
	volatile uint32_t CTRL_0112;
	volatile uint32_t CTRL_0113;
	volatile uint32_t CTRL_0114;
	volatile uint32_t CTRL_0115;
	volatile uint32_t CTRL_0116;
	volatile uint32_t CTRL_0117;
	volatile uint32_t CTRL_0120;
	volatile uint32_t CTRL_0121;
	volatile uint32_t CTRL_0122;
	volatile uint32_t CTRL_0123;
	volatile uint32_t CTRL_0124;
	volatile uint32_t CTRL_0125;
	volatile uint32_t CTRL_0126;
	volatile uint32_t CTRL_0127;
	volatile uint32_t CTRL_0130;
	volatile uint32_t CTRL_0131;
	volatile uint32_t CTRL_0132;
	uint32_t RSVD9[5];
	volatile uint32_t CTRL_0140;
	volatile uint32_t CTRL_0141;
	volatile uint32_t CTRL_0142;
	volatile uint32_t CTRL_0143;
	volatile uint32_t CTRL_0144;
	volatile uint32_t CTRL_0145;
	volatile uint32_t CTRL_0146;
	volatile uint32_t CTRL_0147;
	volatile uint32_t CTRL_0150;
	volatile uint32_t CTRL_0151;
	volatile uint32_t CTRL_0152;
	volatile uint32_t CTRL_0153;
	volatile uint32_t CTRL_0154;
	volatile uint32_t CTRL_0155;
	volatile uint32_t CTRL_0156;
	volatile uint32_t CTRL_0157;
	uint32_t RSVD10[1];
	volatile uint32_t CTRL_0161;
	volatile uint32_t CTRL_0162;
	uint32_t RSVD11[2];
	volatile uint32_t CTRL_0165;
	uint32_t RSVD12[2];
	volatile uint32_t CTRL_0170;
	volatile uint32_t CTRL_0171;
	uint32_t RSVD13[3];
	volatile uint32_t CTRL_0175;
	uint32_t RSVD14[2];
	volatile uint32_t CTRL_0200;
	volatile uint32_t CTRL_0201;
	volatile uint32_t CTRL_0202;
	volatile uint32_t CTRL_0203;
	volatile uint32_t CTRL_0204;
	volatile uint32_t CTRL_0205;
	volatile uint32_t CTRL_0206;
	volatile uint32_t CTRL_0207;
	uint32_t RSVD15[9];
	volatile uint32_t CTRL_0221;
	volatile uint32_t CTRL_0222;
	volatile uint32_t CTRL_0223;
	volatile uint32_t CTRL_0224;
	uint32_t RSVD16[1];
	volatile uint32_t CTRL_0226;
	volatile uint32_t CTRL_0227;
	uint32_t RSVD17[8];
	volatile uint32_t CTRL_0240;
	volatile uint32_t CTRL_0241;
	volatile uint32_t CTRL_0242;
	volatile uint32_t CTRL_0243;
	volatile uint32_t CTRL_0244;
	volatile uint32_t CTRL_0245;
	volatile uint32_t CTRL_0246;
	uint32_t RSVD18[5];
	volatile uint32_t CTRL_0254;
	volatile uint32_t CTRL_0255;
};

/** @brief GPIO Control 2 registers by pin name */
struct gpio_ctrl2_regs {
	volatile uint32_t CTRL2_0000;
	uint32_t RSVD1[1];
	volatile uint32_t CTRL2_0002;
	volatile uint32_t CTRL2_0003;
	volatile uint32_t CTRL2_0004;
	uint32_t RSVD2[2];
	volatile uint32_t CTRL2_0007;
	volatile uint32_t CTRL2_0010;
	volatile uint32_t CTRL2_0011;
	volatile uint32_t CTRL2_0012;
	volatile uint32_t CTRL2_0013;
	volatile uint32_t CTRL2_0014;
	volatile uint32_t CTRL2_0015;
	volatile uint32_t CTRL2_0016;
	volatile uint32_t CTRL2_0017;
	volatile uint32_t CTRL2_0020;
	volatile uint32_t CTRL2_0021;
	volatile uint32_t CTRL2_0022;
	volatile uint32_t CTRL2_0023;
	volatile uint32_t CTRL2_0024;
	volatile uint32_t CTRL2_0025;
	volatile uint32_t CTRL2_0026;
	volatile uint32_t CTRL2_0027;
	volatile uint32_t CTRL2_0030;
	volatile uint32_t CTRL2_0031;
	volatile uint32_t CTRL2_0032;
	volatile uint32_t CTRL2_0033;
	volatile uint32_t CTRL2_0034;
	volatile uint32_t CTRL2_0035;
	volatile uint32_t CTRL2_0036;
	uint32_t RSVD3[1];
	volatile uint32_t CTRL2_0040;
	uint32_t RSVD4[1];
	volatile uint32_t CTRL2_0042;
	volatile uint32_t CTRL2_0043;
	volatile uint32_t CTRL2_0044;
	volatile uint32_t CTRL2_0045;
	volatile uint32_t CTRL2_0046;
	volatile uint32_t CTRL2_0047;
	volatile uint32_t CTRL2_0050;
	volatile uint32_t CTRL2_0051;
	volatile uint32_t CTRL2_0052;
	volatile uint32_t CTRL2_0053;
	volatile uint32_t CTRL2_0054;
	volatile uint32_t CTRL2_0055;
	volatile uint32_t CTRL2_0056;
	volatile uint32_t CTRL2_0057;
	volatile uint32_t CTRL2_0060;
	volatile uint32_t CTRL2_0061;
	volatile uint32_t CTRL2_0062;
	volatile uint32_t CTRL2_0063;
	volatile uint32_t CTRL2_0064;
	volatile uint32_t CTRL2_0065;
	volatile uint32_t CTRL2_0066;
	volatile uint32_t CTRL2_0067;
	volatile uint32_t CTRL2_0070;
	volatile uint32_t CTRL2_0071;
	volatile uint32_t CTRL2_0072;
	volatile uint32_t CTRL2_0073;
	volatile uint32_t CTRL2_0074;
	volatile uint32_t CTRL2_0075;
	volatile uint32_t CTRL2_0076;
	uint32_t RSVD5[1];
	volatile uint32_t CTRL2_0100;
	volatile uint32_t CTRL2_0101;
	volatile uint32_t CTRL2_0102;
	uint32_t RSVD6[1];
	volatile uint32_t CTRL2_0104;
	volatile uint32_t CTRL2_0105;
	volatile uint32_t CTRL2_0106;
	volatile uint32_t CTRL2_0107;
	uint32_t RSVD7[2];
	volatile uint32_t CTRL2_0112;
	volatile uint32_t CTRL2_0113;
	volatile uint32_t CTRL2_0114;
	volatile uint32_t CTRL2_0115;
	volatile uint32_t CTRL2_0116;
	volatile uint32_t CTRL2_0117;
	volatile uint32_t CTRL2_0120;
	volatile uint32_t CTRL2_0121;
	volatile uint32_t CTRL2_0122;
	volatile uint32_t CTRL2_0123;
	volatile uint32_t CTRL2_0124;
	volatile uint32_t CTRL2_0125;
	volatile uint32_t CTRL2_0126;
	volatile uint32_t CTRL2_0127;
	volatile uint32_t CTRL2_0130;
	volatile uint32_t CTRL2_0131;
	volatile uint32_t CTRL2_0132;
	uint32_t RSVD9[5];
	volatile uint32_t CTRL2_0140;
	volatile uint32_t CTRL2_0141;
	volatile uint32_t CTRL2_0142;
	volatile uint32_t CTRL2_0143;
	volatile uint32_t CTRL2_0144;
	volatile uint32_t CTRL2_0145;
	volatile uint32_t CTRL2_0146;
	volatile uint32_t CTRL2_0147;
	volatile uint32_t CTRL2_0150;
	volatile uint32_t CTRL2_0151;
	volatile uint32_t CTRL2_0152;
	volatile uint32_t CTRL2_0153;
	volatile uint32_t CTRL2_0154;
	volatile uint32_t CTRL2_0155;
	volatile uint32_t CTRL2_0156;
	volatile uint32_t CTRL2_0157;
	uint32_t RSVD10[1];
	volatile uint32_t CTRL2_0161;
	volatile uint32_t CTRL2_0162;
	uint32_t RSVD11[2];
	volatile uint32_t CTRL2_0165;
	uint32_t RSVD12[2];
	volatile uint32_t CTRL2_0170;
	volatile uint32_t CTRL2_0171;
	uint32_t RSVD13[3];
	volatile uint32_t CTRL2_0175;
	uint32_t RSVD14[2];
	volatile uint32_t CTRL2_0200;
	volatile uint32_t CTRL2_0201;
	volatile uint32_t CTRL2_0202;
	volatile uint32_t CTRL2_0203;
	volatile uint32_t CTRL2_0204;
	volatile uint32_t CTRL2_0205;
	volatile uint32_t CTRL2_0206;
	volatile uint32_t CTRL2_0207;
	uint32_t RSVD15[9];
	volatile uint32_t CTRL2_0221;
	volatile uint32_t CTRL2_0222;
	volatile uint32_t CTRL2_0223;
	volatile uint32_t CTRL2_0224;
	uint32_t RSVD16[1];
	volatile uint32_t CTRL2_0226;
	volatile uint32_t CTRL2_0227;
	uint32_t RSVD17[8];
	volatile uint32_t CTRL2_0240;
	volatile uint32_t CTRL2_0241;
	volatile uint32_t CTRL2_0242;
	volatile uint32_t CTRL2_0243;
	volatile uint32_t CTRL2_0244;
	volatile uint32_t CTRL2_0245;
	volatile uint32_t CTRL2_0246;
	uint32_t RSVD18[5];
	volatile uint32_t CTRL2_0254;
	volatile uint32_t CTRL2_0255;
};

/** @brief GPIO Parallel Input register. 32 GPIO pins per bank */
struct gpio_parin_regs {
	volatile uint32_t PARIN0;
	volatile uint32_t PARIN1;
	volatile uint32_t PARIN2;
	volatile uint32_t PARIN3;
	volatile uint32_t PARIN4;
	volatile uint32_t PARIN5;
};

/** @brief GPIO Parallel Output register. 32 GPIO pins per bank */
struct gpio_parout_regs {
	volatile uint32_t PAROUT0;
	volatile uint32_t PAROUT1;
	volatile uint32_t PAROUT2;
	volatile uint32_t PAROUT3;
	volatile uint32_t PAROUT4;
	volatile uint32_t PAROUT5;
};

/** @brief GPIO Lock registers. 32 GPIO pins per bank. Write-once bits */
struct gpio_lock_regs {
	volatile uint32_t LOCK5;
	volatile uint32_t LOCK4;
	volatile uint32_t LOCK3;
	volatile uint32_t LOCK2;
	volatile uint32_t LOCK1;
	volatile uint32_t LOCK0;
};

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _MEC172X_GPIO_H */
