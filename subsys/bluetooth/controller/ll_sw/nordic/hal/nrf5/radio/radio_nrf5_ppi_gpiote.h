/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Extracted from radio_nrf_ppi.h functions that reference gpiote variables. */

#if defined(HAL_RADIO_GPIO_HAVE_PA_PIN) || defined(HAL_RADIO_GPIO_HAVE_LNA_PIN)
static inline void hal_palna_ppi_setup(void)
{
	nrf_ppi_channel_endpoint_setup(
		NRF_PPI,
		HAL_ENABLE_PALNA_PPI,
		(uint32_t)&(EVENT_TIMER->EVENTS_COMPARE[2]),
		(uint32_t)&(gpiote_palna.p_reg->TASKS_OUT[gpiote_ch_palna]));
	nrf_ppi_channel_endpoint_setup(
		NRF_PPI,
		HAL_DISABLE_PALNA_PPI,
		(uint32_t)&(NRF_RADIO->EVENTS_DISABLED),
		(uint32_t)&(gpiote_palna.p_reg->TASKS_OUT[gpiote_ch_palna]));
}
#endif /* defined(HAL_RADIO_GPIO_HAVE_PA_PIN) || defined(HAL_RADIO_GPIO_HAVE_LNA_PIN) */

/******************************************************************************/
#if defined(HAL_RADIO_FEM_IS_NRF21540)
static inline void hal_pa_ppi_setup(void)
{
	/* Nothing specific to PA with FEM to handle inside TRX chains */
}

static inline void hal_lna_ppi_setup(void)
{
	/* Nothing specific to LNA with FEM to handle inside TRX chains */
}

static inline void hal_fem_ppi_setup(void)
{
	nrf_ppi_channel_endpoint_setup(
		NRF_PPI,
		HAL_ENABLE_FEM_PPI,
		(uint32_t)&(EVENT_TIMER->EVENTS_COMPARE[3]),
		(uint32_t)&(gpiote_pdn.p_reg->TASKS_OUT[gpiote_ch_pdn]));
	nrf_ppi_channel_endpoint_setup(
		NRF_PPI,
		HAL_DISABLE_FEM_PPI,
		(uint32_t)&(NRF_RADIO->EVENTS_DISABLED),
		(uint32_t)&(gpiote_pdn.p_reg->TASKS_OUT[gpiote_ch_pdn]));
}

#endif /* HAL_RADIO_FEM_IS_NRF21540 */
