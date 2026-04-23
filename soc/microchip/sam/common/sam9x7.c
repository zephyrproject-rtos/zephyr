/*
 * Copyright (C) 2026 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <limits.h>
#include <pmc.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>

LOG_MODULE_REGISTER(pmc_setup, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#define nck(a) (a[ARRAY_SIZE(a) - 1].id + 1)

#define PMC_INIT_TABLE(_table, _count)			\
	do {						\
		uint8_t _i;				\
		for (_i = 0; _i < (_count); _i++) {	\
			(_table)[_i] = _i;		\
		}					\
	} while (0)

#define PMC_FILL_TABLE(_to, _from, _count)		\
	do {						\
		uint8_t _i;				\
		for (_i = 0; _i < (_count); _i++) {	\
			(_to)[_i] = (_from)[_i];	\
		}					\
	} while (0)

static struct k_spinlock pmc_pll_lock;
static struct k_spinlock pmc_mck_lock;
static struct k_spinlock pmc_pcr_lock;

enum clock_count {
	CLK_CNT_CORE = PMC_LVDSPLL + 1,
	CLK_CNT_SYSTEM = 9 + 1,
	CLK_CNT_PERIPH = 67 + 1,
	CLK_CNT_GCK = 67 + 1,
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

/** @brief PLL clocks identifiers */
enum pll_ids {
	PLL_ID_PLLA,
	PLL_ID_UPLL,
	PLL_ID_AUDIO,
	PLL_ID_LVDS,
	PLL_ID_PLLA_DIV2,
	PLL_ID_MAX,
};

/** @brief PLL component identifiers */
enum pll_component_id {
	PLL_COMPID_FRAC,
	PLL_COMPID_DIV0,
	PLL_COMPID_DIV1,
	PLL_COMPID_MAX,
};

/** @brief PLL type identifiers */
enum pll_type {
	PLL_TYPE_FRAC,
	PLL_TYPE_DIV,
};

static const struct clk_master_characteristics mck_characteristics = {
	.output = CLK_RANGE(MHZ(32), 266666667),
	.divisors = { 1, 2, 4, 3, 5},
	.have_div3_pres = 1,
};

static const struct clk_master_layout sam9x7_master_layout = {
	.mask = 0x373,
	.pres_shift = 4,
	.offset = 0x28,
};

static const struct clk_range plla_core_outputs[] = { CLK_RANGE(MHZ(800), MHZ(1600)) };

static const struct clk_range upll_core_outputs[] = { CLK_RANGE(MHZ(600), MHZ(960)) };

static const struct clk_range lvdspll_core_outputs[] = { CLK_RANGE(MHZ(600), MHZ(1200)) };

static const struct clk_range audiopll_core_outputs[] = { CLK_RANGE(MHZ(600), MHZ(1200)) };

static const struct clk_range plladiv2_core_outputs[] = { CLK_RANGE(MHZ(800), MHZ(1600)) };

static const struct clk_range plla_outputs[] = { CLK_RANGE(MHZ(400), MHZ(800)) };

static const struct clk_range upll_outputs[] = { CLK_RANGE(MHZ(300), MHZ(480)) };

static const struct clk_range lvdspll_outputs[] = { CLK_RANGE(MHZ(175), MHZ(550)) };

static const struct clk_range audiopll_outputs[] = { CLK_RANGE(0, MHZ(300)) };

static const struct clk_range plladiv2_outputs[] = { CLK_RANGE(MHZ(200), MHZ(400)) };

static const struct clk_pll_characteristics plla_characteristics = {
	.input = CLK_RANGE(MHZ(20), MHZ(50)),
	.num_output = ARRAY_SIZE(plla_outputs),
	.output = plla_outputs,
	.core_output = plla_core_outputs,
	.acr = 0x00020010, /* Old ACR_DEFAULT_PLLA value */
};

static const struct clk_pll_characteristics upll_characteristics = {
	.input = CLK_RANGE(MHZ(20), MHZ(50)),
	.num_output = ARRAY_SIZE(upll_outputs),
	.output = upll_outputs,
	.core_output = upll_core_outputs,
	.upll = true,
	.acr = 0x12023010, /* fIN=[20 MHz, 32 MHz] */
};

