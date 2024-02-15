/*
 * Copyright (c) 2018, Diego Sueiro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ccm_imx7d.h>
#include <ccm_analog_imx7d.h>
#include "soc_clk_freq.h"

#ifdef CONFIG_PWM_IMX
uint32_t get_pwm_clock_freq(PWM_Type *base)
{
	uint32_t root;
	uint32_t hz;
	uint32_t pre, post;

	switch ((uint32_t)base) {
	case PWM1_BASE:
		root = CCM_GetRootMux(CCM, CCMROOTPWM1);
		CCM_GetRootDivider(CCM, CCMROOTPWM1, &pre, &post);
		break;
	case PWM2_BASE:
		root = CCM_GetRootMux(CCM, CCMROOTPWM2);
		CCM_GetRootDivider(CCM, CCMROOTPWM2, &pre, &post);
		break;
	case PWM3_BASE:
		root = CCM_GetRootMux(CCM, CCMROOTPWM3);
		CCM_GetRootDivider(CCM, CCMROOTPWM3, &pre, &post);
		break;
	case PWM4_BASE:
		root = CCM_GetRootMux(CCM, CCMROOTPWM4);
		CCM_GetRootDivider(CCM, CCMROOTPWM4, &pre, &post);
		break;
	default:
		return 0;
	}

	switch (root) {
	case CCMROOTMUXPWMOSC24M:
		hz = 24000000U;
		break;
	case CCMROOTMUXPWMSYSPLLDIV4:
		hz = CCM_ANALOG_GetSysPllFreq(CCM_ANALOG) >> 2;
		break;
	default:
		return 0;
	}

	return hz / (pre + 1) / (post + 1);
}
#endif /* CONFIG_PWM_IMX */
