/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/firmware/scmi/clk.h>
#include <zephyr/drivers/firmware/scmi/power.h>
#include <zephyr/drivers/firmware/scmi/nxp/cpu.h>
#include <zephyr/dt-bindings/clock/imx943_clock.h>
#include <zephyr/dt-bindings/power/imx943_power.h>
#include <soc.h>

#define SOC_CLK_NO_PARENT_CLK 0xffffffff

void soc_early_init_hook(void)
{
#ifdef CONFIG_CACHE_MANAGEMENT
	sys_cache_data_enable();
	sys_cache_instr_enable();
#endif
}

#if ((defined(CONFIG_ETH_NXP_IMX_NETC) && (DT_CHILD_NUM_STATUS_OKAY(DT_NODELABEL(netc)) != 0)) \
	|| (DT_NUM_INST_STATUS_OKAY(nxp_mcux_i2s) > 0))
static int soc_clock_set_rate_and_parent(struct soc_clk *sclk)
{
	const struct device *clk_dev = DEVICE_DT_GET(DT_NODELABEL(scmi_clk));
	struct scmi_protocol *proto = clk_dev->data;
	struct scmi_clock_rate_config clk_cfg = {0};
	int ret = 0;

	if (sclk->parent_id != SOC_CLK_NO_PARENT_CLK) {
		ret = scmi_clock_parent_set(proto, sclk->clk_id, sclk->parent_id);
		if (ret) {
			return ret;
		}
	}

	clk_cfg.flags = sclk->flags;
	clk_cfg.clk_id = sclk->clk_id;
	clk_cfg.rate[0] = sclk->rate & 0xffffffff;
	clk_cfg.rate[1] = (sclk->rate >> 32) & 0xffffffff;

	return scmi_clock_rate_set(proto, &clk_cfg);
}
#endif

#if DT_NUM_INST_STATUS_OKAY(nxp_mcux_i2s) > 0
static int soc_clock_enable(struct soc_clk *sclk)
{
	const struct device *clk_dev = DEVICE_DT_GET(DT_NODELABEL(scmi_clk));
	struct scmi_protocol *proto = clk_dev->data;
	struct scmi_clock_config clk_cfg = {0};

	clk_cfg.clk_id = sclk->clk_id;
	clk_cfg.attributes = SCMI_CLK_CONFIG_ENABLE_DISABLE(sclk->on);

	return scmi_clock_config_set(proto, &clk_cfg);
}
#endif

static int soc_init(void)
{
#if defined(CONFIG_NXP_SCMI_CPU_DOMAIN_HELPERS)
	struct scmi_nxp_cpu_sleep_mode_config cpu_cfg = {0};
#endif /* CONFIG_NXP_SCMI_CPU_DOMAIN_HELPERS */
	int ret = 0;
#if ((defined(CONFIG_ETH_NXP_IMX_NETC) && (DT_CHILD_NUM_STATUS_OKAY(DT_NODELABEL(netc)) != 0)) \
	|| (DT_NUM_INST_STATUS_OKAY(nxp_mcux_i2s) > 0))
	struct soc_clk sclk = {0};
#endif

#if defined(CONFIG_ETH_NXP_IMX_NETC) && (DT_CHILD_NUM_STATUS_OKAY(DT_NODELABEL(netc)) != 0)
	struct scmi_power_state_config pwr_cfg = {0};
	uint32_t power_state = SCMI_POWER_STATE_GENERIC_OFF;

	/* Power up NETCMIX */
	pwr_cfg.domain_id = IMX943_PD_NETC;
	pwr_cfg.power_state = SCMI_POWER_STATE_GENERIC_ON;

	ret = scmi_power_state_set(&pwr_cfg);
	if (ret) {
		return ret;
	}

	while (power_state != SCMI_POWER_STATE_GENERIC_ON) {
		ret = scmi_power_state_get(IMX943_PD_NETC, &power_state);
		if (ret) {
			return ret;
		}
	}

	sclk.parent_id = IMX943_CLK_SYSPLL1_PFD0;
	sclk.flags = SCMI_CLK_RATE_SET_FLAGS_ROUNDS_AUTO;
	sclk.rate = 250000000;
	sclk.clk_id = IMX943_CLK_ENETREF;
	ret = soc_clock_set_rate_and_parent(&sclk);
	if (ret) {
		return ret;
	}

	sclk.clk_id = IMX943_CLK_MAC0;
	ret = soc_clock_set_rate_and_parent(&sclk);
	if (ret) {
		return ret;
	}

	sclk.clk_id = IMX943_CLK_MAC1;
	ret = soc_clock_set_rate_and_parent(&sclk);
	if (ret) {
		return ret;
	}

	sclk.clk_id = IMX943_CLK_MAC2;
	ret = soc_clock_set_rate_and_parent(&sclk);
	if (ret) {
		return ret;
	}

	sclk.clk_id = IMX943_CLK_MAC3;
	ret = soc_clock_set_rate_and_parent(&sclk);
	if (ret) {
		return ret;
	}

	sclk.clk_id = IMX943_CLK_MAC4;
	ret = soc_clock_set_rate_and_parent(&sclk);
	if (ret) {
		return ret;
	}

	sclk.clk_id = IMX943_CLK_MAC5;
	ret = soc_clock_set_rate_and_parent(&sclk);
	if (ret) {
		return ret;
	}
#endif

#if DT_NUM_INST_STATUS_OKAY(nxp_mcux_i2s) > 0
	sclk.flags = SCMI_CLK_RATE_SET_FLAGS_ROUNDS_AUTO;
	sclk.parent_id = SOC_CLK_NO_PARENT_CLK;
	sclk.rate = 3932160000;
	sclk.clk_id = IMX943_CLK_AUDIOPLL1_VCO;
	ret = soc_clock_set_rate_and_parent(&sclk);
	if (ret) {
		return ret;
	}

	sclk.on = true;
	ret = soc_clock_enable(&sclk);
	if (ret) {
		return ret;
	}

	sclk.clk_id = IMX943_CLK_AUDIOPLL1;
	ret = soc_clock_set_rate_and_parent(&sclk);
	if (ret) {
		return ret;
	}

	sclk.on = true;
	ret = soc_clock_enable(&sclk);
	if (ret) {
		return ret;
	}

	sclk.parent_id = IMX943_CLK_AUDIOPLL1;
	sclk.rate = 12288000;
	sclk.clk_id = IMX943_CLK_SAI1;
	ret = soc_clock_set_rate_and_parent(&sclk);
	if (ret) {
		return ret;
	}

	sclk.on = true;
	ret = soc_clock_enable(&sclk);
	if (ret) {
		return ret;
	}
#endif

#if defined(CONFIG_NXP_SCMI_CPU_DOMAIN_HELPERS)
	cpu_cfg.cpu_id = CPU_IDX_M33P_S;
	cpu_cfg.sleep_mode = CPU_SLEEP_MODE_RUN;

	ret = scmi_nxp_cpu_sleep_mode_set(&cpu_cfg);
	if (ret) {
		return ret;
	}
#endif /* CONFIG_NXP_SCMI_CPU_DOMAIN_HELPERS */
	return ret;
}

/*
 * Because platform is using ARM SCMI, drivers like scmi, mbox etc. are
 * initialized during PRE_KERNEL_1. Common init hooks is not able to use.
 * SoC early init and board early init could be run during PRE_KERNEL_2 instead.
 */
SYS_INIT(soc_init, PRE_KERNEL_2, 0);