static const struct clk_pll_characteristics lvdspll_characteristics = {
	.input = CLK_RANGE(MHZ(20), MHZ(50)),
	.num_output = ARRAY_SIZE(lvdspll_outputs),
	.output = lvdspll_outputs,
	.core_output = lvdspll_core_outputs,
	.acr = 0x12023010, /* fIN=[20 MHz, 32 MHz] */
};

static const struct clk_pll_characteristics audiopll_characteristics = {
	.input = CLK_RANGE(MHZ(20), MHZ(50)),
	.num_output = ARRAY_SIZE(audiopll_outputs),
	.output = audiopll_outputs,
	.core_output = audiopll_core_outputs,
	.acr = 0x12023010, /* fIN=[20 MHz, 32 MHz] */
};

static const struct clk_pll_characteristics plladiv2_characteristics = {
	.input = CLK_RANGE(MHZ(20), MHZ(50)),
	.num_output = ARRAY_SIZE(plladiv2_outputs),
	.output = plladiv2_outputs,
	.core_output = plladiv2_core_outputs,
	.acr = 0x00020010,  /* Old ACR_DEFAULT_PLLA value */
};

/** Layout for fractional PLL ID PLLA. */
static const struct clk_pll_layout plla_frac_layout = {
	.mul_mask = GENMASK(31, 24),
	.frac_mask = GENMASK(21, 0),
	.mul_shift = 24,
	.frac_shift = 0,
	.div2 = 1,
};

/** Layout for fractional PLLs. */
static const struct clk_pll_layout pll_frac_layout = {
	.mul_mask = GENMASK(31, 24),
	.frac_mask = GENMASK(21, 0),
	.mul_shift = 24,
	.frac_shift = 0,
};

/** Layout for DIV PLLs. */
static const struct clk_pll_layout pll_divpmc_layout = {
	.div_mask = GENMASK(7, 0),
	.endiv_mask = BIT(29),
	.div_shift = 0,
	.endiv_shift = 29,
};

/** Layout for DIV PLL ID PLLADIV2. */
static const struct clk_pll_layout plladiv2_divpmc_layout = {
	.div_mask = GENMASK(7, 0),
	.endiv_mask = BIT(29),
	.div_shift = 0,
	.endiv_shift = 29,
	.div2 = 1,
};

/** Layout for DIVIO dividers. */
static const struct clk_pll_layout pll_divio_layout = {
	.div_mask	= GENMASK(19, 12),
	.endiv_mask	= BIT(30),
	.div_shift	= 12,
	.endiv_shift	= 30,
};

enum sam9x7_pll_parent {
	CLK_PARENT_MAINCK,
	CLK_PARENT_MAIN_OSC,
	CLK_PARENT_FRACCL,
	CLK_PARENT_PLLA_FRACCK,
};

/**
 * @brief description for PLL clocks
 * @n:		clock name
 * @p:		clock parent
 * @l:		clock layout
 * @t:		clock type
 * @c:		pll characteristics
 * @clk:	handle for the clock
 * @eid:	export index in sam9x7->chws[] array
 */
