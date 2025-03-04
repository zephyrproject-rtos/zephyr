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

	union {
		volatile uint32_t CTRL;

		struct {
			uint32_t: 28;
			volatile uint32_t CLKSRC: 1;
			volatile uint32_t INVT: 1;
			volatile uint32_t RST: 1;
			volatile uint32_t EN: 1;
		} CTRL_b;
	};
} PWM_Type;

/* CTRL */
#define PWM_CTRL_CLKSRC_Pos (28UL)
#define PWM_CTRL_CLKSRC_Msk BIT(28)
#define PWM_CTRL_INVT_Pos   (29UL)
#define PWM_CTRL_INVT_Msk   BIT(29)
#define PWM_CTRL_RST_Pos    (30UL)
#define PWM_CTRL_RST_Msk    BIT(30)
#define PWM_CTRL_EN_Pos     (31UL)
#define PWM_CTRL_EN_Msk     BIT(31)

#endif /* ZEPHYR_SOC_REALTEK_RTS5912_REG_PWM_H */
