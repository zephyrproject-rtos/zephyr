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

#define nck(a) (a[ARRAY_SIZE(a) - 1].id + 1)

static struct k_spinlock pmc_mck_lock;
static struct k_spinlock pmc_pcr_lock;

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

static const struct clk_programmable_layout programmable_layout = {
	.pres_mask      = PMC_PCK_PRES_Msk >> PMC_PCK_PRES_Pos,
	.pres_shift     = PMC_PCK_PRES_Pos,
	.css_mask       = PMC_PCK_CSS_Msk,
	.have_slck_mck  = 0,
	.is_pres_direct = 1,
};

static const struct {
	char *n;
	char *p;
	uint32_t parent_idx;
	uint8_t id;
} sama5d2_systemck[] = {
	{ .n = "pck0", .p = "prog0", .id = PMC_SCER_PCK0_Pos, .parent_idx = CLK_OFFSET_PROG + 0 },
	{ .n = "pck1", .p = "prog1", .id = PMC_SCER_PCK1_Pos, .parent_idx = CLK_OFFSET_PROG + 1 },
	{ .n = "pck2", .p = "prog2", .id = PMC_SCER_PCK2_Pos, .parent_idx = CLK_OFFSET_PROG + 2 },
};

/*
 * Peripheral clock PCR layout
 */
static const struct clk_pcr_layout sama5d2_pcr_layout = {
	.offset      = PMC_PCR_REG_OFST,
	.cmd         = PMC_PCR_CMD_Msk,
	.gckcss_mask = PMC_PCR_GCKCSS_Msk,
	.pid_mask    = PMC_PCR_PID_Msk,
};

/*
 * Peripheral clock descriptors
 *
 * ref: SAMA5D2 datasheet (DS60001476J) Table 18-9 (Peripheral Identifiers).
 * The clock parents are MCK (AHB 64-bit MATRIX system) or MCK2 (AHB 32-bit MATRIX system).
 */
static struct {
	const char *n;
	struct clk_range r;
	uint8_t id;
	bool in_h64mx;
} sama5d2_peripheral_clk[] = {
	{ .n = "gmac_clk",     .id = 5, },
	{ .n = "xdmac0_clk",   .id = 6,  .in_h64mx = true, },
	{ .n = "xdmac1_clk",   .id = 7,  .in_h64mx = true, },
	{ .n = "icm_clk",      .id = 8, },
	{ .n = "aes_clk",      .id = 9,  .in_h64mx = true, },
	{ .n = "aesb_clk",     .id = 10, .in_h64mx = true, },
	{ .n = "tdes_clk",     .id = 11, },
	{ .n = "sha_clk",      .id = 12, .in_h64mx = true, },
	{ .n = "mpddrc_clk",   .id = 13, .in_h64mx = true, },
	{ .n = "matrix1_clk",  .id = 14, },
	{ .n = "matrix0_clk",  .id = 15, },
	{ .n = "secumod_clk",  .id = 16, },
	{ .n = "hsmc_clk",     .id = 17, },
	{ .n = "pioa_clk",     .id = 18, },
	{ .n = "flexcom0_clk", .id = 19, },
	{ .n = "flexcom1_clk", .id = 20, },
	{ .n = "flexcom2_clk", .id = 21, },
	{ .n = "flexcom3_clk", .id = 22, },
	{ .n = "flexcom4_clk", .id = 23, },
	{ .n = "uart0_clk",    .id = 24, },
	{ .n = "uart1_clk",    .id = 25, },
	{ .n = "uart2_clk",    .id = 26, },
	{ .n = "uart3_clk",    .id = 27, },
	{ .n = "uart4_clk",    .id = 28, },
	{ .n = "twi0_clk",     .id = 29, },
	{ .n = "twi1_clk",     .id = 30, },
	{ .n = "sdmmc0_clk",   .id = 31, .in_h64mx = true, .r = { .max = MHZ(208) }, },
	{ .n = "sdmmc1_clk",   .id = 32, .in_h64mx = true, .r = { .max = MHZ(208) }, },
	{ .n = "spi0_clk",     .id = 33, },
	{ .n = "spi1_clk",     .id = 34, },
	{ .n = "tc0_clk",      .id = 35, .r = { .max = MHZ(166) }, },
	{ .n = "tc1_clk",      .id = 36, .r = { .max = MHZ(166) }, },
	{ .n = "pwm_clk",      .id = 38, },
	{ .n = "adc_clk",      .id = 40, .r = { .max = MHZ(83) }, },
	{ .n = "uhphs_clk",    .id = 41, },
	{ .n = "udphs_clk",    .id = 42, },
	{ .n = "ssc0_clk",     .id = 43, .r = { .max = MHZ(83) }, },
	{ .n = "ssc1_clk",     .id = 44, .r = { .max = MHZ(83) }, },
	{ .n = "lcdc_clk",     .id = 45, .in_h64mx = true, },
	{ .n = "isi_clk",      .id = 46, .in_h64mx = true, },
	{ .n = "trng_clk",     .id = 47, },
	{ .n = "pdmic_clk",    .id = 48, .r = { .max = MHZ(83) }, },
	{ .n = "securam_clk",  .id = 51, },
	{ .n = "qspi0_clk",    .id = 52, .in_h64mx = true, },
	{ .n = "qspi1_clk",    .id = 53, .in_h64mx = true, },
	{ .n = "i2sc0_clk",    .id = 54, .r = { .max = MHZ(83) }, },
	{ .n = "i2sc1_clk",    .id = 55, .r = { .max = MHZ(83) }, },
	{ .n = "can0_clk",     .id = 56, .r = { .max = MHZ(83) }, },
	{ .n = "can1_clk",     .id = 57, .r = { .max = MHZ(83) }, },
	{ .n = "ptc_clk",      .id = 58, },
	{ .n = "classd_clk",   .id = 59, .r = { .max = MHZ(83) }, },
};

