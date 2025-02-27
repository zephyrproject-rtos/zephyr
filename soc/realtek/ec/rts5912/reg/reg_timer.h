/*
 * Copyright (c) 2025 Realtek, SIBG-SD7
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_REALTEK_RTS5912_REG_TMRER_H
#define ZEPHYR_SOC_REALTEK_RTS5912_REG_TMRER_H

/**
 * @brief 32-bit Timer Controller (TMR)
 */

struct timer32_type {
	uint32_t ldcnt;
	uint32_t cnt;
	uint32_t ctrl;
	uint32_t intclr;
	uint32_t intsts;
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

#endif /* ZEPHYR_SOC_REALTEK_RTS5912_REG_TMRER_H */
