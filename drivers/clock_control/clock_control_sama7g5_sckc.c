/*
 * Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
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

#define SAM_DT_SLOW_XTAL DEVICE_DT_GET(DT_NODELABEL(slow_xtal))

static sckc_registers_t * const sckc_reg =
	(sckc_registers_t *)DT_REG_ADDR(DT_NODELABEL(clk32k));

static int sckc_on(const struct device *dev, clock_control_subsys_t sys)
{
	ARG_UNUSED(dev);

	uint32_t reg = sckc_reg->SCKC_CR & ~SCKC_CR_Msk;
	const struct sam_sckc_config *cfg = sys;

	if (cfg == NULL) {
		LOG_ERR("The SCKC config can not be NULL.");
		return -ENXIO;
	}

	LOG_DBG("%s Oscillator", cfg->crystal_osc ? "Crystal" : "RC");

	if (cfg->crystal_osc) {
		reg |= SCKC_CR_TD_OSCSEL(SCKC_CR_TD_OSCSEL_XTAL_Val);
	} else {
		reg |= SCKC_CR_TD_OSCSEL(SCKC_CR_TD_OSCSEL_RC_Val);
	}
	sckc_reg->SCKC_CR = reg;

	return 0;
}

static int sckc_get_rate(const struct device *dev,
			 clock_control_subsys_t sys,
			 uint32_t *rate)
{
	ARG_UNUSED(dev);

	int ret = 0;

	const struct sam_sckc_config *cfg = sys;

	if (cfg == NULL) {
		LOG_ERR("The SCKC config can not be NULL.");
		return -ENXIO;
	}

	LOG_DBG("%s Oscillator", cfg->crystal_osc ? "Crystal" : "RC");

	if (cfg->crystal_osc) {
		ret = clock_control_get_rate(SAM_DT_SLOW_XTAL, NULL, rate);
	} else {
		*rate = KHZ(64);
	}
	LOG_DBG("Rate: %d", *rate);

	return ret;
}

static enum clock_control_status sckc_get_status(const struct device *dev,
						 clock_control_subsys_t sys)
{
	ARG_UNUSED(dev);

	const struct sam_sckc_config *cfg = sys;
	enum clock_control_status status = CLOCK_CONTROL_STATUS_OFF;

	if (cfg == NULL) {
		LOG_ERR("The SCKC config can not be NULL.");
		return -ENXIO;
	}

	LOG_DBG("%s Oscillator", cfg->crystal_osc ? "Crystal" : "RC");

	if (cfg->crystal_osc) {
		if ((sckc_reg->SCKC_CR & SCKC_CR_Msk) ==
			SCKC_CR_TD_OSCSEL(SCKC_CR_TD_OSCSEL_XTAL_Val)) {
			status = CLOCK_CONTROL_STATUS_ON;
		}
	} else {
		if ((sckc_reg->SCKC_CR & SCKC_CR_Msk) ==
			SCKC_CR_TD_OSCSEL(SCKC_CR_TD_OSCSEL_RC_Val)) {
			status = CLOCK_CONTROL_STATUS_ON;
		}
	}

	return status;
}

static DEVICE_API(clock_control, sckc_api) = {
	.on = sckc_on,
	.get_rate = sckc_get_rate,
	.get_status = sckc_get_status,
};

DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, PRE_KERNEL_1,
		      CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		      &sckc_api);
