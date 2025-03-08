/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/firmware/scmi/clk.h>
#include <zephyr/drivers/firmware/scmi/power.h>
#include <zephyr/dt-bindings/clock/imx95_clock.h>
#include <zephyr/dt-bindings/power/imx95_power.h>
#include <soc.h>

/* SCMI power domain states */
#define POWER_DOMAIN_STATE_ON  0x00000000
#define POWER_DOMAIN_STATE_OFF 0x40000000

void soc_late_init_hook(void)
{
#ifdef CONFIG_CACHE_MANAGEMENT
	sys_cache_data_enable();
	sys_cache_instr_enable();
#endif /* CONFIG_CACHE_MANAGEMENT */
}

static int soc_init(void)
{
#if defined(CONFIG_ETH_NXP_IMX_NETC) && (DT_CHILD_NUM_STATUS_OKAY(DT_NODELABEL(netc)) != 0)
	const struct device *clk_dev = DEVICE_DT_GET(DT_NODELABEL(scmi_clk));
	struct scmi_protocol *proto = clk_dev->data;
	struct scmi_clock_rate_config clk_cfg = {0};
	struct scmi_power_state_config pwr_cfg = {0};
	uint32_t power_state = POWER_DOMAIN_STATE_OFF;
	uint64_t enetref_clk = 250000000; /* 250 MHz*/
	int ret;

	/* Power up NETCMIX */
	pwr_cfg.domain_id = IMX95_PD_NETC;
	pwr_cfg.power_state = POWER_DOMAIN_STATE_ON;

	ret = scmi_power_state_set(&pwr_cfg);
	if (ret) {
		return ret;
	}

	while (power_state != POWER_DOMAIN_STATE_ON) {
		ret = scmi_power_state_get(IMX95_PD_NETC, &power_state);
		if (ret) {
			return ret;
		}
	}

	/* ENETREF clock init */
	ret = scmi_clock_parent_set(proto, IMX95_CLK_ENETREF, IMX95_CLK_SYSPLL1_PFD0);
	if (ret) {
		return ret;
	}

	clk_cfg.flags = SCMI_CLK_RATE_SET_FLAGS_ROUNDS_AUTO;
	clk_cfg.clk_id = IMX95_CLK_ENETREF;
	clk_cfg.rate[0] = enetref_clk & 0xffffffff;
	clk_cfg.rate[1] = (enetref_clk >> 32) & 0xffffffff;

	ret = scmi_clock_rate_set(proto, &clk_cfg);
	if (ret) {
		return ret;
	}
#endif
	return 0;
}

/*
 * Because platform is using ARM SCMI, drivers like scmi, mbox etc. are
 * initialized during PRE_KERNEL_1. Common init hooks is not able to use.
 * SoC early init and board early init could be run during PRE_KERNEL_2 instead.
 */
SYS_INIT(soc_init, PRE_KERNEL_2, 0);