static struct {
	const char *n;
	const enum sam9x7_pll_parent p;
	const struct clk_pll_layout *l;
	uint8_t t;
	const struct clk_pll_characteristics *c;
	struct device *clk;
	uint8_t eid;
} sam9x7_plls[][3] = {
	[PLL_ID_PLLA] = {
		{
			.n = "plla_fracck",
			.p = CLK_PARENT_MAINCK,
			.l = &plla_frac_layout,
			.t = PLL_TYPE_FRAC,
			.c = &plla_characteristics,
		},

		{
			.n = "plla_divpmcck",
			.p = CLK_PARENT_FRACCL,
			.l = &pll_divpmc_layout,
			.t = PLL_TYPE_DIV,
			.eid = PMC_PLLACK,
			.c = &plla_characteristics,
		},
	},

	[PLL_ID_UPLL] = {
		{
			.n = "upll_fracck",
			.p = CLK_PARENT_MAIN_OSC,
			.l = &pll_frac_layout,
			.t = PLL_TYPE_FRAC,
			.c = &upll_characteristics,
		},

		{
			.n = "upll_divpmcck",
			.p = CLK_PARENT_FRACCL,
			.l = &pll_divpmc_layout,
			.t = PLL_TYPE_DIV,
			.eid = PMC_UTMI,
			.c = &upll_characteristics,
		},
	},

	[PLL_ID_AUDIO] = {
		{
			.n = "audiopll_fracck",
			.p = CLK_PARENT_MAIN_OSC,
			.l = &pll_frac_layout,
			.c = &audiopll_characteristics,
			.t = PLL_TYPE_FRAC,
		},

		{
			.n = "audiopll_divpmcck",
			.p = CLK_PARENT_FRACCL,
			.l = &pll_divpmc_layout,
			.c = &audiopll_characteristics,
			.eid = PMC_AUDIOPMCPLL,
			.t = PLL_TYPE_DIV,
		},

		{
			.n = "audiopll_diviock",
			.p = CLK_PARENT_FRACCL,
			.l = &pll_divio_layout,
			.c = &audiopll_characteristics,
			.eid = PMC_AUDIOIOPLL,
			.t = PLL_TYPE_DIV,
		},
	},

	[PLL_ID_LVDS] = {
		{
			.n = "lvdspll_fracck",
			.p = CLK_PARENT_MAIN_OSC,
			.l = &pll_frac_layout,
			.c = &lvdspll_characteristics,
			.t = PLL_TYPE_FRAC,
		},

		{
			.n = "lvdspll_divpmcck",
			.p = CLK_PARENT_FRACCL,
			.l = &pll_divpmc_layout,
			.c = &lvdspll_characteristics,
			.eid = PMC_LVDSPLL,
			.t = PLL_TYPE_DIV,
		},
	},

	[PLL_ID_PLLA_DIV2] = {
		{
			.n = "plla_div2pmcck",
			.p = CLK_PARENT_PLLA_FRACCK,
			.l = &plladiv2_divpmc_layout,
			.c = &plladiv2_characteristics,
			.eid = PMC_PLLADIV2,
			.t = PLL_TYPE_DIV,
		},
	},
};

static const struct clk_programmable_layout sam9x7_programmable_layout = {
	.pres_mask = 0xff,
	.pres_shift = 8,
	.css_mask = 0x1f,
	.have_slck_mck = 0,
	.is_pres_direct = 1,
};

static const struct clk_pcr_layout sam9x7_pcr_layout = {
	.offset = 0x88,
	.cmd = BIT(31),
	.gckcss_mask = GENMASK(12, 8),
	.pid_mask = GENMASK(6, 0),
};

static const struct {
	char *n;
	char *p;
	uint32_t parent_idx;
	uint8_t id;
} sam9x7_systemck[] = {
	{ .n = "pck0", .p = "prog0", .id = 8, .parent_idx = CLK_OFFSET_PROG + 0 },
	{ .n = "pck1", .p = "prog1", .id = 9, .parent_idx = CLK_OFFSET_PROG + 1 },
};

/**
 * @brief Peripheral clocks description
 * @n:		clock name
 * @id:		peripheral id
 */
