/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>

#include <cy_sysclk.h>

#define DT_DRV_COMPAT infineon_cat1_trace

/*
 * Infineon CAT1 parallel trace port enable (Arm CoreSight SoC-600 / DEBUG600).
 *
 * Two things here need the secure world and so must be done by firmware, not the
 * debug host: HSIOM (a secure peripheral) routing the trace pins, and the
 * DEBUG600 trace-output clock, which lives behind the SYS access port. This
 * driver does both, so a debug host only has to program the trace fabric (ETM,
 * ATB funnel, ETF, TPIU) through the ordinary core access port.
 *
 * The trace-output clock is the DEBUG600 trace-input pclk (PERI PCLK group 7 on
 * PSE84), driven by a 16.5-bit fractional divider - the only divider type that
 * group has. Without it the TPIU output path is unclocked and enabling the TPIU
 * formatter stalls the debug bus.
 */
#define TRACE_CLK_INT_DIV 3U /* TRACECLK = source / (INT + 1) */

struct cat1_trace_config {
	const struct pinctrl_dev_config *pcfg;
};

static int cat1_trace_init(const struct device *dev)
{
	const struct cat1_trace_config *config = dev->config;

	Cy_SysClk_PeriPclkDisableDivider(PCLK_DEBUG600_CLOCK_TRACE_IN, CY_SYSCLK_DIV_16_5_BIT, 0U);
	Cy_SysClk_PeriPclkSetFracDivider(PCLK_DEBUG600_CLOCK_TRACE_IN, CY_SYSCLK_DIV_16_5_BIT, 0U,
					 TRACE_CLK_INT_DIV, 0U);
	Cy_SysClk_PeriPclkAssignDivider(PCLK_DEBUG600_CLOCK_TRACE_IN, CY_SYSCLK_DIV_16_5_BIT, 0U);
	Cy_SysClk_PeriPclkEnableDivider(PCLK_DEBUG600_CLOCK_TRACE_IN, CY_SYSCLK_DIV_16_5_BIT, 0U);

	return pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
}

#define CAT1_TRACE_INIT(idx)                                                                       \
	PINCTRL_DT_INST_DEFINE(idx);                                                               \
                                                                                                  \
	static const struct cat1_trace_config cat1_trace_config_##idx = {                          \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),                                       \
	};                                                                                         \
                                                                                                  \
	DEVICE_DT_INST_DEFINE(idx, &cat1_trace_init, NULL, NULL, &cat1_trace_config_##idx,         \
			      POST_KERNEL, CONFIG_DEBUG_DRIVER_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(CAT1_TRACE_INIT)
