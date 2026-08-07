/*
 * Copyright (c) 2025 Realtek, SIBG-SD7
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_REALTEK_RTS5912_REG_PWM_H
#define ZEPHYR_SOC_REALTEK_RTS5912_REG_PWM_H

/*
 * @brief PWM Controller (PWM)
 */

struct pwm_regs {
	uint32_t duty;
	uint32_t div;
	uint32_t ctrl;
};

/* CTRL */
#define PWM_CTRL_CLKSRC BIT(28)
#define PWM_CTRL_INVT   BIT(29)
#define PWM_CTRL_RST    BIT(30)
#define PWM_CTRL_EN     BIT(31)

#endif /* ZEPHYR_SOC_REALTEK_RTS5912_REG_PWM_H */
