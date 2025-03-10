/*
 * Copyright (c) 2025 Realtek, SIBG-SD7
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_REALTEK_RTS5912_REG_SLWTMR_H
#define ZEPHYR_SOC_REALTEK_RTS5912_REG_SLWTMR_H

/**
 * @brief SLOW Timer Controller (SLWTMR)
 */

typedef struct {
	volatile uint32_t LDCNT;
	volatile uint32_t CNT;

	union {
		volatile uint32_t CTRL;

		struct {
			volatile uint32_t EN: 1;
			volatile uint32_t MDSEL: 1;
			volatile uint32_t INTEN: 1;
			uint32_t: 29;
		} CTRL_b;
	};

	union {
		volatile uint32_t INTSTS;

		struct {
			volatile uint32_t STS: 1;
			uint32_t: 31;
		} INTSTS_b;
	};
} SLWTMR_Type;

/* CTRL */
#define SLWTMR_CTRL_EN     BIT(0)
#define SLWTMR_CTRL_EN_Pos (0UL)

#define SLWTMR_CTRL_MDSELS_ONESHOT 0
#define SLWTMR_CTRL_MDSELS_PERIOD  BIT(1)
#define SLWTMR_CTRL_MDSEL_Pos      (1UL)

#define SLWTMR_CTRL_INTEN_EN  BIT(2)
#define SLWTMR_CTRL_INTEN_Pos (2UL)
/* INTSTS */
#define SLWTMR_INTSTS_STS     BIT(0)
#define SLWTMR_INTSTS_STS_Pos (0UL)

#endif /* ZEPHYR_SOC_REALTEK_RTS5912_REG_SLWTMR_H */
