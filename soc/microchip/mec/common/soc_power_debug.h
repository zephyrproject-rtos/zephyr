/*
 * Copyright (c) 2024 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SOC_POWER_DEBUG_H__
#define __SOC_POWER_DEBUG_H__

/* #define SOC_SLEEP_STATE_GPIO_MARKER_DEBUG */

#ifdef SOC_SLEEP_STATE_GPIO_MARKER_DEBUG

/* GPIO selection is via Device Tree special node zephyr,user
 *
 * ZEPHYR_USER_NODE DT_PATH(zephyr_user)
 *
 * produces a node_id
 * DT_GPIO_CTLR_BY_IDX(ZEPHYR_USER_NODE, pm_gpios, 0)
 *
 * DT_GPIO_CTLR_BY_IDX(ZEPHYR_USER_NODE, pm_gpio, 0) -> DT_NODELABEL(gpio_xxx_yyy)
 * DT_GPIO_PIN_BY_IDX(ZEPHYR_USER_NODE, pm_gpios, 0)
 *
 */

#define PM_DP_GPIO_NODE_ID	DT_GPIO_CTLR_BY_IDX(ZEPHYR_USER_NODE, pm_gpio, 0)
#define PM_DP_GPIO_PIN		DT_GPIO_PIN_BY_IDX(ZEPHYR_USER_NODE, pm_gpios, 0)

/* Select a gpio not used. LED4 on EVB. High = ON */
#define DP_GPIO_ID		MCHP_GPIO_0241_ID

/* output drive high */
#define PM_DP_ENTER_GPIO_VAL	0x10240U
/* output drive low */
#define PM_DP_EXIT_GPIO_VAL	0x0240U

static inline void pm_dp_gpio(uint32_t gpio_ctrl_val)
{
	struct gpio_regs * const regs =
		(struct gpio_regs * const)(DT_REG_ADDR(PM_DP_GPIO_NODE_ID));

	regs->CTRL[PM_DP_GPIO_PIN] = gpio_ctrl_val;
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
