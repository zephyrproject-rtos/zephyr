/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * PLLA of the PMC found on SAM E70/S70/V70/V71 and PIC32CZ CA70/MC70
 * (CKGR_PLLAR). The PLL is configured at boot by the SoC code; this clock
 * reports its state and rate.
 */

#include <pmc.h>
#include <zephyr/device.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clk_plla, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

struct clk_plla {
	pmc_registers_t *pmc;
	struct device clk;
	const struct device *parent;
};

static struct clk_plla plla;

#define to_clk_plla(ptr) CONTAINER_OF(ptr, struct clk_plla, clk)

static bool clk_plla_locked(const struct clk_plla *pll)
{
	return (pll->pmc->PMC_SR & PMC_SR_LOCKA_Msk) != 0U;
}

static int clk_plla_on(const struct device *dev, clock_control_subsys_t sys)
{
	ARG_UNUSED(sys);

	return clk_plla_locked(to_clk_plla(dev)) ? 0 : -EIO;
}

static enum clock_control_status clk_plla_get_status(const struct device *dev,
						     clock_control_subsys_t sys)
{
	ARG_UNUSED(sys);

	return clk_plla_locked(to_clk_plla(dev)) ? CLOCK_CONTROL_STATUS_ON
						 : CLOCK_CONTROL_STATUS_OFF;
}

static int clk_plla_get_rate(const struct device *dev, clock_control_subsys_t sys, uint32_t *rate)
{
	ARG_UNUSED(sys);

	struct clk_plla *pll = to_clk_plla(dev);
	uint32_t pllar = pll->pmc->CKGR_PLLAR;
	uint32_t mula = (pllar & CKGR_PLLAR_MULA_Msk) >> CKGR_PLLAR_MULA_Pos;
	uint32_t diva = (pllar & CKGR_PLLAR_DIVA_Msk) >> CKGR_PLLAR_DIVA_Pos;
	uint32_t parent_rate;
	int ret;

	if (mula == 0U || diva == 0U) {
		*rate = 0U;
		return 0;
	}

	ret = clock_control_get_rate(pll->parent, NULL, &parent_rate);
	if (ret < 0) {
		return ret;
	}

	*rate = (uint32_t)((uint64_t)parent_rate * (mula + 1U) / diva);
	LOG_DBG("%s, rate %u", dev->name, *rate);

	return 0;
}

static DEVICE_API(clock_control, clk_plla_api) = {
	.on = clk_plla_on,
	.get_rate = clk_plla_get_rate,
	.get_status = clk_plla_get_status,
};

int clk_register_plla(pmc_registers_t *const pmc, const char *name, const struct device *parent,
		      struct device **clk)
{
	if (name == NULL || parent == NULL) {
		return -EINVAL;
	}

	plla.pmc = pmc;
	plla.parent = parent;
	plla.clk.name = name;
	plla.clk.api = &clk_plla_api;
	*clk = &plla.clk;

	return 0;
}
