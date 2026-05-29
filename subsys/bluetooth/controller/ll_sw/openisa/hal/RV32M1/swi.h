/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ll_irqs.h"

#define SetPendingIRQ(x) (EVENT_UNIT->INTPTPENDSET |= (uint32_t)(1 << (x)))

static inline void hal_swi_init(void)
{
	/* No platform-specific initialization required. */
}

/* SW IRQs required for the SW defined BLE Controller on RV32M1. */
#if defined(CONFIG_BT_LL_SW_SPLIT)
/* Split architecture uses max. two SWI */
#define HAL_SWI_RADIO_IRQ  LL_SWI4_IRQn
#define HAL_SWI_WORKER_IRQ LL_RTC0_IRQn

#if !defined(CONFIG_BT_CTLR_LOW_LAT) && \
	(CONFIG_BT_CTLR_ULL_HIGH_PRIO == CONFIG_BT_CTLR_ULL_LOW_PRIO)
#define HAL_SWI_JOB_IRQ    HAL_SWI_WORKER_IRQ
#else
#define HAL_SWI_JOB_IRQ    LL_SWI5_IRQn
#endif

static inline void hal_swi_lll_pend(void)
{
	SetPendingIRQ(HAL_SWI_RADIO_IRQ);
}

#else
#error "CTRL architecture not defined"
#endif

static inline void hal_swi_worker_pend(void)
{
	SetPendingIRQ(HAL_SWI_WORKER_IRQ);
}

static inline void hal_swi_job_pend(void)
{
	SetPendingIRQ(HAL_SWI_JOB_IRQ);
}
