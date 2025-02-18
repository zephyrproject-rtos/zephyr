/*
 * Copyright (c) 2025 Realtek, SIBG-SD7
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_REALTEK_RTS5912_REG_MBX_H
#define ZEPHYR_SOC_REALTEK_RTS5912_REG_MBX_H

typedef struct {
	uint32_t STS;
	uint32_t DATA[48];
	uint32_t INTCTRL;
	uint32_t IRQNUM;
	uint32_t INTSTS;
} MBX_Type;

/* STS */
#define MBX_STS_STS_Pos       (0UL)
#define MBX_STS_STS_Msk       GENMASK(7, 0)
/* INTCTRL */
#define MBX_INTCTRL_IRQEN_Pos (0UL)
#define MBX_INTCTRL_IRQEN_Msk BIT(MBX_INTCTRL_IRQEN_Pos)
#define MBX_INTCTRL_TGLV_Pos  (1UL)
#define MBX_INTCTRL_TGLV_Msk  BIT(MBX_INTCTRL_TGLV_Pos)
#define MBX_INTCTRL_INTEN_Pos (2UL)
#define MBX_INTCTRL_INTEN_Msk BIT(MBX_INTCTRL_INTEN_Pos)
/* IRQNUM */
#define MBX_IRQNUM_NUM_Pos    (0UL)
#define MBX_IRQNUM_NUM_Msk    GENMASK(7, 0)
/* INTSTS */
#define MBX_INTSTS_STS_Pos    (0UL)
#define MBX_INTSTS_STS_Msk    BIT(MBX_INTSTS_STS_Pos)
#define MBX_INTSTS_CLR_Pos    (1UL)
#define MBX_INTSTS_CLR_Msk    BIT(MBX_INTSTS_CLR_Pos)

#endif /* ZEPHYR_SOC_REALTEK_RTS5912_REG_MBX_H */
