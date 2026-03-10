/*
 * Copyright (c) 2025 Realtek, SIBG-SD7
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_REALTEK_RTS5912_REG_WDT_H
#define ZEPHYR_SOC_REALTEK_RTS5912_REG_WDT_H

/**
 * @brief WDT Controller (WDT)
 */

typedef struct {
	volatile uint32_t CTRL;
	const volatile uint32_t STS;
	volatile uint32_t CNT;
	volatile uint32_t DIV;
	volatile uint32_t INTEN;
} WDT_Type;

/* CTRL */
#define WDT_CTRL_EN         BIT(0)
#define WDT_CTRL_RSTEN      BIT(1)
#define WDT_CTRL_RELOAD     BIT(2)
#define WDT_CTRL_CLRRSTFLAG BIT(3)
/* STS */
#define WDT_STS_RSTFLAG     BIT(0)
/* INTEN */
#define WDT_INTEN_WDTINTEN  BIT(0)

#endif /* ZEPHYR_SOC_REALTEK_RTS5912_REG_WDT_H */
