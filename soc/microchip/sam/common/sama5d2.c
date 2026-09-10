/*
 * Copyright (C) 2025-2026 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <pmc.h>

#include <zephyr/spinlock.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pmc_setup_sama5d2, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

static struct k_spinlock pmc_mck_lock;

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

static const struct clk_master_layout mck_layout = {
	.offset = PMC_MCKR_REG_OFST,
	.css_msk = PMC_MCKR_CSS_Msk,
	.pres_shift = PMC_MCKR_PRES_Pos,
};

static const struct clk_master_characteristics mck_characteristics = {
	.output = { .min = 32768, .max = MHZ(166), },
	.divisors = { 1, 2, 4, 3, 0},
	.have_div3_pres = 1,
};

static int sam_pmc_register_master(const struct device *dev, struct pmc_data *sama5d2_pmc,
				   struct device **clk)
{
	const struct sam_pmc_cfg *cfg = dev->config;
	const struct device *parents[4];
	void *const regmap = cfg->reg;
	struct device *mck_pres;
	int ret;

	parents[0] = cfg->md_slck;
	parents[1] = sama5d2_pmc->chws[PMC_MAIN];
	parents[2] = sama5d2_pmc->chws[PMC_PLLA];
	parents[3] = sama5d2_pmc->chws[PMC_UPLL];
	ret = clk_register_master_pres(regmap, "masterck_pres", parents, ARRAY_SIZE(parents),
				       &mck_layout, &mck_characteristics, &pmc_mck_lock, &mck_pres);
	if (ret) {
		LOG_ERR("Register clock masterck_pres failed");
		return ret;
	}

	ret = clk_register_master_div(regmap, "mck", mck_pres, &mck_layout, &mck_characteristics,
				      &pmc_mck_lock, 0, clk);
	if (ret) {
		LOG_ERR("Register MCK div clock failed");
		return ret;
	}

	return 0;
}

static struct clock_matrix {
	mm_reg_t pmc;
	struct device clk;
	const struct device *parent;
} sama5d2_h32mx_ck;

static int clk_mck_4_h32mx_get_rate(const struct device *dev, clock_control_subsys_t sys,
				    uint32_t *rate)
{
	struct clock_matrix *h32mxclk = CONTAINER_OF(dev, struct clock_matrix, clk);
	uint32_t h32mxdiv;
	int retval;

	retval = clock_control_get_rate(h32mxclk->parent, NULL, rate);
	if (retval) {
		LOG_ERR("get parent clock rate failed");
	} else {
		h32mxdiv = FIELD_GET(PMC_MCKR_H32MXDIV_Msk,
				     sys_read32(h32mxclk->pmc + PMC_MCKR_REG_OFST));

		if (h32mxdiv != 0) {
			*rate /= 2;
		}
	}

	return retval;
}

static DEVICE_API(clock_control, clk_mck_4_h32mx_api) = {
	.get_rate = clk_mck_4_h32mx_get_rate,
};

static void clk_register_h32mx(mm_reg_t pmc, const char *name, const struct device *parent,
			       struct device **clk)
{
	struct clock_matrix *h32mxclk = &sama5d2_h32mx_ck;

	h32mxclk->pmc = pmc;
	h32mxclk->clk.name = name;
	h32mxclk->clk.api = &clk_mck_4_h32mx_api;
	h32mxclk->parent = parent;
	*clk = &h32mxclk->clk;
}

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

	ret = sam_pmc_register_plla((mm_reg_t)regmap, sama5d2_pmc, sama5d2_pmc->chws[PMC_MAIN],
				    &sama5d2_pmc->chws[PMC_PLLA]);
	if (ret) {
		return;
	}

	ret = sam_pmc_register_master(dev, sama5d2_pmc, &sama5d2_pmc->chws[PMC_MCK]);
	if (ret) {
		return;
	}

	clk_register_h32mx((mm_reg_t)regmap, "h32mxck", sama5d2_pmc->chws[PMC_MCK],
			   &sama5d2_pmc->chws[PMC_MCK2]);
}