static const struct {
	char *n;
	uint8_t id;
} sam9x7_periphck[] = {
	{ .n = "pioA_clk",	.id = 2, },
	{ .n = "pioB_clk",	.id = 3, },
	{ .n = "pioC_clk",	.id = 4, },
	{ .n = "flex0_clk",	.id = 5, },
	{ .n = "flex1_clk",	.id = 6, },
	{ .n = "flex2_clk",	.id = 7, },
	{ .n = "flex3_clk",	.id = 8, },
	{ .n = "flex6_clk",	.id = 9, },
	{ .n = "flex7_clk",	.id = 10, },
	{ .n = "flex8_clk",	.id = 11, },
	{ .n = "sdmmc0_clk",	.id = 12, },
	{ .n = "flex4_clk",	.id = 13, },
	{ .n = "flex5_clk",	.id = 14, },
	{ .n = "flex9_clk",	.id = 15, },
	{ .n = "flex10_clk",	.id = 16, },
	{ .n = "tcb0_clk",	.id = 17, },
	{ .n = "pwm_clk",	.id = 18, },
	{ .n = "adc_clk",	.id = 19, },
	{ .n = "dma0_clk",	.id = 20, },
	{ .n = "uhphs_clk",	.id = 22, },
	{ .n = "udphs_clk",	.id = 23, },
	{ .n = "macb0_clk",	.id = 24, },
	{ .n = "lcd_clk",	.id = 25, },
	{ .n = "sdmmc1_clk",	.id = 26, },
	{ .n = "ssc_clk",	.id = 28, },
	{ .n = "can0_clk",	.id = 29, },
	{ .n = "can1_clk",	.id = 30, },
	{ .n = "flex11_clk",	.id = 32, },
	{ .n = "flex12_clk",	.id = 33, },
	{ .n = "i2s_clk",	.id = 34, },
	{ .n = "qspi_clk",	.id = 35, },
	{ .n = "gfx2d_clk",	.id = 36, },
	{ .n = "pit64b0_clk",	.id = 37, },
	{ .n = "trng_clk",	.id = 38, },
	{ .n = "aes_clk",	.id = 39, },
	{ .n = "tdes_clk",	.id = 40, },
	{ .n = "sha_clk",	.id = 41, },
	{ .n = "classd_clk",	.id = 42, },
	{ .n = "isi_clk",	.id = 43, },
	{ .n = "pioD_clk",	.id = 44, },
	{ .n = "tcb1_clk",	.id = 45, },
	{ .n = "dbgu_clk",	.id = 47, },
	{ .n = "pmecc_clk",	.id = 48, },
	/*
	 * mpddr_clk feeds DDR controller and is enabled by bootloader thus we
	 * need to keep it enabled in case there is no Linux consumer for it.
	 */
	{ .n = "mpddr_clk",	.id = 49, },
	{ .n = "csi2dc_clk",	.id = 52, },
	{ .n = "csi4l_clk",	.id = 53, },
	{ .n = "dsi4l_clk",	.id = 54, },
	{ .n = "lvdsc_clk",	.id = 56, },
	{ .n = "pit64b1_clk",	.id = 58, },
	{ .n = "puf_clk",	.id = 59, },
	{ .n = "gmactsu_clk",	.id = 67, },
};

/**
 * @brief Generic clock description
 * @n:			clock name
 * @pp:			PLL parents
 * @pp_mux_table:	PLL parents mux table
 * @r:			clock output range
 * @pp_chg_id:		id in parent array of changeable PLL parent
 * @pp_count:		PLL parents count
 * @id:			clock id
 */
