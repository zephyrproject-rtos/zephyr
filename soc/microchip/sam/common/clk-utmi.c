/*
 * Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pmc.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clk_utmi, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

/*
 * The purpose of this clock is to generate a 480 MHz signal. A different
 * rate can't be configured.
 */
#define UTMI_RATE	MHZ(48)

struct clk_utmi {
	struct device clk;
	const struct device *parent;
	pmc_registers_t *pmc;
};

#define to_clk_utmi(ptr) CONTAINER_OF(ptr, struct clk_utmi, clk)

static struct clk_utmi clock_utmi;

static int clk_utmi_sama7g5_get_rate(const struct device *dev,
				     clock_control_subsys_t sys, uint32_t *rate)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(sys);

	/* UTMI clk rate is fixed. */
	*rate = UTMI_RATE;

	return 0;
}

static int at91_clk_register_utmi_internal(pmc_registers_t *const pmc, const char *name,
					   const struct device *parent, const void *api,
					   struct device **clk)
{
	struct clk_utmi *utmi = &clock_utmi;

	if (!parent) {
		return -EINVAL;
	}

	*clk = &utmi->clk;
	(*clk)->name = name;
	(*clk)->api = api;
	utmi->parent = parent;
	utmi->pmc = pmc;

	return 0;
}

static int clk_utmi_sama7g5_on(const struct device *dev, clock_control_subsys_t sys)
{
	ARG_UNUSED(sys);

	struct clk_utmi *utmi = to_clk_utmi(dev);
	uint32_t parent_rate;
	uint32_t val;
	int ret;

	ret = clock_control_get_rate(utmi->parent, NULL, &parent_rate);
	if (ret) {
		LOG_ERR("get parent clock rate failed.");
		return ret;
	}

	switch (parent_rate) {
	case MHZ(16):
		val = 0;
		break;
	case MHZ(20):
		val = 2;
		break;
	case MHZ(24):
		val = 3;
		break;
	case MHZ(32):
		val = 5;
		break;
	default:
		LOG_ERR("UTMICK: unsupported main_xtal rate\n");
		return -EINVAL;
	}

	utmi->pmc->PMC_XTALF = val;

	return 0;

}

static enum clock_control_status clk_utmi_sama7g5_get_status(const struct device *dev,
							     clock_control_subsys_t sys)
{
	ARG_UNUSED(sys);

	struct clk_utmi *utmi = to_clk_utmi(dev);
	uint32_t parent_rate;
	int ret;

	ret = clock_control_get_rate(utmi->parent, NULL, &parent_rate);
	if (ret) {
		LOG_ERR("get parent clock rate failed.");
		return ret;
	}

	switch (utmi->pmc->PMC_XTALF & 0x7) {
	case 0:
		if (parent_rate == MHZ(16)) {
			return CLOCK_CONTROL_STATUS_ON;
		}
		break;
	case 2:
		if (parent_rate == MHZ(20)) {
			return CLOCK_CONTROL_STATUS_ON;
		}
		break;
	case 3:
		if (parent_rate == MHZ(24)) {
			return CLOCK_CONTROL_STATUS_ON;
		}
		break;
	case 5:
		if (parent_rate == MHZ(32)) {
			return CLOCK_CONTROL_STATUS_ON;
		}
		break;
	default:
		break;
	}

	return CLOCK_CONTROL_STATUS_UNKNOWN;
}

static DEVICE_API(clock_control, sama7g5_utmi_api) = {
	.on = clk_utmi_sama7g5_on,
	.get_rate = clk_utmi_sama7g5_get_rate,
	.get_status = clk_utmi_sama7g5_get_status,
};

int clk_register_utmi(pmc_registers_t *const pmc, const char *name,
		      const struct device *parent, struct device **clk)
{
	return at91_clk_register_utmi_internal(pmc, name, parent, &sama7g5_utmi_api, clk);
}
