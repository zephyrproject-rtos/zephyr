/*
 * Copyright (c) 2023 Realtek, SIBG-SD7
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_REALTEK_RTS5912_REG_RTMR_H
#define ZEPHYR_SOC_REALTEK_RTS5912_REG_RTMR_H

/*
 * @brief RTOS Timer Controller (RTMR)
 */

typedef struct {
	volatile uint32_t LDCNT;
	volatile uint32_t CNT;
	volatile uint32_t CTRL;
	volatile uint32_t INTSTS;
} RTOSTMR_Type;
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

#endif /* ZEPHYR_SOC_REALTEK_RTS5912_REG_RTMR_H */
