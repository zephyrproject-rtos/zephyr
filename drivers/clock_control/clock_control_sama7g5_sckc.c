/*
 * Copyright (C) 2025-2026 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_sama7g5_sckc

#include <soc.h>

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mchp_sam_pmc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sckc, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

BUILD_ASSERT(DT_INST_PROP(0, osc_sel_pos) < 32, "Invalid position of OSC_SEL bit");

static const struct mchp_sckc_config {
	DEVICE_MMIO_ROM;
	const struct device *clock;
	const uint32_t slow_rc_frequency;
	const uint8_t osc_sel_pos;
} mchp_sckc_cfg = {
	DEVICE_MMIO_ROM_INIT(DT_DRV_INST(0)),
	.clock = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0)),
	.slow_rc_frequency = DT_INST_PROP(0, slow_rc_frequency),
	.osc_sel_pos =  DT_INST_PROP(0, osc_sel_pos),
};

static struct mchp_sckc_data {
	DEVICE_MMIO_RAM;
} mchp_sckc_data;

static int sckc_on(const struct device *dev, clock_control_subsys_t sys)
{
	const struct mchp_sckc_config *const config = dev->config;
	uint32_t reg = FIELD_GET(~BIT(config->osc_sel_pos),
				 sys_read32(DEVICE_MMIO_GET(dev) + SCKC_CR_REG_OFST));
	const struct sam_sckc_config *cfg = sys;

	if (cfg == NULL) {
		LOG_ERR("The SCKC config can not be NULL.");
		return -ENXIO;
	}

	LOG_DBG("%s Oscillator", cfg->crystal_osc ? "Crystal" : "RC");

	if (cfg->crystal_osc) {
		reg |= BIT(config->osc_sel_pos);
	}

	sys_write32(reg, DEVICE_MMIO_GET(dev) + SCKC_CR_REG_OFST);

	return 0;
}

static int sckc_get_rate(const struct device *dev,
			 clock_control_subsys_t sys,
			 uint32_t *rate)
{
	ARG_UNUSED(sys);

	const struct mchp_sckc_config *const config = dev->config;

	bool sel_xtal = FIELD_GET(~BIT(config->osc_sel_pos),
				  sys_read32(DEVICE_MMIO_GET(dev) + SCKC_CR_REG_OFST)) != 0;
	int ret = 0;

	LOG_DBG("%s Oscillator", sel_xtal ? "Crystal" : "RC");

	if (sel_xtal) {
		ret = clock_control_get_rate(config->clock, NULL, rate);
	} else {
		*rate = config->slow_rc_frequency;
	}
	LOG_DBG("Rate: %d", *rate);

	return ret;
}

static enum clock_control_status sckc_get_status(const struct device *dev,
						 clock_control_subsys_t sys)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(sys);

	return CLOCK_CONTROL_STATUS_ON;
}

static int sckc_driver_init(const struct device *dev)
{
	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	return 0;
}

static DEVICE_API(clock_control, sckc_api) = {
	.on = sckc_on,
	.get_rate = sckc_get_rate,
	.get_status = sckc_get_status,
};

DEVICE_DT_INST_DEFINE(0, sckc_driver_init, NULL, &mchp_sckc_data, &mchp_sckc_cfg, PRE_KERNEL_1,
		      CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		      &sckc_api);
