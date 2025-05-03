/*
 * Copyright (c) 2025 Realtek, SIBG-SD7
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_REALTEK_RTS5912_REG_PORT80_H
#define ZEPHYR_SOC_REALTEK_RTS5912_REG_PORT80_H

struct port80_reg {
	const uint32_t STS;
	uint32_t CFG;
	uint32_t INTEN;
	uint32_t DATA;
	uint32_t ADDR;
};

/* STS */
#define PORT80_STS_FIFOEM   BIT(0)
#define PORT80_STS_FIFOFUL  BIT(1)
#define PORT80_STS_FIFOOVRN BIT(2)

/* CFG */
#define PORT80_CFG_CLRFLG   BIT(0)
#define PORT80_CFG_THRE     GENMASK(2, 1)
#define PORT80_CFG_THREEN   BIT(7)
#define PORT80_CFG_UARTPASS BIT(8)

/* INTEN */
#define PORT80_INTEN_THREINTEN BIT(0)

/* DATA */
#define PORT80_DATA_DATA GENMASK(7, 0)

/* ADDR */
#define PORT80_ADDR_ADDR GENMASK(7, 0)

#endif /* ZEPHYR_SOC_REALTEK_RTS5912_REG_PORT80_H */
