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

typedef struct TIMER32_Type {
	volatile uint32_t LDCNT;
	volatile uint32_t CNT;

	union {
		volatile uint32_t CTRL;

		struct {
			volatile uint32_t EN: 1;
			volatile uint32_t MDSEL: 1;
			volatile uint32_t INTMSK: 1;
			uint32_t: 29;
		} CTRL_b;
	};
	volatile uint32_t INTCLR;

	union {
		volatile uint32_t INTSTS;

		struct {
			volatile uint32_t STS: 1;
			uint32_t: 31;
		} INTSTS_b;
	};
} TIMER32_Type;

/* CTRL */
#define TIMER32_CTRL_EN     BIT(0)
#define TIMER32_CTRL_EN_Pos (0UL)

#define TIMER32_CTRL_MDSELS_ONESHOT 0
#define TIMER32_CTRL_MDSELS_PERIOD  BIT(1)
#define TIMER32_CTRL_MDSEL_Pos      (1UL)

#define TIMER32_CTRL_INTEN_DIS    BIT(2)
#define TIMER32_CTRL_INTEN_Pos    (2UL)
/* INTCLR */
#define TIMER32_INTCLR_INTCLR     BIT(0)
#define TIMER32_INTCLR_INTCLR_Pos (0UL)
/* INTSTS */
#define TIMER32_INTSTS_STS_Pos    (0UL)

#endif /* ZEPHYR_SOC_REALTEK_RTS5912_REG_TMR32_H */