static const struct {
	const char *n;
	struct {
		int pll_id;
		int pll_compid;
	} pp[8];
	const char pp_mux_table[8];
	struct clk_range r;
	int pp_chg_id;
	uint8_t pp_count;
	uint8_t id;
} sam9x7_gck[] = {
	{
		.n = "flex0_gclk",
		.id = 5,
		.pp = { { PLL_ID_PLLA_DIV2, 0 } },
		.pp_mux_table = { 8, },
		.pp_count = 1,
		.pp_chg_id = INT_MIN,
	},

	{
		.n = "flex1_gclk",
		.id = 6,
		.pp = {  { PLL_ID_PLLA_DIV2, 0 } },
		.pp_mux_table = { 8, },
		.pp_count = 1,
		.pp_chg_id = INT_MIN,
	},

	{
		.n = "flex2_gclk",
		.id = 7,
		.pp = { { PLL_ID_PLLA_DIV2, 0 } },
		.pp_mux_table = { 8, },
		.pp_count = 1,
		.pp_chg_id = INT_MIN,
	},

	{
		.n = "flex3_gclk",
		.id = 8,
		.pp = { { PLL_ID_PLLA_DIV2, 0 } },
		.pp_mux_table = { 8, },
		.pp_count = 1,
		.pp_chg_id = INT_MIN,
	},

	{
		.n = "flex6_gclk",
		.id = 9,
		.pp = { { PLL_ID_PLLA_DIV2, 0 } },
		.pp_mux_table = { 8, },
		.pp_count = 1,
		.pp_chg_id = INT_MIN,
	},

	{
		.n = "flex7_gclk",
		.id = 10,
		.pp = { { PLL_ID_PLLA_DIV2, 0 } },
		.pp_mux_table = { 8, },
		.pp_count = 1,
		.pp_chg_id = INT_MIN,
	},

	{
		.n = "flex8_gclk",
		.id = 11,
		.pp = { { PLL_ID_PLLA_DIV2, 0 } },
		.pp_mux_table = { 8, },
		.pp_count = 1,
		.pp_chg_id = INT_MIN,
	},

	{
		.n = "sdmmc0_gclk",
		.id = 12,
		.r = { .max = MHZ(105) },
		.pp = { { PLL_ID_AUDIO, 1 }, { PLL_ID_PLLA_DIV2, 0 } },
		.pp_mux_table = { 6, 8, },
		.pp_count = 2,
		.pp_chg_id = INT_MIN,
	},

	{
		.n = "flex4_gclk",
		.id = 13,
		.pp = { { PLL_ID_PLLA_DIV2, 0 } },
		.pp_mux_table = { 8, },
		.pp_count = 1,
		.pp_chg_id = INT_MIN,
	},

	{
		.n = "flex5_gclk",
		.id = 14,
		.pp = { { PLL_ID_PLLA_DIV2, 0 } },
		.pp_mux_table = { 8, },
		.pp_count = 1,
		.pp_chg_id = INT_MIN,
	},

	{
		.n = "flex9_gclk",
		.id = 15,
		.pp = { { PLL_ID_PLLA_DIV2, 0 } },
		.pp_mux_table = { 8, },
		.pp_count = 1,
		.pp_chg_id = INT_MIN,
	},

	{
		.n = "flex10_gclk",
		.id = 16,
		.pp = { { PLL_ID_PLLA_DIV2, 0 } },
		.pp_mux_table = { 8, },
		.pp_count = 1,
		.pp_chg_id = INT_MIN,
	},

	{
		.n = "tcb0_gclk",
		.id = 17,
		.pp = { { PLL_ID_AUDIO, 1 }, { PLL_ID_PLLA_DIV2, 0 } },
		.pp_mux_table = { 6, 8, },
		.pp_count = 2,
		.pp_chg_id = INT_MIN,
	},

	{
		.n = "adc_gclk",
		.id = 19,
		.pp = { { PLL_ID_UPLL, 1 }, { PLL_ID_PLLA_DIV2, 0 } },
		.pp_mux_table = { 5, 8, },
		.pp_count = 2,
		.pp_chg_id = INT_MIN,
	},

	{
		.n = "lcd_gclk",
		.id = 25,
		.r = { .max = MHZ(75) },
		.pp = { { PLL_ID_AUDIO, 1 }, { PLL_ID_PLLA_DIV2, 0 } },
		.pp_mux_table = { 6, 8, },
		.pp_count = 2,
		.pp_chg_id = INT_MIN,
	},

	{
		.n = "sdmmc1_gclk",
		.id = 26,
		.r = { .max = MHZ(105) },
		.pp = { { PLL_ID_AUDIO, 1 }, { PLL_ID_PLLA_DIV2, 0 } },
		.pp_mux_table = { 6, 8, },
		.pp_count = 2,
		.pp_chg_id = INT_MIN,
	},

	{
		.n = "mcan0_gclk",
		.id = 29,
		.r = { .max = MHZ(80) },
		.pp = { { PLL_ID_UPLL, 1 }, { PLL_ID_PLLA_DIV2, 0 } },
		.pp_mux_table = { 5, 8, },
		.pp_count = 2,
		.pp_chg_id = INT_MIN,
	},

	{
		.n = "mcan1_gclk",
		.id = 30,
		.r = { .max = MHZ(80) },
		.pp = { { PLL_ID_UPLL, 1 }, { PLL_ID_PLLA_DIV2, 0 } },
		.pp_mux_table = { 5, 8, },
		.pp_count = 2,
		.pp_chg_id = INT_MIN,
	},

	{
		.n = "flex11_gclk",
		.id = 32,
		.pp = { { PLL_ID_PLLA_DIV2, 0 } },
		.pp_mux_table = { 8, },
		.pp_count = 1,
		.pp_chg_id = INT_MIN,
	},

	{
		.n = "flex12_gclk",
		.id = 33,
		.pp = { { PLL_ID_PLLA_DIV2, 0 } },
		.pp_mux_table = { 8, },
		.pp_count = 1,
		.pp_chg_id = INT_MIN,
	},

	{
		.n = "i2s_gclk",
		.id = 34,
		.r = { .max = MHZ(100) },
		.pp = { { PLL_ID_AUDIO, 1 }, { PLL_ID_PLLA_DIV2, 0 } },
		.pp_mux_table = { 6, 8, },
		.pp_count = 2,
		.pp_chg_id = INT_MIN,
	},

	{
		.n = "qspi_gclk",
		.id = 35,
		.r = { .max = MHZ(200) },
		.pp = { { PLL_ID_AUDIO, 1 }, { PLL_ID_PLLA_DIV2, 0 } },
		.pp_mux_table = { 6, 8, },
		.pp_count = 2,
		.pp_chg_id = INT_MIN,
	},

	{
		.n = "pit64b0_gclk",
		.id = 37,
		.pp = { { PLL_ID_PLLA_DIV2, 0 } },
		.pp_mux_table = { 8, },
		.pp_count = 1,
		.pp_chg_id = INT_MIN,
	},

	{
		.n = "classd_gclk",
		.id = 42,
		.r = { .max = MHZ(100) },
		.pp = { { PLL_ID_AUDIO, 1 }, { PLL_ID_PLLA_DIV2, 0 } },
		.pp_mux_table = { 6, 8, },
		.pp_count = 2,
		.pp_chg_id = INT_MIN,
	},

	{
		.n = "tcb1_gclk",
		.id = 45,
		.pp = { { PLL_ID_AUDIO, 1 }, { PLL_ID_PLLA_DIV2, 0 } },
		.pp_mux_table = { 6, 8, },
		.pp_count = 2,
		.pp_chg_id = INT_MIN,
	},

	{
		.n = "dbgu_gclk",
		.id = 47,
		.pp = { { PLL_ID_PLLA_DIV2, 0 } },
		.pp_mux_table = { 8, },
		.pp_count = 1,
		.pp_chg_id = INT_MIN,
	},

	{
		.n = "mipiphy_gclk",
		.id = 55,
		.r = { .max = MHZ(27) },
		.pp = { { PLL_ID_PLLA_DIV2, 0 } },
		.pp_mux_table = { 8, },
		.pp_count = 1,
		.pp_chg_id = INT_MIN,
	},

	{
		.n = "pit64b1_gclk",
		.id = 58,
		.pp = { { PLL_ID_PLLA_DIV2, 0 } },
		.pp_mux_table = { 8, },
		.pp_count = 1,
		.pp_chg_id = INT_MIN,
	},

	{
		.n = "gmac_gclk",
		.id = 67,
		.pp = { { PLL_ID_AUDIO, 1 }, { PLL_ID_PLLA_DIV2, 0 } },
		.pp_mux_table = { 6, 8, },
		.pp_count = 2,
		.pp_chg_id = INT_MIN,
	},
};