/*
 * Generic clock (GCK) descriptors
 *
 * SAMA5D2 GCK parents (via PMC_PCR GCKCSS field, 3-bit):
 *   0: Slow clock  (md_slck)
 *   1: Main clock  (mainck)
 *   2: PLLACK
 *   3: UPLL clock
 *   4: Main System Bus clock
 *   3: Audio PLL clock
 */
static const struct {
	const char *n;
	uint8_t id;
} sama5d2_gck[] = {
	{ .n = "gmac_gclk",     .id = 5, },
	{ .n = "xdmac0_gclk",   .id = 6, },
	{ .n = "xdmac1_gclk",   .id = 7, },
	{ .n = "icm_gclk",      .id = 8, },
	{ .n = "aes_gclk",      .id = 9, },
	{ .n = "aesb_gclk",     .id = 10, },
	{ .n = "tdes_gclk",     .id = 11, },
	{ .n = "sha_gclk",      .id = 12, },
	{ .n = "mpddrc_gclk",   .id = 13, },
	{ .n = "matrix1_gclk",  .id = 14, },
	{ .n = "matrix0_gclk",  .id = 15, },
	{ .n = "secumod_gclk",  .id = 16, },
	{ .n = "hsmc_gclk",     .id = 17, },
	{ .n = "pioa_gclk",     .id = 18, },
	{ .n = "flexcom0_gclk", .id = 19, },
	{ .n = "flexcom1_gclk", .id = 20, },
	{ .n = "flexcom2_gclk", .id = 21, },
	{ .n = "flexcom3_gclk", .id = 22, },
	{ .n = "flexcom4_gclk", .id = 23, },
	{ .n = "uart0_gclk",    .id = 24, },
	{ .n = "uart1_gclk",    .id = 25, },
	{ .n = "uart2_gclk",    .id = 26, },
	{ .n = "uart3_gclk",    .id = 27, },
	{ .n = "uart4_gclk",    .id = 28, },
	{ .n = "twi0_gclk",     .id = 29, },
	{ .n = "twi1_gclk",     .id = 30, },
	{ .n = "sdmmc0_gclk",   .id = 31, },
	{ .n = "sdmmc1_gclk",   .id = 32, },
	{ .n = "spi0_gclk",     .id = 33, },
	{ .n = "spi1_gclk",     .id = 34, },
	{ .n = "tc0_gclk",      .id = 35, },
	{ .n = "tc1_gclk",      .id = 36, },
	{ .n = "pwm_gclk",      .id = 38, },
	{ .n = "adc_gclk",      .id = 40, },
	{ .n = "uhphs_gclk",    .id = 41, },
	{ .n = "udphs_gclk",    .id = 42, },
	{ .n = "ssc0_gclk",     .id = 43, },
	{ .n = "ssc1_gclk",     .id = 44, },
	{ .n = "lcdc_gclk",     .id = 45, },
	{ .n = "isi_gclk",      .id = 46, },
	{ .n = "trng_gclk",     .id = 47, },
	{ .n = "pdmic_gclk",    .id = 48, },
	{ .n = "securam_gclk",  .id = 51, },
	{ .n = "qspi0_gclk",    .id = 52, },
	{ .n = "qspi1_gclk",    .id = 53, },
	{ .n = "i2sc0_gclk",    .id = 54, },
	{ .n = "i2sc1_gclk",    .id = 55, },
	{ .n = "can0_gclk",     .id = 56, },
	{ .n = "can1_gclk",     .id = 57, },
	{ .n = "ptc_gclk",      .id = 58, },
	{ .n = "classd_gclk",   .id = 59, },
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

static int sam_pmc_register_programmable(const struct device *dev, struct pmc_data *sama5d2_pmc)
{
	const struct sam_pmc_cfg *cfg = dev->config;
	const struct device *parents[6];
	struct device *clk;
	int ret, i;

	parents[0] = cfg->md_slck;
	parents[1] = sama5d2_pmc->chws[PMC_MAIN];
	parents[2] = sama5d2_pmc->chws[PMC_PLLA];
	parents[3] = sama5d2_pmc->chws[PMC_UPLL];
	parents[4] = sama5d2_pmc->chws[PMC_MCK];
	parents[5] = sama5d2_pmc->chws[PMC_AUDIOPMCPLL];

	for (i = 0; i < SOC_NUM_CLOCK_PROGRAMMABLE; i++) {
		ret = clk_register_programmable((void *const)cfg->reg, parents, ARRAY_SIZE(parents),
						i, &programmable_layout, NULL, &clk);
		if (ret) {
			LOG_ERR("Register programmable clock %d failed", i);
			return ret;
		}

		sama5d2_pmc->pchws[i] = clk;
	}

	return 0;
}

static int sam_pmc_register_system(const struct device *dev, struct pmc_data *sama5d2_pmc)
{
	const struct sam_pmc_cfg *cfg = dev->config;
	struct device *clk;
	int ret, i;

	for (i = 0; i < ARRAY_SIZE(sama5d2_systemck); i++) {
		ret = clk_register_system((void *const)cfg->reg, sama5d2_systemck[i].n,
					  pmc_table[sama5d2_systemck[i].parent_idx],
					  sama5d2_systemck[i].id, &clk);
		if (ret) {
			LOG_ERR("Register system clock %d failed", i);
			return ret;
		}

		sama5d2_pmc->shws[sama5d2_systemck[i].id] = clk;
	}

	return 0;
}

static int sam_pmc_register_peripheral(const struct device *dev, struct pmc_data *sama5d2_pmc)
{
	const struct sam_pmc_cfg *cfg = dev->config;
	struct device *parent;
	struct device *clk;
	int ret, i;

	for (i = 0; i < ARRAY_SIZE(sama5d2_peripheral_clk); i++) {
		if (sama5d2_peripheral_clk[i].in_h64mx) {
			parent = sama5d2_pmc->chws[PMC_MCK];
		} else {
			parent = sama5d2_pmc->chws[PMC_MCK2];
		}

		ret = clk_register_peripheral((void *const)cfg->reg, &pmc_pcr_lock,
					      &sama5d2_pcr_layout,
					      sama5d2_peripheral_clk[i].n,
					      parent,
					      sama5d2_peripheral_clk[i].id,
					      NULL,
					      &clk);
		if (ret) {
			LOG_ERR("Register peripheral clock %d failed", i);
			return ret;
		}

		sama5d2_pmc->phws[sama5d2_peripheral_clk[i].id] = clk;
	}

	return 0;
}

static int sam_pmc_register_generated(const struct device *dev, struct pmc_data *sama5d2_pmc)
{
	const struct sam_pmc_cfg *cfg = dev->config;
	uint32_t mux_table[] = {0, 1, 2, 3, 4, 5};
	const struct device *parents[6];
	struct device *clk;
	int ret, i;

	parents[0] = cfg->md_slck;
	parents[1] = sama5d2_pmc->chws[PMC_MAIN];
	parents[2] = sama5d2_pmc->chws[PMC_PLLA];
	parents[3] = sama5d2_pmc->chws[PMC_UPLL];
	parents[4] = sama5d2_pmc->chws[PMC_MCK];
	parents[5] = sama5d2_pmc->chws[PMC_AUDIOPMCPLL];

	for (i = 0; i < ARRAY_SIZE(sama5d2_gck); i++) {
		ret = clk_register_generated((void *const)cfg->reg, &pmc_pcr_lock,
					     &sama5d2_pcr_layout, sama5d2_gck[i].n, parents,
					     mux_table, ARRAY_SIZE(parents), sama5d2_gck[i].id,
					     NULL, 0, &clk);
		if (ret) {
			LOG_ERR("Register generated clock %d failed", i);
			return ret;
		}

		sama5d2_pmc->ghws[sama5d2_gck[i].id] = clk;
	}

	return 0;
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

	if (CLK_CNT_SYSTEM != nck(sama5d2_systemck) ||
	    CLK_CNT_PERIPH != nck(sama5d2_peripheral_clk) || CLK_CNT_GCK != nck(sama5d2_gck)) {
		LOG_ERR("Incorrect definitions could make array for pmc clocks not enough");
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

	ret = sam_pmc_register_programmable(dev, sama5d2_pmc);
	if (ret) {
		return;
	}

	ret = sam_pmc_register_system(dev, sama5d2_pmc);
	if (ret) {
		return;
	}

	ret = sam_pmc_register_peripheral(dev, sama5d2_pmc);
	if (ret) {
		return;
	}

	ret = sam_pmc_register_generated(dev, sama5d2_pmc);
	if (ret) {
		return;
	}

	LOG_DBG("SAMA5D2 PMC clocks registered successfully");
}
