/*
 * Copyright (C) 2025-2026 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <pmc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pmc_setup_sama5d2, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

/* Clock count constants */
enum clock_count {
	CLK_CNT_CORE = PMC_UPLL + 1,
	/* PCK IDs are 8..10 → table size = 11 */
	CLK_CNT_SYSTEM = 10 + 1,
	/* Highest peripheral ID is 59 */
	CLK_CNT_PERIPH = 59 + 1,
	/* Highest GCK ID is 59 */
	CLK_CNT_GCK    = 59 + 1,
	CLK_CNT_PROG = SOC_NUM_CLOCK_PROGRAMMABLE,
};

enum clock_offset {
	CLK_OFFSET_CORE = 0,
	CLK_OFFSET_SYSTEM = CLK_OFFSET_CORE + CLK_CNT_CORE,
	CLK_OFFSET_PERIPH = CLK_OFFSET_SYSTEM + CLK_CNT_SYSTEM,
	CLK_OFFSET_GCK = CLK_OFFSET_PERIPH + CLK_CNT_PERIPH,
	CLK_OFFSET_PROG = CLK_OFFSET_GCK + CLK_CNT_GCK,
};

static struct device *pmc_table[CLK_CNT_CORE + CLK_CNT_SYSTEM + CLK_CNT_PERIPH +
				CLK_CNT_GCK + CLK_CNT_PROG];

void sam_pmc_setup(const struct device *dev)
{
	const struct sam_pmc_cfg *cfg  = dev->config;
	struct sam_pmc_data *data = dev->data;

	const struct device *main_xtal = cfg->main_xtal;
	const struct device *td_slck = cfg->td_slck;
	const struct device *md_slck = cfg->md_slck;

	void *const regmap = cfg->reg;

	struct pmc_data *sama5d2_pmc;
	struct device *clk, *main_rc, *main_osc;
	const struct device *parents[2];
	uint32_t rate;
	int ret;

	if (!td_slck || !md_slck || !main_xtal || !regmap) {
		LOG_ERR("Incorrect parameters");
		return;
	}

	sama5d2_pmc = pmc_data_allocate(CLK_CNT_CORE, CLK_CNT_SYSTEM, CLK_CNT_PERIPH, CLK_CNT_GCK,
					CLK_CNT_PROG, &pmc_table[0]);

	if (!sama5d2_pmc) {
		LOG_ERR("Allocate PMC data failed");
		return;
	}

	data->pmc = sama5d2_pmc;

	sama5d2_pmc->chws[PMC_SLOW] = (struct device *)td_slck;

	ret = clk_register_main_rc_osc(regmap, "main_rc_osc", MHZ(12), &main_rc);
	if (ret) {
		LOG_ERR("Register main_rc_osc failed");
		return;
	}

	if (clock_control_get_rate(main_xtal, NULL, &rate)) {
		LOG_ERR("Get main_xtal rate failed");
		return;
	}

	ret = clk_register_main_osc(regmap, "main_osc", 0, rate, &main_osc);
	if (ret) {
		LOG_ERR("Register main_osc failed");
		return;
	}

	parents[0] = main_rc;
	parents[1] = main_osc;
	ret = clk_register_main(regmap, "mainck", parents, 2, &clk);
	if (ret) {
		LOG_ERR("Register mainck failed");
		return;
	}

	sama5d2_pmc->chws[PMC_MAIN] = clk;
}
