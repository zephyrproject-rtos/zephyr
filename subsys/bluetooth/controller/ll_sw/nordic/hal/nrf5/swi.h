/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_SOC_SERIES_NRF51X) || defined(CONFIG_SOC_COMPATIBLE_NRF52X)

static inline void hal_swi_init(void)
{
	/* No platform-specific initialization required. */
}

/* SW IRQs required for the nRF5 BLE Controller. */
#if defined(CONFIG_BT_LL_SW_SPLIT)
/* Split architecture uses max. two SWI */
#define HAL_SWI_RADIO_IRQ  SWI4_IRQn
#define HAL_SWI_WORKER_IRQ RTC0_IRQn

#if !defined(CONFIG_BT_CTLR_LOW_LAT) && \
	(CONFIG_BT_CTLR_ULL_HIGH_PRIO == CONFIG_BT_CTLR_ULL_LOW_PRIO)
#define HAL_SWI_JOB_IRQ    HAL_SWI_WORKER_IRQ
#else
#define HAL_SWI_JOB_IRQ    SWI5_IRQn
#endif

static inline void hal_swi_lll_pend(void)
{
	NVIC_SetPendingIRQ(HAL_SWI_RADIO_IRQ);
}

#elif defined(CONFIG_BT_LL_SW_LEGACY)
/* Legacy controller uses max. one SWI */
#define HAL_SWI_WORKER_IRQ RTC0_IRQn
#define HAL_SWI_JOB_IRQ    SWI5_IRQn

#else
#error "CTRL architecture not defined"
#endif

static inline void hal_swi_worker_pend(void)
{
	NVIC_SetPendingIRQ(HAL_SWI_WORKER_IRQ);
}

static inline void hal_swi_job_pend(void)
{
	NVIC_SetPendingIRQ(HAL_SWI_JOB_IRQ);
}

#endif /* CONFIG_SOC_SERIES_NRF51X || CONFIG_SOC_COMPATIBLE_NRF52X */
