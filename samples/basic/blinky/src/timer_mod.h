/*
 * Copyright 2018, NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _TIMER_MOD_H_
#define _TIMER_MOD_H_

#include "fsl_common.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define SYSTICK_BASE    GPT1
#define SYSTICK_IRQn    GPT1_IRQn
#define SYSTICK_HANDLER GPT1_IRQHandler
#define SYSTICK_CLOCK \
    24000000 / (CLOCK_GetRootPreDivider(kCLOCK_RootGpt1)) / (CLOCK_GetRootPostDivider(kCLOCK_RootGpt1))
/*******************************************************************************
 * Definitions
 ******************************************************************************/
/* Systick counter source clock frequency. Need config the Systick counter's(GPT) clock
 * source to this frequency in application.
 */
#define SYSTICK_COUNTER_SOURCE_CLK_FREQ (SYSTICK_CLOCK)
/* Define the counter clock of the systick (GPT).*/
#define SYSTICK_COUNTER_FREQ (8000000U)
/* Define the count per tick of the systick in run mode. For accuracy purpose,
 * please make SYSTICK_SOURCE_CLOCK times of configTICK_RATE_HZ.
 */
#define SYSTICK_COUNT_PER_TICK (SYSTICK_COUNTER_FREQ / configTICK_RATE_HZ)
/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus*/

/*!
 * @brief Configure the system tick(GPT) before entering the low power mode.
 * @return Return the sleep time ticks.
 */
void TimerInterruptSetup(void);

#if defined(__cplusplus)
}
#endif /* __cplusplus*/
#endif /* _LPM_H_ */