static int sam_pmc_register_pll(const struct device *dev, struct pmc_data *sam9x7_pmc,
				struct device *main_osc)
{
	const struct sam_pmc_cfg *cfg = dev->config;
	const struct device *parent_hw = NULL;
	void *const regmap = cfg->reg;
	struct device *clk;
	int ret, i, j;

	for (i = 0; i < PLL_ID_MAX; i++) {
		for (j = 0; j < PLL_COMPID_MAX; j++) {

			if (!sam9x7_plls[i][j].n) {
				continue;
			}

			switch (sam9x7_plls[i][j].t) {
			case PLL_TYPE_FRAC:
				switch (sam9x7_plls[i][j].p) {
				case CLK_PARENT_MAINCK:
					parent_hw = sam9x7_pmc->chws[PMC_MAIN];
					break;
				case CLK_PARENT_MAIN_OSC:
					parent_hw = main_osc;
					break;
				default:
					__ASSERT(false, "Invalid parent clock");
					break;
				}
				ret = sam9x60_clk_register_frac_pll(regmap,
								    &pmc_pll_lock,
								    sam9x7_plls[i][j].n,
								    parent_hw, i,
								    sam9x7_plls[i][j].c,
								    sam9x7_plls[i][j].l,
								    &clk);
				break;

			case PLL_TYPE_DIV:
				switch (sam9x7_plls[i][j].p) {
				case CLK_PARENT_FRACCL:
					parent_hw = sam9x7_plls[i][0].clk;
					break;
				case CLK_PARENT_PLLA_FRACCK:
					parent_hw = sam9x7_plls[PLL_ID_PLLA][0].clk;
					break;
				default:
					__ASSERT(false, "Invalid parent clock");
					break;
				}
				ret = sam9x60_clk_register_div_pll(regmap,
								   &pmc_pll_lock,
								   sam9x7_plls[i][j].n,
								   parent_hw, i,
								   sam9x7_plls[i][j].c,
								   sam9x7_plls[i][j].l,
								   &clk);
				break;

			default:
				continue;
			}

			if (ret) {
				LOG_ERR("Register clock %s failed.", sam9x7_plls[i][j].n);
				return ret;
			}

			sam9x7_plls[i][j].clk = clk;
			if (sam9x7_plls[i][j].eid) {
				sam9x7_pmc->chws[sam9x7_plls[i][j].eid] = clk;
			}
		}
	}

