/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <hal/nrf_hsfll.h>
#include <hal/nrf_auxpll.h>

LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

#define FICR_ADDR_GET(node_id)									\
	DT_REG_ADDR(DT_PHANDLE(node_id, nordic_ficrs)) +					\
	DT_PHA(node_id, nordic_ficrs, offset)

#define FICR_ADDR_GET_BY_NAME(node_id, name)							\
	DT_REG_ADDR(DT_PHANDLE_BY_NAME(node_id, nordic_ficrs, name)) +				\
	DT_PHA_BY_NAME(node_id, nordic_ficrs, name, offset)

#if defined(CONFIG_SOC_NRF54H20_CPUAPP) || defined(CONFIG_SOC_NRF9280_CPUAPP)
#define LOCAL_HSFLL_NODE DT_NODELABEL(cpuapp_hsfll)
#elif defined(CONFIG_SOC_NRF54H20_CPURAD) || defined(CONFIG_SOC_NRF9280_CPURAD)
#define LOCAL_HSFLL_NODE DT_NODELABEL(cpurad_hsfll)
#else
#error "unsupported"
#endif

static void trim_local_hsfll(void)
{
	NRF_HSFLL_Type *hsfll = (NRF_HSFLL_Type *)DT_REG_ADDR(LOCAL_HSFLL_NODE);
	nrf_hsfll_trim_t trim = {
		.vsup = sys_read32(FICR_ADDR_GET_BY_NAME(LOCAL_HSFLL_NODE, vsup)),
		.coarse = sys_read32(FICR_ADDR_GET_BY_NAME(LOCAL_HSFLL_NODE, coarse)),
		.fine = sys_read32(FICR_ADDR_GET_BY_NAME(LOCAL_HSFLL_NODE, fine))
	};

	LOG_DBG("Trim: HSFLL VSUP: 0x%.8x", trim.vsup);
	LOG_DBG("Trim: HSFLL COARSE: 0x%.8x", trim.coarse);
	LOG_DBG("Trim: HSFLL FINE: 0x%.8x", trim.fine);

	nrf_hsfll_clkctrl_mult_set(hsfll,
				   DT_PROP(LOCAL_HSFLL_NODE, clock_frequency) /
				   DT_PROP(DT_CLOCKS_CTLR(LOCAL_HSFLL_NODE), clock_frequency));
	nrf_hsfll_trim_set(hsfll, &trim);

	nrf_hsfll_task_trigger(hsfll, NRF_HSFLL_TASK_FREQ_CHANGE);
	/* HSFLL task frequency change needs to be triggered twice to take effect.*/
	nrf_hsfll_task_trigger(hsfll, NRF_HSFLL_TASK_FREQ_CHANGE);

	LOG_DBG("NRF_HSFLL->TRIM.VSUP = %d", hsfll->TRIM.VSUP);
	LOG_DBG("NRF_HSFLL->TRIM.COARSE = %d", hsfll->TRIM.COARSE);
	LOG_DBG("NRF_HSFLL->TRIM.FINE = %d", hsfll->TRIM.FINE);
}

#define TRIM_NRF_AUXPLL_DEFINE(node_id)								\
	{											\
		NRF_AUXPLL_Type *auxpll = (NRF_AUXPLL_Type *)DT_REG_ADDR(node_id);		\
												\
		LOG_DBG("Trim: AUXPLL CTUNE: 0x%02x", sys_read8(FICR_ADDR_GET(node_id)));	\
		nrf_auxpll_lock(auxpll);							\
		nrf_auxpll_trim_ctune_set(auxpll, sys_read8(FICR_ADDR_GET(node_id)));		\
		nrf_auxpll_unlock(auxpll);							\
		LOG_DBG("AUXPLL->AUXPL.CTUNE = %d", auxpll->TRIM.CTUNE);			\
	}

void nrf54hx_nrf92x_trim(void)
{
	trim_local_hsfll();

	DT_FOREACH_STATUS_OKAY(nordic_nrf_auxpll, TRIM_NRF_AUXPLL_DEFINE)
}
