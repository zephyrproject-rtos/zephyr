/*
 * Copyright (c) 2025 Realtek, SIBG-SD7
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_REALTEK_RTS5912_REG_TACHO_H
#define ZEPHYR_SOC_REALTEK_RTS5912_REG_TACHO_H

typedef struct {
	union {
		volatile uint32_t CTRL;

		struct {
			volatile uint32_t EN: 1;
			volatile uint32_t FILTEREN: 1;
			volatile uint32_t SELEDGE: 2;
			volatile uint32_t READMODE: 1;
			uint32_t: 11;
			const volatile uint32_t CNT: 16;
		} CTRL_b;
	};

	union {
		volatile uint32_t STS;

		struct {
			volatile uint32_t LIMIT: 1;
			const volatile uint32_t PIN: 1;
			volatile uint32_t CHG: 1;
			volatile uint32_t CNTRDY: 1;
			uint32_t: 28;
		} STS_b;
	};

	union {
		volatile uint32_t LIMITH;

		struct {
			volatile uint32_t VAL: 16;
			uint32_t: 16;
		} LIMITH_b;
	};

	union {
		volatile uint32_t LIMITL;

		struct {
			volatile uint32_t VAL: 16;
			uint32_t: 16;
		} LIMITL_b;
	};

	union {
		volatile uint32_t INTEN;

		struct {
			volatile uint32_t LIMITEN: 1;
			volatile uint32_t CNTRDYEN: 1;
			volatile uint32_t CHGEN: 1;
			uint32_t: 29;
		} INTEN_b;
	};
} TACHO_Type;

/* CTRL */
#define TACHO_CTRL_EN_Pos        (0UL)
#define TACHO_CTRL_EN_Msk        BIT(TACHO_CTRL_EN_Pos)
#define TACHO_CTRL_FILTEREN_Pos  (1UL)
#define TACHO_CTRL_FILTEREN_Msk  BIT(TACHO_CTRL_FILTEREN_Pos)
#define TACHO_CTRL_SELEDGE_Pos   (2UL)
#define TACHO_CTRL_SELEDGE_Msk   GENMASK(3, TACHO_CTRL_SELEDGE_Pos)
#define TACHO_CTRL_READMODE_Pos  (4UL)
#define TACHO_CTRL_READMODE_Msk  BIT(TACHO_CTRL_READMODE_Pos)
#define TACHO_CTRL_CNT_Pos       (16UL)
#define TACHO_CTRL_CNT_Msk       GENMASK(31, TACHO_CTRL_CNT_Pos)
/* STS */
#define TACHO_STS_LIMIT_Pos      (0UL)
#define TACHO_STS_LIMIT_Msk      BIT(TACHO_STS_LIMIT_Pos)
#define TACHO_STS_PIN_Pos        (1UL)
#define TACHO_STS_PIN_Msk        BIT(TACHO_STS_PIN_Pos)
#define TACHO_STS_CHG_Pos        (2UL)
#define TACHO_STS_CHG_Msk        BIT(TACHO_STS_CHG_Pos)
#define TACHO_STS_CNTRDY_Pos     (3UL)
#define TACHO_STS_CNTRDY_Msk     BIT(TACHO_STS_CNTRDY_Pos)
/* LIMITH */
#define TACHO_LIMITH_VAL_Pos     (0UL)
#define TACHO_LIMITH_VAL_Msk     GENMASK(15, TACHO_LIMITH_VAL_Pos)
/* LIMITL */
#define TACHO_LIMITL_VAL_Pos     (0UL)
#define TACHO_LIMITL_VAL_Msk     GENMASK(15, TACHO_LIMITL_VAL_Pos)
/* INTEN */
#define TACHO_INTEN_LIMITEN_Pos  (0UL)
#define TACHO_INTEN_LIMITEN_Msk  BIT(TACHO_INTEN_LIMITEN_Pos)
#define TACHO_INTEN_CNTRDYEN_Pos (1UL)
#define TACHO_INTEN_CNTRDYEN_Msk BIT(TACHO_INTEN_CNTRDYEN_Pos)
#define TACHO_INTEN_CHGEN_Pos    (2UL)
#define TACHO_INTEN_CHGEN_Msk    BIT(TACHO_INTEN_CHGEN_Pos)

#endif /* ZEPHYR_SOC_REALTEK_RTS5912_REG_TACHO_H */
