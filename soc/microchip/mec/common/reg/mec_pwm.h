/*
 * Copyright (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MEC_PWM_H
#define _MEC_PWM_H

#include <stdint.h>
#include <stddef.h>

#define MCHP_PWM_INST_SPACING		0x10u
#define MCHP_PWM_INST_SPACING_P2	4u

/* PWM Count On register */
#define MCHP_PWM_COUNT_ON_REG_OFS	0u
#define MCHP_PWM_COUNT_ON_MASK		0xffffu

/* PWM Count Off register */
#define MCHP_PWM_COUNT_OFF_REG_OFS	4u
#define MCHP_PWM_COUNT_OFF_MASK		0xffffu

/* PWM Configuration Register */
#define MCHP_PWM_CONFIG_REG_OFS		8u
#define MCHP_PWM_CONFIG_MASK		0x7fu
/*
 * Enable and start PWM. Clearing this bit resets internal counters.
 * COUNT_ON and COUNT_OFF registers are not affected by enable bit.
 */
#define MCHP_PWM_CFG_ENABLE_POS		0
#define MCHP_PWM_CFG_ENABLE		BIT(MCHP_PWM_CFG_ENABLE_POS)
/* Clock select */
#define MCHP_PWM_CFG_CLK_SEL_POS	1u
#define MCHP_PWM_CFG_CLK_SEL_48M	0u
#define MCHP_PWM_CFG_CLK_SEL_100K	BIT(MCHP_PWM_CFG_CLK_SEL_POS)
/*
 * ON state polarity.
 * Default ON state is High.
 */
#define MCHP_PWM_CFG_ON_POL_POS		2u
#define MCHP_PWM_CFG_ON_POL_HI		0u
#define MCHP_PWM_CFG_ON_POL_LO		BIT(MCHP_PWM_CFG_ON_POL_POS)
/*
 * Clock pre-divider
 * Clock divider value = pre-divider + 1
 */
#define MCHP_PWM_CFG_CLK_PRE_DIV_POS	3u
#define MCHP_PWM_CFG_CLK_PRE_DIV_MASK0	0x0fU
#define MCHP_PWM_CFG_CLK_PRE_DIV_MASK \
	SHLU32(0x0fu, MCHP_PWM_CFG_CLK_PRE_DIV_POS)

#define MCHP_PWM_CFG_CLK_PRE_DIV(n)		     \
	SHLU32((n) & MCHP_PWM_CFG_CLK_PRE_DIV_MASK0, \
	       MCHP_PWM_CFG_CLK_PRE_DIV_POS)

/* PWM input frequencies selected in configuration register. */
#define MCHP_PWM_INPUT_FREQ_HI	48000000u
#define MCHP_PWM_INPUT_FREQ_LO	100000u

/*
 * PWM Frequency =
 * (1 / (pre_div + 1)) * PWM_INPUT_FREQ / ((COUNT_ON+1) + (COUNT_OFF+1))
 *
 * PWM Duty Cycle =
 * (COUNT_ON+1) / ((COUNT_ON+1) + (COUNT_OFF + 1))
 */

/** @brief PWM controller */
struct pwm_regs {
	volatile uint32_t COUNT_ON;
	volatile uint32_t COUNT_OFF;
	volatile uint32_t CONFIG;
};

#endif	/* #ifndef _MEC_PWM_H */
