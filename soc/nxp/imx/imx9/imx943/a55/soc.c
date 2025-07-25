/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/firmware/scmi/clk.h>
#include <zephyr/drivers/firmware/scmi/power.h>
#include <zephyr/drivers/firmware/scmi/nxp/cpu.h>
#include <zephyr/dt-bindings/clock/imx943_clock.h>
#include <zephyr/dt-bindings/power/imx943_power.h>
#include <soc.h>

/* SCMI power domain states */
#define POWER_DOMAIN_STATE_ON  0x00000000
#define POWER_DOMAIN_STATE_OFF 0x40000000

#if defined(CONFIG_ETH_NXP_IMX_NETC) && (DT_CHILD_NUM_STATUS_OKAY(DT_NODELABEL(netc)) != 0)
/* The function is to reuse code for 250MHz NETC system clock and MACs clocks initialization */
static int soc_netc_clock_init(int clk_id)
{
	const struct device *clk_dev = DEVICE_DT_GET(DT_NODELABEL(scmi_clk));
	struct scmi_protocol *proto = clk_dev->data;
	struct scmi_clock_rate_config clk_cfg = {0};
	uint64_t clk_250m = 250000000;
	int ret = 0;

	ret = scmi_clock_parent_set(proto, clk_id, IMX943_CLK_SYSPLL1_PFD0);
	if (ret) {
		return ret;
	}

	clk_cfg.flags = SCMI_CLK_RATE_SET_FLAGS_ROUNDS_AUTO;
	clk_cfg.clk_id = clk_id;
	clk_cfg.rate[0] = clk_250m & 0xffffffff;
	clk_cfg.rate[1] = (clk_250m >> 32) & 0xffffffff;

	return scmi_clock_rate_set(proto, &clk_cfg);
}
#endif

static int soc_init(void)
{
#if defined(CONFIG_NXP_SCMI_CPU_DOMAIN_HELPERS)
	struct scmi_cpu_sleep_mode_config cpu_cfg = {0};
#endif /* CONFIG_NXP_SCMI_CPU_DOMAIN_HELPERS */
	int ret = 0;

#if defined(CONFIG_ETH_NXP_IMX_NETC) && (DT_CHILD_NUM_STATUS_OKAY(DT_NODELABEL(netc)) != 0)
	struct scmi_power_state_config pwr_cfg = {0};
	uint32_t power_state = POWER_DOMAIN_STATE_OFF;

	/* Power up NETCMIX */
	pwr_cfg.domain_id = IMX943_PD_NETC;
	pwr_cfg.power_state = POWER_DOMAIN_STATE_ON;

	ret = scmi_power_state_set(&pwr_cfg);
	if (ret) {
		return ret;
	}

	while (power_state != POWER_DOMAIN_STATE_ON) {
		ret = scmi_power_state_get(IMX943_PD_NETC, &power_state);
		if (ret) {
			return ret;
		}
	}

	ret = soc_netc_clock_init(IMX943_CLK_ENETREF);
	if (ret) {
		return ret;
	}

	ret = soc_netc_clock_init(IMX943_CLK_MAC0);
	if (ret) {
		return ret;
	}

	ret = soc_netc_clock_init(IMX943_CLK_MAC1);
	if (ret) {
		return ret;
	}

	ret = soc_netc_clock_init(IMX943_CLK_MAC2);
	if (ret) {
		return ret;
	}

	ret = soc_netc_clock_init(IMX943_CLK_MAC3);
	if (ret) {
		return ret;
	}

	ret = soc_netc_clock_init(IMX943_CLK_MAC4);
	if (ret) {
		return ret;
	}

	ret = soc_netc_clock_init(IMX943_CLK_MAC5);
	if (ret) {
		return ret;
	}
#endif

	return ret;
}

/*
 * Because platform is using ARM SCMI, drivers like scmi, mbox etc. are
 * initialized during PRE_KERNEL_1. Common init hooks is not able to use.
 * SoC early init and board early init could be run during PRE_KERNEL_2 instead.
 */
SYS_INIT(soc_init, PRE_KERNEL_2, 0);