	return 0;
}

static int sam_pmc_register_master(const struct device *dev, struct pmc_data *sam9x7_pmc)
{
	const struct sam_pmc_cfg *cfg = dev->config;
	const struct device *parents[10];
	void *const regmap = cfg->reg;
	struct device *clk;
	int ret;

	parents[0] = cfg->md_slck;
	parents[1] = sam9x7_pmc->chws[PMC_MAIN];
	parents[2] = sam9x7_pmc->chws[PMC_PLLACK];
	parents[3] = sam9x7_pmc->chws[PMC_UTMI];
	ret = clk_register_master_pres(regmap, "masterck_pres",
				       parents, 4,
				       &sam9x7_master_layout,
				       &mck_characteristics,
				       &pmc_mck_lock,
				       &clk);
	if (ret) {
		LOG_ERR("Register clock masterck_pres failed.");
		return ret;
	}

	ret = clk_register_master_div(regmap, "masterck_div",
				      clk, &sam9x7_master_layout,
				      &mck_characteristics, &pmc_mck_lock,
				      0, &clk);
	if (ret) {
		LOG_ERR("Register masterck_div clock failed.");
		return ret;
	}

	sam9x7_pmc->chws[PMC_MCK] = clk;

	return 0;
}

static int sam_pmc_register_generated(const struct device *dev, struct pmc_data *sam9x7_pmc)
{
	const struct sam_pmc_cfg *cfg = dev->config;
	const struct device *parents[7];
	uint32_t mux_table[7];
	struct device *clk;
	int ret, i, j;

	parents[0] = cfg->md_slck;
	parents[1] = cfg->td_slck;
	parents[2] = sam9x7_pmc->chws[PMC_MAIN];
	parents[3] = sam9x7_pmc->chws[PMC_MCK];

	for (i = 0; i < ARRAY_SIZE(sam9x7_gck); i++) {
		if (4 + sam9x7_gck[i].pp_count > ARRAY_SIZE(mux_table)) {
			LOG_ERR("Array for mux table not enough");
			return -ENOMEM;
		}

		PMC_INIT_TABLE(mux_table, 4);
		PMC_FILL_TABLE(&mux_table[4], sam9x7_gck[i].pp_mux_table, sam9x7_gck[i].pp_count);

		for (j = 0; j < sam9x7_gck[i].pp_count; j++) {
			uint8_t pll_id = sam9x7_gck[i].pp[j].pll_id;
			uint8_t pll_compid = sam9x7_gck[i].pp[j].pll_compid;

			parents[4 + j] = sam9x7_plls[pll_id][pll_compid].clk;
		}

		ret = clk_register_generated((void *const)cfg->reg, &pmc_pcr_lock,
					     &sam9x7_pcr_layout,
					     sam9x7_gck[i].n,
					     parents, mux_table,
					     4 + sam9x7_gck[i].pp_count,
					     sam9x7_gck[i].id,
					     &sam9x7_gck[i].r,
					     sam9x7_gck[i].pp_chg_id, &clk);
		if (ret) {
			LOG_ERR("Register generated clock %d failed.", i);
			return ret;
		}

		sam9x7_pmc->ghws[sam9x7_gck[i].id] = clk;
	}

	return 0;
}

