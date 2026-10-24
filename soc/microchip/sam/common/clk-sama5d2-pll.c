/*
 * Copyright (C) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <pmc.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clk_plla, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

struct sama5d2_plla_core {
	mm_reg_t pmc;
	struct device clk;
	const struct device *parent;
};

#define to_sama5d2_plla_core(ptr) CONTAINER_OF(ptr, struct sama5d2_plla_core, clk)

static inline bool plla_ready(mm_reg_t pmc)
{
	uint32_t locka;

	locka = FIELD_GET(PMC_SR_LOCKA_Msk, sys_read32(pmc + PMC_SR_REG_OFST));

	return (locka == 1);
}

static int plla_get_rate(const struct device *dev, clock_control_subsys_t sys, uint32_t *rate)
{
	ARG_UNUSED(sys);

	struct sama5d2_plla_core *core = to_sama5d2_plla_core(dev);
	uint32_t ckgr_pllar, mula;
	int ret;

	*rate = 0;

	if (!plla_ready(core->pmc)) {
		LOG_ERR("PLA not locked");

		return -EIO;
	}

	ret = clock_control_get_rate(core->parent, NULL, rate);
	if (ret) {
		LOG_ERR("get parent clock rate failed");
	} else {
		ckgr_pllar = sys_read32(core->pmc + CKGR_PLLAR_REG_OFST);
		mula = FIELD_GET(CKGR_PLLAR_MULA_Msk, ckgr_pllar);

		if (mula == 0 || FIELD_GET(CKGR_PLLAR_DIVA_Msk, ckgr_pllar) == 0) {
			LOG_DBG("PLLA is disabled");
		} else {
			*rate *= mula + 1;
		}
	}

	LOG_DBG("clock rate of %s is %d", dev->name, *rate);

	return ret;
}

static int plla_div_get_rate(const struct device *dev, clock_control_subsys_t sys, uint32_t *rate)
{
	ARG_UNUSED(sys);

	struct sama5d2_plla_core *core = to_sama5d2_plla_core(dev);
	uint32_t mckr;
	int ret;

	ret = clock_control_get_rate(core->parent, NULL, rate);
	if (ret) {
		LOG_ERR("get parent clock rate failed");
	} else {
		mckr = sys_read32(core->pmc + PMC_MCKR_REG_OFST);

		if (FIELD_GET(PMC_MCKR_PLLADIV2_Msk, mckr) != 0) {
			*rate /= 2;
		}
	}

	LOG_DBG("clock rate of %s is %d", dev->name, *rate);

	return ret;
}

static enum clock_control_status plla_get_status(const struct device *dev,
						 clock_control_subsys_t sys)
{
	ARG_UNUSED(sys);

	struct sama5d2_plla_core *core = to_sama5d2_plla_core(dev);

	if (plla_ready(core->pmc)) {
		return CLOCK_CONTROL_STATUS_ON;
	} else {
		return CLOCK_CONTROL_STATUS_OFF;
	}
}

static DEVICE_API(clock_control, plla_api) = {
	.get_rate = plla_get_rate,
	.get_status = plla_get_status,
};

static DEVICE_API(clock_control, plla_div_api) = {
	.get_rate = plla_div_get_rate,
	.get_status = plla_get_status,
};

static struct sama5d2_plla_core sama5d2_plla_clks[] = {
	{ .clk = {.name = "plla_clk", .api = &plla_api, }, },
	{ .clk = {.name = "plla_div_clk", .api = &plla_div_api, }, },
};

int sam_pmc_register_plla(mm_reg_t reg, struct pmc_data *sama5d2_pmc, const struct device *parent,
			  struct device **clk)
{
	sama5d2_plla_clks[0].pmc = reg;
	sama5d2_plla_clks[0].parent = parent;

	sama5d2_plla_clks[1].pmc = reg;
	sama5d2_plla_clks[1].parent = &sama5d2_plla_clks[0].clk;

	*clk = &sama5d2_plla_clks[1].clk;

	return 0;
}
