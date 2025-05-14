/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rza2m_cpg

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include "clock_control_renesas_rza2m_cpg_lld.h"

static int clock_control_renesas_rza2m_on_off(const struct device *dev, clock_control_subsys_t sys,
					      bool enable)
{
	if (!dev || !sys) {
		return -EINVAL;
	}

	int ret = -EINVAL;
	uint32_t *clock_id = (uint32_t *)sys;
	uint32_t clk_module = RZA2M_GET_MODULE(*clock_id);

	ret = rza2m_cpg_mstp_clock_endisable(dev, clk_module, enable);

	return ret;
}

static int clock_control_renesas_rza2m_on(const struct device *dev, clock_control_subsys_t sys)
{
	/* Enable the specified clock */
	return clock_control_renesas_rza2m_on_off(dev, sys, true);
}

static int clock_control_renesas_rza2m_off(const struct device *dev, clock_control_subsys_t sys)
{
	/* Disable the specified clock */
	return clock_control_renesas_rza2m_on_off(dev, sys, false);
}

static int clock_control_renesas_rza2m_get_rate(const struct device *dev,
						clock_control_subsys_t sys, uint32_t *rate)
{
	if (!dev || !sys || !rate) {
		return -EINVAL;
	}

	int ret = -EINVAL;
	uint32_t *clock_id = (uint32_t *)sys;
	enum rza2m_cpg_get_freq_src clk_src = RZA2M_GET_CLOCK_SRC(*clock_id);

	ret = rza2m_cpg_get_clock(dev, clk_src, rate);

	return ret;
}

static int clock_control_renesas_rza2m_set_rate(const struct device *dev,
						clock_control_subsys_t sys,
						clock_control_subsys_rate_t rate)
{
	int ret;
	enum rza2m_cp_sub_clock clock_name = (enum rza2m_cp_sub_clock)sys;
	uint32_t clock_rate = (uint32_t)rate;

	ret = rza2m_cpg_set_sub_clock_divider(dev, clock_name, clock_rate);

	return ret;
}

static int clock_control_renesas_rza2m_init(const struct device *dev)
{
	const struct rza2m_cpg_clock_config *config = dev->config;
	uint16_t reg_val;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
	rza2m_cpg_calculate_pll_frequency(dev);
	/* Select Bφ Clock output for CLKIO */
	sys_write16(0, CPG_REG_ADDR(CPG_CKIOSEL_OFFSET));

	/* Enable CLKIO as Low-level output */
	reg_val = sys_read16(CPG_REG_ADDR(CPG_FRQCR_OFFSET));
	reg_val &= ~(CPG_FRQCR_CKOEN | CPG_FRQCR_CKOEN2);
	reg_val |= (1U << CPG_FRQCR_CKOEN_SHIFT) | (1U << CPG_FRQCR_CKOEN2_SHIFT);
	sys_write16(reg_val, CPG_REG_ADDR(CPG_FRQCR_OFFSET));

	rza2m_cpg_set_sub_clock_divider(dev, CPG_SUB_CLOCK_ICLK, config->cpg_iclk_freq_hz_cfg);
	rza2m_cpg_set_sub_clock_divider(dev, CPG_SUB_CLOCK_BCLK, config->cpg_bclk_freq_hz_cfg);
	rza2m_cpg_set_sub_clock_divider(dev, CPG_SUB_CLOCK_P1CLK, config->cpg_p1clk_freq_hz_cfg);

	return 0;
}
static DEVICE_API(clock_control, rza2m_clock_control_driver_api) = {
	.on = clock_control_renesas_rza2m_on,
	.off = clock_control_renesas_rza2m_off,
	.get_rate = clock_control_renesas_rza2m_get_rate,
	.set_rate = clock_control_renesas_rza2m_set_rate,
};

static const struct rza2m_cpg_clock_config g_rza2m_cpg_clock_config = {
	DEVICE_MMIO_ROM_INIT(DT_DRV_INST(0)),
	.cpg_extal_freq_hz_cfg = DT_INST_PROP_BY_PHANDLE(0, clocks, clock_frequency),
	.cpg_iclk_freq_hz_cfg = DT_PROP(DT_NODELABEL(iclk), clock_frequency),
	.cpg_bclk_freq_hz_cfg = DT_PROP(DT_NODELABEL(bclk), clock_frequency),
	.cpg_p1clk_freq_hz_cfg = DT_PROP(DT_NODELABEL(p1clk), clock_frequency),
};

static struct rza2m_cpg_clock_data g_rza2m_cpg_clock_data;

DEVICE_DT_INST_DEFINE(0, clock_control_renesas_rza2m_init, NULL, &g_rza2m_cpg_clock_data,
		      &g_rza2m_cpg_clock_config, PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		      &rza2m_clock_control_driver_api);
