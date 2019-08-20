/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_SOC_SERIES_NRF51X) || defined(CONFIG_SOC_COMPATIBLE_NRF52X)

static inline void hal_nrf5_irq_init(void)
{
	/* No platform-specific initialization required. */
}

/* SW IRQs required for the nRF5 BLE Controller (Split Architecture). */
#if defined(CONFIG_BT_LL_SW_SPLIT)
#define HAL_RADIO_LLL_IRQ     NRF5_IRQ_SWI4_IRQn
#define HAL_RADIO_ULL_LOW_IRQ NRF5_IRQ_SWI5_IRQn

#define HAL_RADIO_SW_IRQ      HAL_RADIO_ULL_LOW_IRQ

static inline void hal_nrf5_pend_lll_irq(void)
{
	NVIC_SetPendingIRQ(HAL_RADIO_LLL_IRQ);
}

#elif defined(CONFIG_BT_LL_SW_LEGACY)
/* A single SW IRQ is required for the nRF Legacy BLE Controller. */
#define HAL_RADIO_SW_IRQ NRF5_IRQ_SWI5_IRQn

#else
#error "CTRL architecture not defined"
#endif

static inline void hal_nrf5_pend_job_irq(void)
{
	NVIC_SetPendingIRQ(HAL_RADIO_SW_IRQ);
}

#endif /* CONFIG_SOC_SERIES_NRF51X || CONFIG_SOC_COMPATIBLE_NRF52X */
