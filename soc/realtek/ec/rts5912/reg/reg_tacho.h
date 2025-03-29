/*
 * Copyright (c) 2025 Realtek, SIBG-SD7
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_REALTEK_RTS5912_REG_TACHO_H
#define ZEPHYR_SOC_REALTEK_RTS5912_REG_TACHO_H

struct tacho_regs {
	union {
		uint32_t CTRL;

		struct {
			uint32_t EN: 1;
			uint32_t FILTEREN: 1;
			uint32_t SELEDGE: 2;
			uint32_t READMODE: 1;
			uint32_t: 11;
			const uint32_t CNT: 16;
		} CTRL_b;
	};

	union {
		uint32_t STS;

		struct {
			uint32_t LIMIT: 1;
			const uint32_t PIN: 1;
			uint32_t CHG: 1;
			uint32_t CNTRDY: 1;
			uint32_t: 28;
		} STS_b;
	};

	union {
		uint32_t LIMITH;

		struct {
			uint32_t VAL: 16;
			uint32_t: 16;
		} LIMITH_b;
	};

	union {
		uint32_t LIMITL;

		struct {
			uint32_t VAL: 16;
			uint32_t: 16;
		} LIMITL_b;
	};

	union {
		uint32_t INTEN;

		struct {
			uint32_t LIMITEN: 1;
			uint32_t CNTRDYEN: 1;
			uint32_t CHGEN: 1;
			uint32_t: 29;
		} INTEN_b;
	};
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
