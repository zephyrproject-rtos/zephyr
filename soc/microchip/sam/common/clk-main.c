/*
 * Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pmc.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clk_main, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#define SLOW_CLOCK_FREQ		32768
#define MAINF_DIV		16

#define clk_main_parent_select(s)	(((s) & \
					(CKGR_MOR_MOSCXTEN_Msk | \
					AT91_PMC_OSCBYPASS)) ? 1 : 0)

struct clk_main_osc {
	struct device clk;

	pmc_registers_t *pmc;
	uint32_t frequency;
};

#define to_clk_main_osc(ptr) CONTAINER_OF(ptr, struct clk_main_osc, clk)

struct clk_main_rc_osc {
	struct device clk;

	pmc_registers_t *pmc;
	uint32_t frequency;
};

#define to_clk_main_rc_osc(ptr) CONTAINER_OF(ptr, struct clk_main_rc_osc, clk)

struct clk_main {
	struct device clk;

	pmc_registers_t *pmc;
	struct device *parents[2];
	uint8_t num_parents;
	uint8_t parent;
};

#define to_clk_main(ptr) CONTAINER_OF(ptr, struct clk_main, clk)

static struct clk_main_osc main_osc;
static struct clk_main_rc_osc main_rc_osc;
static struct clk_main clocks_main;

static int main_osc_on(const struct device *dev, clock_control_subsys_t sys)
{
	ARG_UNUSED(sys);

	struct clk_main_osc *osc = to_clk_main_osc(dev);
	pmc_registers_t *pmc = osc->pmc;
	uint32_t tmp, status;

	tmp = pmc->CKGR_MOR & ~CKGR_MOR_KEY_Msk;

	if (tmp & AT91_PMC_OSCBYPASS) {
		return 0;
	}

	if (!(tmp & CKGR_MOR_MOSCXTEN_Msk)) {
		tmp |= CKGR_MOR_MOSCXTEN_Msk | CKGR_MOR_KEY_PASSWD;
		pmc->CKGR_MOR = tmp;
	}

	do {
		status = pmc->PMC_SR;
	} while (!(status & CKGR_MOR_MOSCXTEN_Msk));

	return 0;
}

static int main_osc_off(const struct device *dev, clock_control_subsys_t sys)
{
	ARG_UNUSED(sys);

	struct clk_main_osc *osc = to_clk_main_osc(dev);
	pmc_registers_t *pmc = osc->pmc;
	uint32_t tmp;

	tmp = pmc->CKGR_MOR;
	if (!(tmp & AT91_PMC_OSCBYPASS) && (tmp & CKGR_MOR_MOSCXTEN_Msk)) {
		tmp &= ~(CKGR_MOR_KEY_PASSWD | CKGR_MOR_MOSCXTEN_Msk);
		pmc->CKGR_MOR = tmp | CKGR_MOR_KEY_PASSWD;
	}

	return 0;
}

static int main_osc_get_rate(const struct device *dev, clock_control_subsys_t sys, uint32_t *rate)
{
	ARG_UNUSED(sys);

	struct clk_main_osc *osc = to_clk_main_osc(dev);

	*rate = osc->frequency;

	return 0;
}

static enum clock_control_status main_osc_get_status(const struct device *dev,
						     clock_control_subsys_t sys)
{
	ARG_UNUSED(sys);

	struct clk_main_osc *osc = to_clk_main_osc(dev);
	pmc_registers_t *pmc = osc->pmc;
	uint32_t tmp, status;

	tmp = pmc->CKGR_MOR;
	if (tmp & AT91_PMC_OSCBYPASS) {
		return CLOCK_CONTROL_STATUS_ON;
	}

	status = pmc->PMC_SR;
	if ((status & PMC_SR_MOSCXTS_Msk) && clk_main_parent_select(tmp)) {
		return CLOCK_CONTROL_STATUS_ON;
	} else {
		return CLOCK_CONTROL_STATUS_OFF;
	}
}

static DEVICE_API(clock_control, main_osc_api) = {
	.on = main_osc_on,
	.off = main_osc_off,
	.get_rate = main_osc_get_rate,
	.get_status = main_osc_get_status,
};

int clk_register_main_osc(pmc_registers_t *const pmc, const char *name, uint32_t bypass,
			  uint32_t frequency, struct device **clk)
{
	struct clk_main_osc *osc;

	if (!name) {
		return -EINVAL;
	}

	osc = &main_osc;

	*clk = &osc->clk;
	(*clk)->name = name;
	(*clk)->api = &main_osc_api;
	osc->pmc = pmc;
	osc->frequency = frequency;

	if (bypass) {
		reg_update_bits(pmc->CKGR_MOR, CKGR_MOR_KEY_Msk | AT91_PMC_OSCBYPASS,
				AT91_PMC_OSCBYPASS | CKGR_MOR_KEY_PASSWD);
	}

	return 0;
}

static int main_rc_osc_on(const struct device *dev, clock_control_subsys_t sys)
{
	ARG_UNUSED(sys);

	struct clk_main_rc_osc *osc = to_clk_main_rc_osc(dev);
	pmc_registers_t *pmc = osc->pmc;
	uint32_t mor, status;

	mor = pmc->CKGR_MOR;

	if (!(mor & CKGR_MOR_MOSCRCEN_Msk)) {
		reg_update_bits(pmc->CKGR_MOR,
				CKGR_MOR_KEY_Msk | CKGR_MOR_MOSCRCEN_Msk,
				CKGR_MOR_MOSCRCEN_Msk | CKGR_MOR_KEY_PASSWD);
	}

	do {
		status = pmc->PMC_SR;
	} while (!(status & PMC_SR_MOSCRCS_Msk));

	return 0;
}

static int main_rc_osc_off(const struct device *dev, clock_control_subsys_t sys)
{
	ARG_UNUSED(sys);

	struct clk_main_rc_osc *osc = to_clk_main_rc_osc(dev);
	pmc_registers_t *pmc = osc->pmc;

	if (pmc->CKGR_MOR & CKGR_MOR_MOSCRCEN_Msk) {
		reg_update_bits(pmc->CKGR_MOR,
				CKGR_MOR_KEY_Msk | CKGR_MOR_MOSCRCEN_Msk, CKGR_MOR_KEY_PASSWD);
	}

	return 0;
}

static int main_rc_osc_get_rate(const struct device *dev,
				clock_control_subsys_t sys, uint32_t *rate)
{
	ARG_UNUSED(sys);

	struct clk_main_rc_osc *osc = to_clk_main_rc_osc(dev);

	*rate = osc->frequency;

	return 0;
}

static enum clock_control_status main_rc_osc_get_status(const struct device *dev,
							clock_control_subsys_t sys)
{
	ARG_UNUSED(sys);

	struct clk_main_rc_osc *osc = to_clk_main_rc_osc(dev);
	pmc_registers_t *pmc = osc->pmc;

	if ((pmc->CKGR_MOR & CKGR_MOR_MOSCRCEN_Msk) && (pmc->PMC_SR & PMC_SR_MOSCRCS_Msk)) {
		return CLOCK_CONTROL_STATUS_ON;
	} else {
		return CLOCK_CONTROL_STATUS_OFF;
	}
}

static DEVICE_API(clock_control, main_rc_osc_api) = {
	.on = main_rc_osc_on,
	.off = main_rc_osc_off,
	.get_rate = main_rc_osc_get_rate,
	.get_status = main_rc_osc_get_status,
};

int clk_register_main_rc_osc(pmc_registers_t *const pmc, const char *name,
			     uint32_t frequency, struct device **clk)
{
	struct clk_main_rc_osc *osc;

	if (!name || !frequency) {
		return -EINVAL;
	}

	osc = &main_rc_osc;

	*clk = &osc->clk;
	(*clk)->api = &main_rc_osc_api;
	(*clk)->name = name;
	osc->pmc = pmc;
	osc->frequency = frequency;

	return 0;
}

static int clk_main_probe_frequency(pmc_registers_t *pmc)
{
	uint32_t timeout = 0;

	do {
		if (pmc->CKGR_MCFR & CKGR_MCFR_MAINFRDY_Msk) {
			return 0;
		}
		k_busy_wait(10);
	} while (timeout++ < 10);
	LOG_WRN("Wait Main Clock Frequency Measure Ready timeout.");

	return -ETIMEDOUT;
}

static inline bool clk_main_ready(pmc_registers_t *pmc)
{
	return !!(pmc->PMC_SR & PMC_SR_MOSCSELS_Msk);
}

static int clk_main_on(const struct device *dev, clock_control_subsys_t sys)
{
	ARG_UNUSED(sys);

	struct clk_main *clkmain = to_clk_main(dev);
	pmc_registers_t *pmc = clkmain->pmc;

	while (!clk_main_ready(pmc)) {
		k_busy_wait(1);
	}

	return clk_main_probe_frequency(pmc);
}

static int clk_main_get_rate(const struct device *dev, clock_control_subsys_t sys, uint32_t *rate)
{
	ARG_UNUSED(sys);

	struct clk_main *clkmain = to_clk_main(dev);
	pmc_registers_t *pmc = clkmain->pmc;
	uint32_t mcfr;

	if (clkmain->parent == 1) {
		return clock_control_get_rate(clkmain->parents[1], NULL, rate);
	}

	mcfr = pmc->CKGR_MCFR;
	if (!(mcfr & CKGR_MCFR_MAINFRDY_Msk)) {
		*rate = 0;
	} else {
		*rate = ((mcfr & CKGR_MCFR_MAINF_Msk) * SLOW_CLOCK_FREQ) / MAINF_DIV;
	}

	return 0;
}

static enum clock_control_status clk_main_get_status(const struct device *dev,
						     clock_control_subsys_t sys)
{
	ARG_UNUSED(sys);

	struct clk_main *clkmain = to_clk_main(dev);

	if (clk_main_ready(clkmain->pmc)) {
		return CLOCK_CONTROL_STATUS_ON;
	} else {
		return CLOCK_CONTROL_STATUS_OFF;
	}
}

static DEVICE_API(clock_control, main_api) = {
	.on = clk_main_on,
	.get_rate = clk_main_get_rate,
	.get_status = clk_main_get_status,
};

int clk_register_main(pmc_registers_t *const pmc, const char *name,
		      const struct device **parents, int num_parents, struct device **clk)
{
	struct clk_main *clkmain;
	unsigned int status;

	if (!name || !parents || !num_parents) {
		return -EINVAL;
	}

	clkmain = &clocks_main;
	if (num_parents > ARRAY_SIZE(clkmain->parents)) {
		LOG_ERR("Array for parent clock not enough");
		return -ENOMEM;
	}
	memcpy(clkmain->parents, parents, sizeof(struct device *) * num_parents);

	*clk = &clkmain->clk;
	(*clk)->name = name;
	(*clk)->api = &main_api;
	clkmain->num_parents = num_parents;
	clkmain->pmc = pmc;
	status = pmc->CKGR_MOR;
	clkmain->parent = clk_main_parent_select(status);

	return 0;
}
