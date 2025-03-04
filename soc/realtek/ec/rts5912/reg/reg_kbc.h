/*
 * Copyright (c) 2025 Realtek, SIBG-SD7
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_REALTEK_RTS5912_REG_KBC_H
#define ZEPHYR_SOC_REALTEK_RTS5912_REG_KBC_H

typedef struct {
	volatile uint32_t STS;
	volatile uint32_t IB;
	volatile uint32_t OB;
	volatile uint32_t PTADDR;
	volatile uint32_t INTEN;
	volatile uint32_t VWCTRL0;
	volatile uint32_t VWCTRL1;
} KBC_Type;

/* STS */
#define KBC_STS_OBF_Pos        (0UL)
#define KBC_STS_OBF_Msk        BIT(KBC_STS_OBF_Pos)
#define KBC_STS_IBF_Pos        (1UL)
#define KBC_STS_IBF_Msk        BIT(KBC_STS_IBF_Pos)
#define KBC_STS_STS0_Pos       (2UL)
#define KBC_STS_STS0_Msk       BIT(KBC_STS_STS0_Pos)
#define KBC_STS_CMDSEL_Pos     (3UL)
#define KBC_STS_CMDSEL_Msk     BIT(KBC_STS_CMDSEL_Pos)
#define KBC_STS_STS1_Pos       (4UL)
#define KBC_STS_STS1_Msk       BIT(KBC_STS_STS1_Pos)
#define KBC_STS_STS2_Pos       (5UL)
#define KBC_STS_STS2_Msk       BIT(KBC_STS_STS2_Pos)
#define KBC_STS_STS3_Pos       (6UL)
#define KBC_STS_STS3_Msk       BIT(KBC_STS_STS3_Pos)
#define KBC_STS_STS4_Pos       (7UL)
#define KBC_STS_STS4_Msk       BIT(KBC_STS_STS4_Pos)
/* IB */
#define KBC_IB_IBDAT_Pos       (0UL)
#define KBC_IB_IBDAT_Msk       GENMASK(7, 0)
#define KBC_IB_IBCLR_Pos       (8UL)
#define KBC_IB_IBCLR_Msk       BIT(KBC_IB_IBCLR_Pos)
/* OB */
#define KBC_OB_OBDAT_Pos       (0UL)
#define KBC_OB_OBDAT_Msk       GENMASK(7, 0)
#define KBC_OB_OBCLR_Pos       (8UL)
#define KBC_OB_OBCLR_Msk       BIT(KBC_OB_OBCLR_Pos)
/* PTADDR */
#define KBC_PTADDR_DATADDR_Pos (0UL)
#define KBC_PTADDR_DATADDR_Msk GENMASK(11, 0)
#define KBC_PTADDR_CMDOFS_Pos  (12UL)
#define KBC_PTADDR_CMDOFS_Msk  GENMASK(14, 12)
/* INTEN */
#define KBC_INTEN_OBFINTEN_Pos (0UL)
#define KBC_INTEN_OBFINTEN_Msk BIT(KBC_INTEN_OBFINTEN_Pos)
#define KBC_INTEN_IBFINTEN_Pos (1UL)
#define KBC_INTEN_IBFINTEN_Msk BIT(KBC_INTEN_IBFINTEN_Pos)
/* VWCTRL0 */
#define KBC_VWCTRL0_IRQEN_Pos  (0UL)
#define KBC_VWCTRL0_IRQEN_Msk  BIT(KBC_VWCTRL0_IRQEN_Pos)
#define KBC_VWCTRL0_TGLV_Pos   (1UL)
#define KBC_VWCTRL0_TGLV_Msk   BIT(KBC_VWCTRL0_TGLV_Pos)
/* VWCTRL1 */
#define KBC_VWCTRL1_IRQNUM_Pos (0UL)
#define KBC_VWCTRL1_IRQNUM_Msk GENMASK(7, 0)
#define KBC_VWCTRL1_ACTEN_Pos  (8UL)
#define KBC_VWCTRL1_ACTEN_Msk  BIT(KBC_VWCTRL1_ACTEN_Pos)

#endif /* ZEPHYR_SOC_REALTEK_RTS5912_REG_KBC_H */
