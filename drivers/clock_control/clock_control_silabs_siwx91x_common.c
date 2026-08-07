/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>

#include "clock_control_silabs_siwx91x_common.h"

/* For sli_si91x_xtal_turn_on_request_from_m4_to_TA
 * Will be removed when the function will be implemented
 * in the NWP driver and we will call it through the NWP device API
 */
 #include "rsi_m4.h"

static const struct device *const siwx91x_clk_dev_table[] = {
	/* --- AON domain --- */
	/* AON manager children */
	SIWX91X_CLOCK_ROUTE_DEV(ref_hp_mux)
	SIWX91X_CLOCK_ROUTE_DEV(ref_ulp_mux)
	SIWX91X_CLOCK_ROUTE_DEV(aon_ref_hf_mux)
	SIWX91X_CLOCK_ROUTE_DEV(aon_ref_lf_mux)
	/* Oscillator clocks accessed via the AON driver */
	SIWX91X_CLOCK_DEV_AON(SIWX91X_CLK_RC_KHZ),
	SIWX91X_CLOCK_DEV_AON(SIWX91X_CLK_RC_MHZ),
	SIWX91X_CLOCK_DEV_AON(SIWX91X_CLK_XTAL_KHZ),
	SIWX91X_CLOCK_DEV_AON(SIWX91X_CLK_XTAL_MHZ),
	/* Gate-only or synthetic clkids */
	SIWX91X_CLOCK_DEV_AON(SIWX91X_CLK_UULP_GPIO),
	SIWX91X_CLOCK_DEV_AON(SIWX91X_CLK_RTC),
	SIWX91X_CLOCK_DEV_AON(SIWX91X_CLK_SYSRTC),
	SIWX91X_CLOCK_DEV_AON(SIWX91X_CLK_WATCHDOG),

	/* --- HP domain --- */
	/* HP manager children */
	SIWX91X_CLOCK_ROUTE_DEV(cpu_lp_mux)
	SIWX91X_CLOCK_ROUTE_DEV(cpu_mux)
	SIWX91X_CLOCK_ROUTE_DEV(pll_i2s)
	SIWX91X_CLOCK_ROUTE_DEV(pll_intf)
	SIWX91X_CLOCK_ROUTE_DEV(pin_out_mux)
	SIWX91X_CLOCK_ROUTE_DEV(ref_pll_mux)
	SIWX91X_CLOCK_ROUTE_DEV(qspi2_mux)
	SIWX91X_CLOCK_ROUTE_DEV(qspi_mux)
	SIWX91X_CLOCK_ROUTE_DEV(pll_soc)
	SIWX91X_CLOCK_ROUTE_DEV(ssi_mux)
	SIWX91X_CLOCK_ROUTE_DEV(uart0_mux)
	SIWX91X_CLOCK_ROUTE_DEV(uart1_mux)
	/* Gate-only or synthetic clkids */
	SIWX91X_CLOCK_DEV_HP(SIWX91X_CLK_GPDMA),
	SIWX91X_CLOCK_DEV_HP(SIWX91X_CLK_GSPI),
	SIWX91X_CLOCK_DEV_HP(SIWX91X_CLK_GPIO),
	SIWX91X_CLOCK_DEV_HP(SIWX91X_CLK_HP_REF_ULP),
	SIWX91X_CLOCK_DEV_HP(SIWX91X_CLK_I2C0),
	SIWX91X_CLOCK_DEV_HP(SIWX91X_CLK_I2C1),
	SIWX91X_CLOCK_DEV_HP(SIWX91X_CLK_I2S),
	SIWX91X_CLOCK_DEV_HP(SIWX91X_CLK_ICACHE),
	SIWX91X_CLOCK_DEV_HP(SIWX91X_CLK_PWM),
	SIWX91X_CLOCK_DEV_HP(SIWX91X_CLK_RNG),
	SIWX91X_CLOCK_DEV_HP(SIWX91X_CLK_STATIC_I2S),
	SIWX91X_CLOCK_DEV_HP(SIWX91X_CLK_UDMA),

	/* --- ULP domain --- */
	/* ULP manager children */
	SIWX91X_CLOCK_ROUTE_DEV(ulp_adc_mux)
	SIWX91X_CLOCK_ROUTE_DEV(ulp_i2s_mux)
	SIWX91X_CLOCK_ROUTE_DEV(ulp_ref_cpu_mux)
	SIWX91X_CLOCK_ROUTE_DEV(ulp_ref_aon_mux)
	SIWX91X_CLOCK_ROUTE_DEV(ulp_ssi_mux)
	SIWX91X_CLOCK_ROUTE_DEV(ulp_timer_mux)
	SIWX91X_CLOCK_ROUTE_DEV(ulp_uart_mux)
	/* Gate-only or synthetic clkids */
	SIWX91X_CLOCK_DEV_ULP(SIWX91X_CLK_ULP_GPIO),
	SIWX91X_CLOCK_DEV_ULP(SIWX91X_CLK_ULP_I2C),
	SIWX91X_CLOCK_DEV_ULP(SIWX91X_CLK_ULP_STATIC_I2S),
	SIWX91X_CLOCK_DEV_ULP(SIWX91X_CLK_ULP_UDMA),
};

const struct device *siwx91x_clock_control_get_device(uint32_t clkid)
{
	if (clkid >= ARRAY_SIZE(siwx91x_clk_dev_table)) {
		return NULL;
	}

	return siwx91x_clk_dev_table[clkid];
}

/* TODO: Remove theses functions when the NWP driver is fully integrated with zephyr */
void siwx91x_clock_request_xtal_to_nwp(void)
{
	__maybe_unused const struct device *nwp_dev = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(nwp));

#ifdef CONFIG_SILABS_SIWX91X_NWP
	if (nwp_dev) {
		/* need to call nwp api */
		sli_si91x_xtal_turn_on_request_from_m4_to_TA();
	}
#endif
	return;
}

void siwx91x_clock_release_xtal_to_nwp(void)
{
	__maybe_unused const struct device *nwp_dev = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(nwp));

#ifdef CONFIG_SILABS_SIWX91X_NWP
	if (nwp_dev) {
		/* need to call nwp api */
		sli_si91x_xtal_turn_off_request_from_m4_to_TA();
	}
#endif
	return;
}
