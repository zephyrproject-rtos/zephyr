/*
 * Copyright (c) 2025 Realtek, SIBG-SD7
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_REALTEK_RTS5912_REG_PORT80_H
#define ZEPHYR_SOC_REALTEK_RTS5912_REG_PORT80_H

typedef struct {
	const volatile uint32_t STS;
	volatile uint32_t CFG;
	volatile uint32_t INTEN;
	volatile uint32_t DATA;
	volatile uint32_t ADDR;
} PORT80_Type;

/* STS */
#define PORT80_STS_FIFOEM_Pos      (0UL)
#define PORT80_STS_FIFOEM_Msk      BIT(PORT80_STS_FIFOEM_Pos)
#define PORT80_STS_FIFOFUL_Pos     (1UL)
#define PORT80_STS_FIFOFUL_Msk     BIT(PORT80_STS_FIFOFUL_Pos)
#define PORT80_STS_FIFOOVRN_Pos    (2UL)
#define PORT80_STS_FIFOOVRN_Msk    BIT(PORT80_STS_FIFOOVRN_Pos)
/* CFG */
#define PORT80_CFG_CLRFLG_Pos      (0UL)
#define PORT80_CFG_CLRFLG_Msk      BIT(PORT80_CFG_CLRFLG_Pos)
#define PORT80_CFG_THRE_Pos        (1UL)
#define PORT80_CFG_THRE_Msk        GENMASK(2, 1)
#define PORT80_CFG_THREEN_Pos      (7UL)
#define PORT80_CFG_THREEN_Msk      BIT(PORT80_CFG_THREEN_Pos)
#define PORT80_CFG_UARTPASS_Pos    (8UL)
#define PORT80_CFG_UARTPASS_Msk    BIT(PORT80_CFG_UARTPASS_Pos)
/* INTEN */
#define PORT80_INTEN_THREINTEN_Pos (0UL)
#define PORT80_INTEN_THREINTEN_Msk BIT(PORT80_INTEN_THREINTEN_Pos)
/* DATA */
#define PORT80_DATA_DATA_Pos       (0UL)
#define PORT80_DATA_DATA_Msk       GENMASK(7, 0)
/* ADDR */
#define PORT80_ADDR_ADDR_Pos       (0UL)
#define PORT80_ADDR_ADDR_Msk       GENMASK(7, 0)

#endif /* ZEPHYR_SOC_REALTEK_RTS5912_REG_PORT80_H */
