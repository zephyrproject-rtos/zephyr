/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Master clock of the PMC found on SAM E70/S70/V70/V71 and PIC32CZ CA70/MC70
 * (PMC_MCKR). PMC_MCKR.CSS selects the source, PMC_MCKR.PRES divides it into
 * the processor clock and PMC_MCKR.MDIV divides the processor clock into MCK.
 * The master clock is configured at boot by the SoC code; these clocks report
 * their state and rate.
 */

#include <pmc.h>
#include <zephyr/device.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clk_master_mckr, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#define MCKR_NUM_PARENTS 4U

struct clk_mckr {
	pmc_registers_t *pmc;
	struct device clk;
	const struct device *parents[MCKR_NUM_PARENTS];
};

static struct clk_mckr mckr_pres;
static struct clk_mckr mckr_div;

#define to_clk_mckr(ptr) CONTAINER_OF(ptr, struct clk_mckr, clk)

static bool clk_mckr_ready(const struct clk_mckr *mckr)
{
	return (mckr->pmc->PMC_SR & PMC_SR_MCKRDY_Msk) != 0U;
}

static int clk_mckr_on(const struct device *dev, clock_control_subsys_t sys)
{
	ARG_UNUSED(sys);

	return clk_mckr_ready(to_clk_mckr(dev)) ? 0 : -EIO;
}

static enum clock_control_status clk_mckr_get_status(const struct device *dev,
						     clock_control_subsys_t sys)
{
	ARG_UNUSED(sys);

	return clk_mckr_ready(to_clk_mckr(dev)) ? CLOCK_CONTROL_STATUS_ON
						: CLOCK_CONTROL_STATUS_OFF;
}

static int clk_mckr_pres_get_rate(const struct device *dev, clock_control_subsys_t sys,
				  uint32_t *rate)
{
	ARG_UNUSED(sys);

	struct clk_mckr *mckr = to_clk_mckr(dev);
	uint32_t reg = mckr->pmc->PMC_MCKR;
	uint32_t css = (reg & PMC_MCKR_CSS_Msk) >> PMC_MCKR_CSS_Pos;
	uint32_t pres = (reg & PMC_MCKR_PRES_Msk) >> PMC_MCKR_PRES_Pos;
	uint32_t parent_rate;
	int ret;

	if (mckr->parents[css] == NULL) {
		return -ENOTSUP;
	}

	ret = clock_control_get_rate(mckr->parents[css], NULL, &parent_rate);
	if (ret < 0) {
		return ret;
	}

	/* PRES 0..6 divides by 2^PRES, PRES 7 divides by 3. */
	*rate = (pres == 7U) ? (parent_rate / 3U) : (parent_rate >> pres);
	LOG_DBG("%s, rate %u", dev->name, *rate);

	return 0;
}

static int clk_mckr_div_get_rate(const struct device *dev, clock_control_subsys_t sys,
				 uint32_t *rate)
{
	ARG_UNUSED(sys);

	/* MDIV 0, 1, 2, 3 divides by 1, 2, 4, 3. */
	static const uint8_t divisors[] = {1U, 2U, 4U, 3U};
	struct clk_mckr *mckr = to_clk_mckr(dev);
	uint32_t mdiv = (mckr->pmc->PMC_MCKR & PMC_MCKR_MDIV_Msk) >> PMC_MCKR_MDIV_Pos;
	uint32_t parent_rate;
	int ret;

	ret = clock_control_get_rate(mckr->parents[0], NULL, &parent_rate);
	if (ret < 0) {
		return ret;
	}

	*rate = parent_rate / divisors[mdiv];
	LOG_DBG("%s, rate %u", dev->name, *rate);

	return 0;
}

static DEVICE_API(clock_control, clk_mckr_pres_api) = {
	.on = clk_mckr_on,
	.get_rate = clk_mckr_pres_get_rate,
	.get_status = clk_mckr_get_status,
};

static DEVICE_API(clock_control, clk_mckr_div_api) = {
	.on = clk_mckr_on,
	.get_rate = clk_mckr_div_get_rate,
	.get_status = clk_mckr_get_status,
};

int clk_register_master_mckr_pres(pmc_registers_t *const pmc, const char *name,
				  const struct device **parents, uint8_t num_parents,
				  struct device **clk)
{
	if (name == NULL || parents == NULL || num_parents == 0U ||
	    num_parents > MCKR_NUM_PARENTS) {
		return -EINVAL;
	}

	mckr_pres.pmc = pmc;
	for (uint8_t i = 0U; i < num_parents; i++) {
		mckr_pres.parents[i] = parents[i];
	}
	mckr_pres.clk.name = name;
	mckr_pres.clk.api = &clk_mckr_pres_api;
	*clk = &mckr_pres.clk;

	return 0;
}

int clk_register_master_mckr_div(pmc_registers_t *const pmc, const char *name,
				 const struct device *parent, struct device **clk)
{
	if (name == NULL || parent == NULL) {
		return -EINVAL;
	}

	mckr_div.pmc = pmc;
	mckr_div.parents[0] = parent;
	mckr_div.clk.name = name;
	mckr_div.clk.api = &clk_mckr_div_api;
	*clk = &mckr_div.clk;

	return 0;
}
