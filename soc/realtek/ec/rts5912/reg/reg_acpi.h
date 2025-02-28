/*
 * Copyright (c) 2025 Realtek, SIBG-SD7
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_REALTEK_RTS5912_REG_ACPI_H
#define ZEPHYR_SOC_REALTEK_RTS5912_REG_ACPI_H

typedef struct {
	volatile uint32_t STS;
	volatile uint32_t IB;
	volatile uint32_t OB;
	volatile uint32_t PTADDR;
	volatile uint32_t INTEN;
	volatile uint32_t VWCTRL0;
	volatile uint32_t VWCTRL1;
} ACPI_Type;

/* STS */
#define ACPI_STS_OBF_Pos        (0UL)
#define ACPI_STS_OBF_Msk        BIT(ACPI_STS_OBF_Pos)
#define ACPI_STS_IBF_Pos        (1UL)
#define ACPI_STS_IBF_Msk        BIT(ACPI_STS_IBF_Pos)
#define ACPI_STS_STS0_Pos       (2UL)
#define ACPI_STS_STS0_Msk       BIT(ACPI_STS_STS0_Pos)
#define ACPI_STS_CMDSEL_Pos     (3UL)
#define ACPI_STS_CMDSEL_Msk     BIT(ACPI_STS_CMDSEL_Pos)
#define ACPI_STS_BURST_Pos      (4UL)
#define ACPI_STS_BURST_Msk      BIT(ACPI_STS_BURST_Pos)
#define ACPI_STS_STS2_Pos       (5UL)
#define ACPI_STS_STS2_Msk       BIT(ACPI_STS_STS2_Msk)
#define ACPI_STS_STS3_Pos       (6UL)
#define ACPI_STS_STS3_Msk       BIT(ACPI_STS_STS3_Msk)
#define ACPI_STS_STS4_Pos       (7UL)
#define ACPI_STS_STS4_Msk       BIT(ACPI_STS_STS4_Pos)
/* IB */
#define ACPI_IB_IBDAT_Pos       (0UL)
#define ACPI_IB_IBDAT_Msk       GENMASK(7, 0)
#define ACPI_IB_IBCLR_Pos       (8UL)
#define ACPI_IB_IBCLR_Msk       BIT(ACPI_IB_IBCLR_Pos)
/* OB */
#define ACPI_OB_OBDAT_Pos       (0UL)
#define ACPI_OB_OBDAT_Msk       GENMASK(7, 0)
#define ACPI_OB_OBCLR_Pos       (8UL)
#define ACPI_OB_OBCLR_Msk       BIT(ACPI_OB_OBCLR_Pos)
/* PTADDR */
#define ACPI_PTADDR_ADDR_Pos    (0UL)
#define ACPI_PTADDR_ADDR_Msk    GENMASK(11, 0)
#define ACPI_PTADDR_OFFSET_Pos  (12UL)
#define ACPI_PTADDR_OFFSET_Msk  GENMASK(15, 12)
/* INTEN */
#define ACPI_INTEN_OBFINTEN_Pos (0UL)
#define ACPI_INTEN_OBFINTEN_Msk BIT(ACPI_INTEN_OBFINTEN_Pos)
#define ACPI_INTEN_IBFINTEN_Pos (1UL)
#define ACPI_INTEN_IBFINTEN_Msk BIT(ACPI_INTEN_IBFINTEN_Pos)
/* VWCTRL0 */
#define ACPI_VWCTRL0_IRQEN_Pos  (0UL)
#define ACPI_VWCTRL0_IRQEN_Msk  BIT(ACPI_VWCTRL0_IRQEN_Pos)
#define ACPI_VWCTRL0_TGLV_Pos   (1UL)
#define ACPI_VWCTRL0_TGLV_Msk   BIT(ACPI_VWCTRL0_TGLV_Pos)
/* VWCTRL1 */
#define ACPI_VWCTRL1_IRQNUM_Pos (0UL)
#define ACPI_VWCTRL1_IRQNUM_Msk GENMASK(7, 0)
#define ACPI_VWCTRL1_ACTEN_Pos  (8UL)
#define ACPI_VWCTRL1_ACTEN_Msk  BIT(ACPI_VWCTRL1_ACTEN_Pos)

#endif /* ZEPHYR_SOC_REALTEK_RTS5912_REG_ACPI_H */
