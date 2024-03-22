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
		root = CCM_GetRootMux(CCM, ccmRootPwm1);
		CCM_GetRootDivider(CCM, ccmRootPwm1, &pre, &post);
		break;
	case PWM2_BASE:
		root = CCM_GetRootMux(CCM, ccmRootPwm2);
		CCM_GetRootDivider(CCM, ccmRootPwm2, &pre, &post);
		break;
	case PWM3_BASE:
		root = CCM_GetRootMux(CCM, ccmRootPwm3);
		CCM_GetRootDivider(CCM, ccmRootPwm3, &pre, &post);
		break;
	case PWM4_BASE:
		root = CCM_GetRootMux(CCM, ccmRootPwm4);
		CCM_GetRootDivider(CCM, ccmRootPwm4, &pre, &post);
		break;
	default:
		return 0;
	}

	switch (root) {
	case ccmRootmuxPwmOsc24m:
		hz = 24000000U;
		break;
	case ccmRootmuxPwmSysPllDiv4:
		hz = CCM_ANALOG_GetSysPllFreq(CCM_ANALOG) >> 2;
		break;
	default:
		return 0;
	}

	return hz / (pre + 1) / (post + 1);
}
#endif /* CONFIG_PWM_IMX */
