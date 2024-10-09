/*
 * Copyright (c) 2023 Realtek, SIBG-SD7
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @brief RTOS Timer Controller (RTMR)
 */

typedef struct {
	volatile uint32_t LDCNT;
	volatile uint32_t CNT;

	union {
		volatile uint32_t CTRL;

		struct {
			volatile uint32_t EN: 1;
			volatile uint32_t MDSEL: 1;
			volatile uint32_t INTEN: 1;
			uint32_t reserved1: 29;
		} CTRL_b;
	};

	union {
		volatile uint32_t INTSTS;

		struct {
			volatile uint32_t STS: 1;
			uint32_t reserved1: 31;
		} INTSTS_b;
	};
} RTOSTMR_Type;
/* LDCNT */
/* CNT */
/* CTRL */
#define RTOSTMR_CTRL_EN_Pos    (0UL)
#define RTOSTMR_CTRL_EN_Msk    BIT(RTOSTMR_CTRL_EN_Pos)
#define RTOSTMR_CTRL_MDSEL_Pos (1UL)
#define RTOSTMR_CTRL_MDSEL_Msk BIT(RTOSTMR_CTRL_MDSEL_Pos)
#define RTOSTMR_CTRL_INTEN_Pos (2UL)
#define RTOSTMR_CTRL_INTEN_Msk BIT(RTOSTMR_CTRL_INTEN_Pos)
/* INTSTS */
#define RTOSTMR_INTSTS_STS_Pos (0UL)
#define RTOSTMR_INTSTS_STS_Msk BIT(RTOSTMR_INTSTS_STS_Pos)