void sam_pmc_setup(const struct device *dev)
{
	const struct sam_pmc_cfg *cfg = dev->config;
	struct sam_pmc_data *data = dev->data;
	const struct device *main_xtal = cfg->main_xtal;
	const struct device *td_slck = cfg->td_slck;
	const struct device *md_slck = cfg->md_slck;
	const struct clk_range range = {.min = 0, .max = 0,};

	void *const regmap = cfg->reg;
	const struct device *parents[7];
	struct pmc_data *sam9x7_pmc;
	struct device *clk, *main_rc, *main_osc;
	uint32_t rate, bypass;
	int ret, i;

	if (!td_slck || !md_slck || !main_xtal || !regmap) {
		LOG_ERR("Incorrect parameters.");
		return;
	}

	if (CLK_CNT_SYSTEM != nck(sam9x7_systemck) ||
	    CLK_CNT_PERIPH != nck(sam9x7_periphck) ||
	    CLK_CNT_GCK != nck(sam9x7_gck)) {
		LOG_ERR("Incorrect definitions could make array for pmc clocks not enough");
		return;
	}

	sam9x7_pmc = pmc_data_allocate(CLK_CNT_CORE,
				       CLK_CNT_SYSTEM,
				       CLK_CNT_PERIPH,
				       CLK_CNT_GCK, CLK_CNT_PROG, &pmc_table[0]);
	if (!sam9x7_pmc) {
		LOG_ERR("allocate PMC data failed.");
		return;
	}
	data->pmc = sam9x7_pmc;

	ret = clk_register_main_rc_osc(regmap, "main_rc_osc", 12000000, &main_rc);
	if (ret) {
		LOG_ERR("Register clock main_rc_osc failed.");
		return;
	}

	if (clock_control_get_rate(main_xtal, NULL, &rate)) {
		LOG_ERR("get clock rate of main_xtal failed.");
		return;
	}

	bypass = 0;
	ret = clk_register_main_osc(regmap, "main_osc", bypass, rate, &main_osc);
	if (ret) {
		LOG_ERR("Register clock main_osc failed.");
		return;
	}

	parents[0] = main_rc;
	parents[1] = main_osc;
	ret = clk_register_main(regmap, "mainck", parents, 2, &clk);
	if (ret) {
		LOG_ERR("Register clock mainck failed.");
		return;
	}
	sam9x7_pmc->chws[PMC_MAIN] = clk;

	ret = sam_pmc_register_pll(dev, sam9x7_pmc, main_osc);
	if (ret) {
		return;
	}

	ret = sam_pmc_register_master(dev, sam9x7_pmc);
	if (ret) {
		return;
	}

	parents[0] = md_slck;
	parents[1] = td_slck;
	parents[2] = sam9x7_pmc->chws[PMC_MAIN];
	parents[3] = sam9x7_pmc->chws[PMC_MCK];
	parents[4] = sam9x7_pmc->chws[PMC_PLLACK];
	parents[5] = sam9x7_pmc->chws[PMC_UTMI];
	parents[6] = sam9x7_pmc->chws[PMC_AUDIOPMCPLL];
	for (i = 0; i < SOC_NUM_CLOCK_PROGRAMMABLE; i++) {
		ret = clk_register_programmable(regmap, parents,
						7, i,
						&sam9x7_programmable_layout,
						NULL, &clk);
		if (ret) {
			LOG_ERR("Register programmable clock %d failed.", i);
			return;
		}

		sam9x7_pmc->pchws[i] = clk;
	}

	for (i = 0; i < ARRAY_SIZE(sam9x7_systemck); i++) {
		ret = clk_register_system(regmap, sam9x7_systemck[i].n,
					  pmc_table[sam9x7_systemck[i].parent_idx],
					  sam9x7_systemck[i].id, &clk);
		if (ret) {
			LOG_ERR("Register system clock %d failed.", i);
			return;
		}

		sam9x7_pmc->shws[sam9x7_systemck[i].id] = clk;
	}

	for (i = 0; i < ARRAY_SIZE(sam9x7_periphck); i++) {
		ret = clk_register_peripheral(regmap, &pmc_pcr_lock,
					      &sam9x7_pcr_layout,
					      sam9x7_periphck[i].n,
					      sam9x7_pmc->chws[PMC_MCK],
					      sam9x7_periphck[i].id,
					      &range,
					      &clk);
		if (ret) {
			LOG_ERR("Register peripheral clock %d failed.", i);
			return;
		}

		sam9x7_pmc->phws[sam9x7_periphck[i].id] = clk;
	}

	ret = sam_pmc_register_generated(dev, sam9x7_pmc);
	if (ret) {
		return;
	}

	LOG_DBG("Register PMC clocks successfully.");
}
