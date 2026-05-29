/*
 * Copyright (c) 2025 Realtek, SIBG-SD7
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_REALTEK_RTS5912_REG_TACHO_H
#define ZEPHYR_SOC_REALTEK_RTS5912_REG_TACHO_H

struct tacho_regs {
	uint32_t ctrl;
	uint32_t status;
	uint32_t limit_high;
	uint32_t limit_low;
	uint32_t int_en;
};

/* CTRL */
#define TACHO_CTRL_EN          BIT(0UL)
#define TACHO_CTRL_FILTEREN    BIT(1UL)
#define TACHO_CTRL_SELEDGE_Pos (2UL)
#define TACHO_CTRL_SELEDGE_Msk GENMASK(3, TACHO_CTRL_SELEDGE_Pos)
#define TACHO_CTRL_READMODE    BIT(4UL)
#define TACHO_CTRL_CNT_Pos     (16UL)
#define TACHO_CTRL_CNT_Msk     GENMASK(31, TACHO_CTRL_CNT_Pos)
/* STS */
#define TACHO_STS_LIMIT        BIT(0UL)
#define TACHO_STS_PIN          BIT(1UL)
#define TACHO_STS_CHG          BIT(2UL)
#define TACHO_STS_CNTRDY       BIT(3UL)
/* LIMITH */
#define TACHO_LIMITH_VAL_Pos   (0UL)
#define TACHO_LIMITH_VAL_Msk   GENMASK(15, TACHO_LIMITH_VAL_Pos)
/* LIMITL */
#define TACHO_LIMITL_VAL_Pos   (0UL)
#define TACHO_LIMITL_VAL_Msk   GENMASK(15, TACHO_LIMITL_VAL_Pos)
/* INTEN */
#define TACHO_INTEN_LIMITEN    BIT(0UL)
#define TACHO_INTEN_CNTRDYEN   BIT(1UL)
#define TACHO_INTEN_CHGEN      BIT(2UL)

#endif /* ZEPHYR_SOC_REALTEK_RTS5912_REG_TACHO_H */
