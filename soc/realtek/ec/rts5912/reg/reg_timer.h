/*
 * Copyright (c) 2025 Realtek, SIBG-SD7
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_REALTEK_RTS5912_REG_TMR32_H
#define ZEPHYR_SOC_REALTEK_RTS5912_REG_TMR32_H

/**
 * @brief 32-bit Timer Controller (TMR)
 */

struct TIMER32_Type {
	volatile uint32_t LDCNT;
	volatile uint32_t CNT;
	volatile uint32_t CTRL;
	volatile uint32_t INTCLR;
	volatile uint32_t INTSTS;
};

/* CTRL */
#define TIMER32_CTRL_EN BIT(0)

#define TIMER32_CTRL_MDSELS_ONESHOT 0
#define TIMER32_CTRL_MDSELS_PERIOD  BIT(1)

#define TIMER32_CTRL_INTEN_DIS BIT(2)
/* INTCLR */
#define TIMER32_INTCLR_INTCLR  BIT(0)
/* INTSTS */
#define TIMER32_INTSTS_STS     (0UL)

#endif /* ZEPHYR_SOC_REALTEK_RTS5912_REG_TMR32_H */
