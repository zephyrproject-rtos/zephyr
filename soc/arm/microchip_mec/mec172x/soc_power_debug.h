/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SOC_POWER_DEBUG_H__
#define __SOC_POWER_DEBUG_H__

/* #define SOC_SLEEP_STATE_GPIO_MARKER_DEBUG */

#ifdef SOC_SLEEP_STATE_GPIO_MARKER_DEBUG

/* Select a gpio not used. LED4 on EVB. High = ON */
#define DP_GPIO_ID		MCHP_GPIO_0241_ID

/* output drive high */
#define PM_DP_ENTER_GPIO_VAL	0x10240U
/* output drive low */
#define PM_DP_EXIT_GPIO_VAL	0x0240U

static inline void pm_dp_gpio(uint32_t gpio_ctrl_val)
{
	struct gpio_regs * const regs =
		(struct gpio_regs * const)(DT_REG_ADDR(DT_NODELABEL(gpio_000_036)));

	regs->CTRL[DP_GPIO_ID] = gpio_ctrl_val;
}

#endif

#ifdef DP_GPIO_ID
#define PM_DP_ENTER()	pm_dp_gpio(PM_DP_ENTER_GPIO_VAL)
#define PM_DP_EXIT()	pm_dp_gpio(PM_DP_EXIT_GPIO_VAL)
#else
#define PM_DP_ENTER()
#define PM_DP_EXIT()
#endif

#endif /* __SOC_POWER_DEBUG_H__ */
