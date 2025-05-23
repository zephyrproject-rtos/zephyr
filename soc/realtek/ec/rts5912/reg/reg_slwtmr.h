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

struct slwtmr_type {
	uint32_t ldcnt;
	uint32_t cnt;
	uint32_t ctrl;
	uint32_t insts;
};

/* CTRL */
#define SLWTMR_CTRL_EN BIT(0)

#define SLWTMR_CTRL_MDSELS_ONESHOT 0
#define SLWTMR_CTRL_MDSELS_PERIOD  BIT(1)

#define SLWTMR_CTRL_INTEN_EN BIT(2)
/* INTSTS */
#define SLWTMR_INTSTS_STS    BIT(0)

#endif /* ZEPHYR_SOC_REALTEK_RTS5912_REG_SLWTMR_H */
