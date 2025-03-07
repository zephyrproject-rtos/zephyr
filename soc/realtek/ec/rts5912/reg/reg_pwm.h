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

typedef struct {
	volatile uint32_t DUTY;
	volatile uint32_t DIV;
	volatile uint32_t CTRL;
} PWM_Type;

/* CTRL */
#define PWM_CTRL_CLKSRC BIT(28)
#define PWM_CTRL_INVT   BIT(29)
#define PWM_CTRL_RST    BIT(30)
#define PWM_CTRL_EN     BIT(31)

#endif /* ZEPHYR_SOC_REALTEK_RTS5912_REG_PWM_H */
