/*
 * Copyright (c) 2024 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/cache.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <hal/nrf_hsfll.h>
#include <hal/nrf_lrcconf.h>
#include <soc/nrfx_coredep.h>

LOG_MODULE_REGISTER(soc, CONFIG_SOC_LOG_LEVEL);

#if defined(NRF_APPLICATION)
#define HSFLL_NODE DT_NODELABEL(cpuapp_hsfll)
#elif defined(NRF_RADIOCORE)
#define HSFLL_NODE DT_NODELABEL(cpurad_hsfll)
#endif

#define FICR_ADDR_GET(node_id, name)                                           \
	DT_REG_ADDR(DT_PHANDLE_BY_NAME(node_id, nordic_ficrs, name)) +         \
		DT_PHA_BY_NAME(node_id, nordic_ficrs, name, offset)

static void power_domain_init(void)
{
	/*
	 * Set:
	 *  - LRCCONF010.POWERON.MAIN: 1
	 *  - LRCCONF010.POWERON.ACT: 1
	 *  - LRCCONF010.RETAIN.MAIN: 1
	 *  - LRCCONF010.RETAIN.ACT: 1
	 *
	 *  This is done here at boot so that when the idle routine will hit
	 *  WFI the power domain will be correctly retained.
	 */

	nrf_lrcconf_poweron_force_set(NRF_LRCCONF010, NRF_LRCCONF_POWER_MAIN, true);
	nrf_lrcconf_poweron_force_set(NRF_LRCCONF010, NRF_LRCCONF_POWER_DOMAIN_0, true);

	nrf_lrcconf_retain_set(NRF_LRCCONF010, NRF_LRCCONF_POWER_MAIN, true);
	nrf_lrcconf_retain_set(NRF_LRCCONF010, NRF_LRCCONF_POWER_DOMAIN_0, true);
}

static int trim_hsfll(void)
{
	NRF_HSFLL_Type *hsfll = (NRF_HSFLL_Type *)DT_REG_ADDR(HSFLL_NODE);
	nrf_hsfll_trim_t trim = {
		.vsup = sys_read32(FICR_ADDR_GET(HSFLL_NODE, vsup)),
		.coarse = sys_read32(FICR_ADDR_GET(HSFLL_NODE, coarse)),
		.fine = sys_read32(FICR_ADDR_GET(HSFLL_NODE, fine))
	};

	LOG_DBG("Trim: HSFLL VSUP: 0x%.8x", trim.vsup);
	LOG_DBG("Trim: HSFLL COARSE: 0x%.8x", trim.coarse);
	LOG_DBG("Trim: HSFLL FINE: 0x%.8x", trim.fine);

	nrf_hsfll_clkctrl_mult_set(hsfll,
				   DT_PROP(HSFLL_NODE, clock_frequency) /
					   DT_PROP(DT_CLOCKS_CTLR(HSFLL_NODE), clock_frequency));
	nrf_hsfll_trim_set(hsfll, &trim);

	nrf_hsfll_task_trigger(hsfll, NRF_HSFLL_TASK_FREQ_CHANGE);

	LOG_DBG("NRF_HSFLL->TRIM.VSUP = %d", hsfll->TRIM.VSUP);
	LOG_DBG("NRF_HSFLL->TRIM.COARSE = %d", hsfll->TRIM.COARSE);
	LOG_DBG("NRF_HSFLL->TRIM.FINE = %d", hsfll->TRIM.FINE);

	return 0;
}

static int nordicsemi_nrf54h_init(void)
{
#if defined(CONFIG_NRF_ENABLE_ICACHE)
	sys_cache_instr_enable();
#endif

	power_domain_init();

	trim_hsfll();

	return 0;
}

void arch_busy_wait(uint32_t time_us)
{
	nrfx_coredep_delay_us(time_us);
}

SYS_INIT(nordicsemi_nrf54h_init, PRE_KERNEL_1, 0);
