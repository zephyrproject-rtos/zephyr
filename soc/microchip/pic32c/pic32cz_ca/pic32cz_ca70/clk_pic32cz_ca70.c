/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * PMC clock tree of the PIC32CZ CA70 for the PMC clock framework in
 * soc/microchip/sam/common. The clocks are configured at boot by soc.c; the
 * tree describes them so that drivers can enable peripheral clocks and query
 * rates.
 */

#include <pmc.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pmc_setup, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

/* Peripheral IDs below this value have no PMC_PCR entry. */
#define PERIPHERAL_ID_FIRST 7U

#define NUM_CORE   (PMC_PLLACK + 1)
#define NUM_PERIPH (ID_PERIPH_MAX + 1)

BUILD_ASSERT(NUM_PERIPH <= SOC_NUM_CLOCK_PERIPHERAL, "SOC_NUM_CLOCK_PERIPHERAL too small");

#define MAIN_RC_HZ MHZ(12)
#define SLCK_HZ    32768U

static struct device *pmc_table[NUM_CORE + NUM_PERIPH];

static struct k_spinlock pmc_pcr_lock;

static const struct clk_pcr_layout pcr_layout = {
	.offset = PMC_PCR_REG_OFST,
	.cmd = PMC_PCR_CMD_Msk,
	.gckcss_mask = PMC_PCR_GCLKCSS_Msk,
	.pid_mask = PMC_PCR_PID_Msk,
};

static const struct clk_range peripheral_range = {
	.min = 0,
	.max = MHZ(150),
};

static int slck_get_rate(const struct device *dev, clock_control_subsys_t sys, uint32_t *rate)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(sys);

	*rate = SLCK_HZ;

	return 0;
}

static DEVICE_API(clock_control, slck_api) = {
	.get_rate = slck_get_rate,
};

static struct device slck = {
	.name = "slck",
	.api = &slck_api,
};

void sam_pmc_setup(const struct device *dev)
{
	const struct sam_pmc_cfg *cfg = dev->config;
	struct sam_pmc_data *data = dev->data;
	pmc_registers_t *const regmap = (pmc_registers_t *)cfg->reg;
	const struct device *parents[3];
	struct pmc_data *pmc;
	struct device *clk, *main_rc, *main_osc;
	uint32_t rate;
	int ret;

	if (cfg->main_xtal == NULL || regmap == NULL) {
		LOG_ERR("Incorrect parameters.");
		return;
	}

	pmc = pmc_data_allocate(NUM_CORE, 0, NUM_PERIPH, 0, 0, pmc_table);
	data->pmc = pmc;

	pmc->chws[PMC_SLOW] = &slck;

	ret = clk_register_main_rc_osc(regmap, "main_rc_osc", MAIN_RC_HZ, &main_rc);
	if (ret < 0) {
		LOG_ERR("Register clock main_rc_osc failed.");
		return;
	}

	ret = clock_control_get_rate(cfg->main_xtal, NULL, &rate);
	if (ret < 0) {
		LOG_ERR("Get rate of main_xtal failed.");
		return;
	}

	ret = clk_register_main_osc(regmap, "main_osc", 0, rate, &main_osc);
	if (ret < 0) {
		LOG_ERR("Register clock main_osc failed.");
		return;
	}

	parents[0] = main_rc;
	parents[1] = main_osc;
	ret = clk_register_main(regmap, "mainck", parents, 2, &clk);
	if (ret < 0) {
		LOG_ERR("Register clock mainck failed.");
		return;
	}
	pmc->chws[PMC_MAIN] = clk;

	ret = clk_register_plla(regmap, "pllack", pmc->chws[PMC_MAIN], &clk);
	if (ret < 0) {
		LOG_ERR("Register clock pllack failed.");
		return;
	}
	pmc->chws[PMC_PLLACK] = clk;

	/* PMC_MCKR.CSS: slow clock, main clock, PLLA (UPLL not described). */
	parents[0] = pmc->chws[PMC_SLOW];
	parents[1] = pmc->chws[PMC_MAIN];
	parents[2] = pmc->chws[PMC_PLLACK];
	ret = clk_register_master_mckr_pres(regmap, "masterck_pres", parents, 3, &clk);
	if (ret < 0) {
		LOG_ERR("Register clock masterck_pres failed.");
		return;
	}

	ret = clk_register_master_mckr_div(regmap, "masterck_div", clk, &clk);
	if (ret < 0) {
		LOG_ERR("Register clock masterck_div failed.");
		return;
	}
	pmc->chws[PMC_MCK] = clk;

	for (uint32_t id = PERIPHERAL_ID_FIRST; id < NUM_PERIPH; id++) {
		ret = clk_register_peripheral(regmap, &pmc_pcr_lock, &pcr_layout, "periph",
					      pmc->chws[PMC_MCK], id, &peripheral_range, &clk);
		if (ret < 0) {
			LOG_ERR("Register peripheral clock %u failed.", id);
			return;
		}
		pmc->phws[id] = clk;
	}
}
