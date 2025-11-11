/*
 * Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <limits.h>
#include <pmc.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>

#include <zephyr/logging/log.h>
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
static struct k_spinlock pmc_mck0_lock;
static struct k_spinlock pmc_mckX_lock;
static struct k_spinlock pmc_pcr_lock;

/*
 * PLL clocks identifiers
 * @PLL_ID_CPU:		CPU PLL identifier
 * @PLL_ID_SYS:		System PLL identifier
 * @PLL_ID_DDR:		DDR PLL identifier
 * @PLL_ID_IMG:		Image subsystem PLL identifier
 * @PLL_ID_BAUD:	Baud PLL identifier
 * @PLL_ID_AUDIO:	Audio PLL identifier
 * @PLL_ID_ETH:		Ethernet PLL identifier
 */
enum pll_ids {
	PLL_ID_CPU,
	PLL_ID_SYS,
	PLL_ID_DDR,
	PLL_ID_IMG,
	PLL_ID_BAUD,
	PLL_ID_AUDIO,
	PLL_ID_ETH,
	PLL_ID_MAX,
};

/*
 * PLL component identifier
 * @PLL_COMPID_FRAC: Fractional PLL component identifier
 * @PLL_COMPID_DIV0: 1st PLL divider component identifier
 * @PLL_COMPID_DIV1: 2nd PLL divider component identifier
 */
enum pll_component_id {
	PLL_COMPID_FRAC,
	PLL_COMPID_DIV0,
	PLL_COMPID_DIV1,
	PLL_COMPID_MAX,
};

/*
 * PLL type identifiers
 * @PLL_TYPE_FRAC:	fractional PLL identifier
 * @PLL_TYPE_DIV:	divider PLL identifier
 */
enum pll_type {
	PLL_TYPE_FRAC,
	PLL_TYPE_DIV,
};

/* Layout for fractional PLLs. */
static const struct clk_pll_layout pll_layout_frac = {
	.mul_mask	= GENMASK(31, 24),
	.frac_mask	= GENMASK(21, 0),
	.mul_shift	= 24,
	.frac_shift	= 0,
};

/* Layout for DIVPMC dividers. */
static const struct clk_pll_layout pll_layout_divpmc = {
	.div_mask	= GENMASK(7, 0),
	.endiv_mask	= BIT(29),
	.div_shift	= 0,
	.endiv_shift	= 29,
};

/* Layout for DIVIO dividers. */
static const struct clk_pll_layout pll_layout_divio = {
	.div_mask	= GENMASK(19, 12),
	.endiv_mask	= BIT(30),
	.div_shift	= 12,
	.endiv_shift	= 30,
};

/*
 * CPU PLL output range.
 * Notice: The upper limit has been setup to 1000000002 due to hardware
 * block which cannot output exactly 1GHz.
 */
static const struct clk_range cpu_pll_outputs[] = {
	{ .min = 2343750, .max = 1000000002 },
};

/* PLL output range. */
static const struct clk_range pll_outputs[] = {
	{ .min = 2343750, .max = 1200000000 },
};

/* Fractional PLL core output range. */
static const struct clk_range core_outputs[] = {
	{ .min = 600000000, .max = 1200000000 },
};

/* CPU PLL characteristics. */
static const struct clk_pll_characteristics cpu_pll_characteristics = {
	.input = { .min = 12000000, .max = 50000000 },
	.num_output = ARRAY_SIZE(cpu_pll_outputs),
	.output = cpu_pll_outputs,
	.core_output = core_outputs,
};

/* PLL characteristics. */
static const struct clk_pll_characteristics pll_characteristics = {
	.input = { .min = 12000000, .max = 50000000 },
	.num_output = ARRAY_SIZE(pll_outputs),
	.output = pll_outputs,
	.core_output = core_outputs,
};

/*
 * SAMA7G5 PLL possible parents
 * @SAMA7G5_PLL_PARENT_MAINCK: MAINCK is PLL a parent
 * @SAMA7G5_PLL_PARENT_MAIN_XTAL: MAIN XTAL is a PLL parent
 * @SAMA7G5_PLL_PARENT_FRACCK: Frac PLL is a PLL parent (for PLL dividers)
 */
enum sama7g5_pll_parent {
	SAMA7G5_PLL_PARENT_MAINCK,
	SAMA7G5_PLL_PARENT_MAIN_XTAL,
	SAMA7G5_PLL_PARENT_FRACCK,
};

/*
 * PLL clocks description
 * @n:		clock name
 * @l:		clock layout
 * @c:		clock characteristics
 * @clk:	pointer to clk
 * @t:		clock type
 * @f:		clock flags
 * @p:		clock parent
 * @eid:	export index in sama7g5->chws[] array
 * @safe_div:	intermediate divider need to be set on PRE_RATE_CHANGE
 *		notification
 */
static struct sama7g5_pll {
	const char *n;
	const struct clk_pll_layout *l;
	const struct clk_pll_characteristics *c;
	struct device *clk;
	unsigned long f;
	enum sama7g5_pll_parent p;
	uint8_t t;
	uint8_t eid;
	uint8_t safe_div;
} sama7g5_plls[][PLL_COMPID_MAX] = {
	[PLL_ID_CPU] = {
		[PLL_COMPID_FRAC] = {
			.n = "cpupll_fracck",
			.p = SAMA7G5_PLL_PARENT_MAINCK,
			.l = &pll_layout_frac,
			.c = &cpu_pll_characteristics,
			.t = PLL_TYPE_FRAC,
		},

		[PLL_COMPID_DIV0] = {
			.n = "cpupll_divpmcck",
			.p = SAMA7G5_PLL_PARENT_FRACCK,
			.l = &pll_layout_divpmc,
			.c = &cpu_pll_characteristics,
			.t = PLL_TYPE_DIV,
			.eid = PMC_CPUPLL,
			/*
			 * Safe div=15 should be safe even for switching b/w 1GHz and
			 * 90MHz (frac pll might go up to 1.2GHz).
			 */
			.safe_div = 15,
		},
	},

	[PLL_ID_SYS] = {
		[PLL_COMPID_FRAC] = {
			.n = "syspll_fracck",
			.p = SAMA7G5_PLL_PARENT_MAINCK,
			.l = &pll_layout_frac,
			.c = &pll_characteristics,
			.t = PLL_TYPE_FRAC,
		},

		[PLL_COMPID_DIV0] = {
			.n = "syspll_divpmcck",
			.p = SAMA7G5_PLL_PARENT_FRACCK,
			.l = &pll_layout_divpmc,
			.c = &pll_characteristics,
			.t = PLL_TYPE_DIV,
			.eid = PMC_SYSPLL,
		},
	},

	[PLL_ID_DDR] = {
		[PLL_COMPID_FRAC] = {
			.n = "ddrpll_fracck",
			.p = SAMA7G5_PLL_PARENT_MAINCK,
			.l = &pll_layout_frac,
			.c = &pll_characteristics,
			.t = PLL_TYPE_FRAC,
		},

		[PLL_COMPID_DIV0] = {
			.n = "ddrpll_divpmcck",
			.p = SAMA7G5_PLL_PARENT_FRACCK,
			.l = &pll_layout_divpmc,
			.c = &pll_characteristics,
			.t = PLL_TYPE_DIV,
		},
	},

	[PLL_ID_IMG] = {
		[PLL_COMPID_FRAC] = {
			.n = "imgpll_fracck",
			.p = SAMA7G5_PLL_PARENT_MAINCK,
			.l = &pll_layout_frac,
			.c = &pll_characteristics,
			.t = PLL_TYPE_FRAC,
		},

		[PLL_COMPID_DIV0] = {
			.n = "imgpll_divpmcck",
			.p = SAMA7G5_PLL_PARENT_FRACCK,
			.l = &pll_layout_divpmc,
			.c = &pll_characteristics,
			.t = PLL_TYPE_DIV,
		},
	},

	[PLL_ID_BAUD] = {
		[PLL_COMPID_FRAC] = {
			.n = "baudpll_fracck",
			.p = SAMA7G5_PLL_PARENT_MAINCK,
			.l = &pll_layout_frac,
			.c = &pll_characteristics,
			.t = PLL_TYPE_FRAC,
		},

		[PLL_COMPID_DIV0] = {
			.n = "baudpll_divpmcck",
			.p = SAMA7G5_PLL_PARENT_FRACCK,
			.l = &pll_layout_divpmc,
			.c = &pll_characteristics,
			.t = PLL_TYPE_DIV,
		},
	},

	[PLL_ID_AUDIO] = {
		[PLL_COMPID_FRAC] = {
			.n = "audiopll_fracck",
			.p = SAMA7G5_PLL_PARENT_MAIN_XTAL,
			.l = &pll_layout_frac,
			.c = &pll_characteristics,
			.t = PLL_TYPE_FRAC,
		},

		[PLL_COMPID_DIV0] = {
			.n = "audiopll_divpmcck",
			.p = SAMA7G5_PLL_PARENT_FRACCK,
			.l = &pll_layout_divpmc,
			.c = &pll_characteristics,
			.t = PLL_TYPE_DIV,
			.eid = PMC_AUDIOPMCPLL,
		},

		[PLL_COMPID_DIV1] = {
			.n = "audiopll_diviock",
			.p = SAMA7G5_PLL_PARENT_FRACCK,
			.l = &pll_layout_divio,
			.c = &pll_characteristics,
			.t = PLL_TYPE_DIV,
			.eid = PMC_AUDIOIOPLL,
		},
	},

	[PLL_ID_ETH] = {
		[PLL_COMPID_FRAC] = {
			.n = "ethpll_fracck",
			.p = SAMA7G5_PLL_PARENT_MAIN_XTAL,
			.l = &pll_layout_frac,
			.c = &pll_characteristics,
			.t = PLL_TYPE_FRAC,
		},

		[PLL_COMPID_DIV0] = {
			.n = "ethpll_divpmcck",
			.p = SAMA7G5_PLL_PARENT_FRACCK,
			.l = &pll_layout_divpmc,
			.c = &pll_characteristics,
			.t = PLL_TYPE_DIV,
		},
	},
};

/* Used to create an array entry identifying a PLL by its components. */
#define PLL_IDS_TO_ARR_ENTRY(_id, _comp) { PLL_ID_##_id, PLL_COMPID_##_comp}

/*
 * Master clock (MCK[1..4]) description
 * @n:			clock name
 * @ep:			extra parents names array (entry formed by PLL components
 *			identifiers (see enum pll_component_id))
 * @clk:		pointer to clk
 * @ep_chg_id:		index in parents array that specifies the changeable
 *			parent
 * @ep_count:		extra parents count
 * @ep_mux_table:	mux table for extra parents
 * @id:			clock id
 * @eid:		export index in sama7g5->chws[] array
 * @c:			true if clock is critical and cannot be disabled
 */
static struct {
	const char *n;
	struct {
		int pll_id;
		int pll_compid;
	} ep[4];
	struct device *clk;
	int ep_chg_id;
	uint8_t ep_count;
	uint8_t ep_mux_table[4];
	uint8_t id;
	uint8_t eid;
	uint8_t c;
} sama7g5_mckx[] = {
	{ .n = "mck0", }, /* Dummy entry for MCK0 to store hw in probe. */
	{ .n = "mck1",
	  .id = 1,
	  .ep = { PLL_IDS_TO_ARR_ENTRY(SYS, DIV0), },
	  .ep_mux_table = { 5, },
	  .ep_count = 1,
	  .ep_chg_id = INT_MIN,
	  .eid = PMC_MCK1,
	  .c = 1, },

	{ .n = "mck2",
	  .id = 2,
	  .ep = { PLL_IDS_TO_ARR_ENTRY(DDR, DIV0), },
	  .ep_mux_table = { 6, },
	  .ep_count = 1,
	  .ep_chg_id = INT_MIN,
	  .c = 1, },

	{ .n = "mck3",
	  .id = 3,
	  .ep = { PLL_IDS_TO_ARR_ENTRY(SYS, DIV0), PLL_IDS_TO_ARR_ENTRY(DDR, DIV0),
		  PLL_IDS_TO_ARR_ENTRY(IMG, DIV0), },
	  .ep_mux_table = { 5, 6, 7, },
	  .ep_count = 3,
	  .ep_chg_id = 5, },

	{ .n = "mck4",
	  .id = 4,
	  .ep = { PLL_IDS_TO_ARR_ENTRY(SYS, DIV0), },
	  .ep_mux_table = { 5, },
	  .ep_count = 1,
	  .ep_chg_id = INT_MIN,
	  .c = 1, },
};

/*
 * System clock description
 * @n:	clock name
 * @id: clock id
 */
static const struct {
	const char *n;
	uint8_t id;
} sama7g5_systemck[] = {
	{ .n = "pck0", .id = 8, },
	{ .n = "pck1", .id = 9, },
	{ .n = "pck2", .id = 10, },
	{ .n = "pck3", .id = 11, },
	{ .n = "pck4", .id = 12, },
	{ .n = "pck5", .id = 13, },
	{ .n = "pck6", .id = 14, },
	{ .n = "pck7", .id = 15, },
};

/* Mux table for programmable clocks. */
static uint32_t sama7g5_prog_mux_table[] = { 0, 1, 2, 5, 6, 7, 8, 9, 10, };

/*
 * Peripheral clock parent hw identifier (used to index in sama7g5_mckx[])
 * @PCK_PARENT_HW_MCK0: pck parent hw identifier is MCK0
 * @PCK_PARENT_HW_MCK1: pck parent hw identifier is MCK1
 * @PCK_PARENT_HW_MCK2: pck parent hw identifier is MCK2
 * @PCK_PARENT_HW_MCK3: pck parent hw identifier is MCK3
 * @PCK_PARENT_HW_MCK4: pck parent hw identifier is MCK4
 * @PCK_PARENT_HW_MAX: max identifier
 */
enum sama7g5_pck_parent_hw_id {
	PCK_PARENT_HW_MCK0,
	PCK_PARENT_HW_MCK1,
	PCK_PARENT_HW_MCK2,
	PCK_PARENT_HW_MCK3,
	PCK_PARENT_HW_MCK4,
	PCK_PARENT_HW_MAX,
};

/*
 * Peripheral clock description
 * @n:		clock name
 * @p:		clock parent hw id
 * @r:		clock range values
 * @id:		clock id
 * @chgp:	index in parent array of the changeable parent
 */
static struct {
	const char *n;
	enum sama7g5_pck_parent_hw_id p;
	struct clk_range r;
	uint8_t chgp;
	uint8_t id;
} sama7g5_periphck[] = {
	{ .n = "pioA_clk",	.p = PCK_PARENT_HW_MCK0, .id = 11, },
	{ .n = "securam_clk",	.p = PCK_PARENT_HW_MCK0, .id = 18, },
	{ .n = "sfr_clk",	.p = PCK_PARENT_HW_MCK1, .id = 19, },
	{ .n = "hsmc_clk",	.p = PCK_PARENT_HW_MCK1, .id = 21, },
	{ .n = "xdmac0_clk",	.p = PCK_PARENT_HW_MCK1, .id = 22, },
	{ .n = "xdmac1_clk",	.p = PCK_PARENT_HW_MCK1, .id = 23, },
	{ .n = "xdmac2_clk",	.p = PCK_PARENT_HW_MCK1, .id = 24, },
	{ .n = "acc_clk",	.p = PCK_PARENT_HW_MCK1, .id = 25, },
	{ .n = "aes_clk",	.p = PCK_PARENT_HW_MCK1, .id = 27, },
	{ .n = "tzaesbasc_clk",	.p = PCK_PARENT_HW_MCK1, .id = 28, },
	{ .n = "asrc_clk",	.p = PCK_PARENT_HW_MCK1, .id = 30, .r = { .max = 200000000, }, },
	{ .n = "cpkcc_clk",	.p = PCK_PARENT_HW_MCK0, .id = 32, },
	{ .n = "csi_clk",	.p = PCK_PARENT_HW_MCK3, .id = 33,
	  .r = { .max = 266000000, }, .chgp = 1, },
	{ .n = "csi2dc_clk",	.p = PCK_PARENT_HW_MCK3, .id = 34,
	  .r = { .max = 266000000, }, .chgp = 1, },
	{ .n = "eic_clk",	.p = PCK_PARENT_HW_MCK1, .id = 37, },
	{ .n = "flex0_clk",	.p = PCK_PARENT_HW_MCK1, .id = 38, },
	{ .n = "flex1_clk",	.p = PCK_PARENT_HW_MCK1, .id = 39, },
	{ .n = "flex2_clk",	.p = PCK_PARENT_HW_MCK1, .id = 40, },
	{ .n = "flex3_clk",	.p = PCK_PARENT_HW_MCK1, .id = 41, },
	{ .n = "flex4_clk",	.p = PCK_PARENT_HW_MCK1, .id = 42, },
	{ .n = "flex5_clk",	.p = PCK_PARENT_HW_MCK1, .id = 43, },
	{ .n = "flex6_clk",	.p = PCK_PARENT_HW_MCK1, .id = 44, },
	{ .n = "flex7_clk",	.p = PCK_PARENT_HW_MCK1, .id = 45, },
	{ .n = "flex8_clk",	.p = PCK_PARENT_HW_MCK1, .id = 46, },
	{ .n = "flex9_clk",	.p = PCK_PARENT_HW_MCK1, .id = 47, },
	{ .n = "flex10_clk",	.p = PCK_PARENT_HW_MCK1, .id = 48, },
	{ .n = "flex11_clk",	.p = PCK_PARENT_HW_MCK1, .id = 49, },
	{ .n = "gmac0_clk",	.p = PCK_PARENT_HW_MCK1, .id = 51, },
	{ .n = "gmac1_clk",	.p = PCK_PARENT_HW_MCK1, .id = 52, },
	{ .n = "icm_clk",	.p = PCK_PARENT_HW_MCK1, .id = 55, },
	{ .n = "isc_clk",	.p = PCK_PARENT_HW_MCK3, .id = 56,
	  .r = { .max = 266000000, }, .chgp = 1, },
	{ .n = "i2smcc0_clk",	.p = PCK_PARENT_HW_MCK1, .id = 57, .r = { .max = 200000000, }, },
	{ .n = "i2smcc1_clk",	.p = PCK_PARENT_HW_MCK1, .id = 58, .r = { .max = 200000000, }, },
	{ .n = "matrix_clk",	.p = PCK_PARENT_HW_MCK1, .id = 60, },
	{ .n = "mcan0_clk",	.p = PCK_PARENT_HW_MCK1, .id = 61, .r = { .max = 200000000, }, },
	{ .n = "mcan1_clk",	.p = PCK_PARENT_HW_MCK1, .id = 62, .r = { .max = 200000000, }, },
	{ .n = "mcan2_clk",	.p = PCK_PARENT_HW_MCK1, .id = 63, .r = { .max = 200000000, }, },
	{ .n = "mcan3_clk",	.p = PCK_PARENT_HW_MCK1, .id = 64, .r = { .max = 200000000, }, },
	{ .n = "mcan4_clk",	.p = PCK_PARENT_HW_MCK1, .id = 65, .r = { .max = 200000000, }, },
	{ .n = "mcan5_clk",	.p = PCK_PARENT_HW_MCK1, .id = 66, .r = { .max = 200000000, }, },
	{ .n = "pdmc0_clk",	.p = PCK_PARENT_HW_MCK1, .id = 68, .r = { .max = 200000000, }, },
	{ .n = "pdmc1_clk",	.p = PCK_PARENT_HW_MCK1, .id = 69, .r = { .max = 200000000, }, },
	{ .n = "pit64b0_clk",	.p = PCK_PARENT_HW_MCK1, .id = 70, },
	{ .n = "pit64b1_clk",	.p = PCK_PARENT_HW_MCK1, .id = 71, },
	{ .n = "pit64b2_clk",	.p = PCK_PARENT_HW_MCK1, .id = 72, },
	{ .n = "pit64b3_clk",	.p = PCK_PARENT_HW_MCK1, .id = 73, },
	{ .n = "pit64b4_clk",	.p = PCK_PARENT_HW_MCK1, .id = 74, },
	{ .n = "pit64b5_clk",	.p = PCK_PARENT_HW_MCK1, .id = 75, },
	{ .n = "pwm_clk",	.p = PCK_PARENT_HW_MCK1, .id = 77, },
	{ .n = "qspi0_clk",	.p = PCK_PARENT_HW_MCK1, .id = 78, },
	{ .n = "qspi1_clk",	.p = PCK_PARENT_HW_MCK1, .id = 79, },
	{ .n = "sdmmc0_clk",	.p = PCK_PARENT_HW_MCK1, .id = 80, },
	{ .n = "sdmmc1_clk",	.p = PCK_PARENT_HW_MCK1, .id = 81, },
	{ .n = "sdmmc2_clk",	.p = PCK_PARENT_HW_MCK1, .id = 82, },
	{ .n = "sha_clk",	.p = PCK_PARENT_HW_MCK1, .id = 83, },
	{ .n = "spdifrx_clk",	.p = PCK_PARENT_HW_MCK1, .id = 84, .r = { .max = 200000000, }, },
	{ .n = "spdiftx_clk",	.p = PCK_PARENT_HW_MCK1, .id = 85, .r = { .max = 200000000, }, },
	{ .n = "ssc0_clk",	.p = PCK_PARENT_HW_MCK1, .id = 86, .r = { .max = 200000000, }, },
	{ .n = "ssc1_clk",	.p = PCK_PARENT_HW_MCK1, .id = 87, .r = { .max = 200000000, }, },
	{ .n = "tcb0_ch0_clk",	.p = PCK_PARENT_HW_MCK1, .id = 88, .r = { .max = 200000000, }, },
	{ .n = "tcb0_ch1_clk",	.p = PCK_PARENT_HW_MCK1, .id = 89, .r = { .max = 200000000, }, },
	{ .n = "tcb0_ch2_clk",	.p = PCK_PARENT_HW_MCK1, .id = 90, .r = { .max = 200000000, }, },
	{ .n = "tcb1_ch0_clk",	.p = PCK_PARENT_HW_MCK1, .id = 91, .r = { .max = 200000000, }, },
	{ .n = "tcb1_ch1_clk",	.p = PCK_PARENT_HW_MCK1, .id = 92, .r = { .max = 200000000, }, },
	{ .n = "tcb1_ch2_clk",	.p = PCK_PARENT_HW_MCK1, .id = 93, .r = { .max = 200000000, }, },
	{ .n = "tcpca_clk",	.p = PCK_PARENT_HW_MCK1, .id = 94, },
	{ .n = "tcpcb_clk",	.p = PCK_PARENT_HW_MCK1, .id = 95, },
	{ .n = "tdes_clk",	.p = PCK_PARENT_HW_MCK1, .id = 96, },
	{ .n = "trng_clk",	.p = PCK_PARENT_HW_MCK1, .id = 97, },
	{ .n = "udphsa_clk",	.p = PCK_PARENT_HW_MCK1, .id = 104, },
	{ .n = "udphsb_clk",	.p = PCK_PARENT_HW_MCK1, .id = 105, },
	{ .n = "uhphs_clk",	.p = PCK_PARENT_HW_MCK1, .id = 106, },
};

/*
 * Generic clock description
 * @n:			clock name
 * @pp:			PLL parents (entry formed by PLL components identifiers
 *			(see enum pll_component_id))
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
} sama7g5_gck[] = {
	{ .n  = "adc_gclk",
	  .id = 26,
	  .r = { .max = 100000000, },
	  .pp = { PLL_IDS_TO_ARR_ENTRY(SYS, DIV0), PLL_IDS_TO_ARR_ENTRY(IMG, DIV0),
		  PLL_IDS_TO_ARR_ENTRY(AUDIO, DIV0), },
	  .pp_mux_table = { 5, 7, 9, },
	  .pp_count = 3,
	  .pp_chg_id = INT_MIN, },

	{ .n  = "asrc_gclk",
	  .id = 30,
	  .r = { .max = 200000000 },
	  .pp = { PLL_IDS_TO_ARR_ENTRY(AUDIO, DIV0), },
	  .pp_mux_table = { 9, },
	  .pp_count = 1,
	  .pp_chg_id = 3, },

	{ .n  = "csi_gclk",
	  .id = 33,
	  .r = { .max = 27000000  },
	  .pp = { PLL_IDS_TO_ARR_ENTRY(DDR, DIV0), PLL_IDS_TO_ARR_ENTRY(IMG, DIV0), },
	  .pp_mux_table = { 6, 7, },
	  .pp_count = 2,
	  .pp_chg_id = INT_MIN, },

	{ .n  = "flex0_gclk",
	  .id = 38,
	  .r = { .max = 200000000 },
	  .pp = { PLL_IDS_TO_ARR_ENTRY(SYS, DIV0), PLL_IDS_TO_ARR_ENTRY(BAUD, DIV0), },
	  .pp_mux_table = { 5, 8, },
	  .pp_count = 2,
	  .pp_chg_id = INT_MIN, },

	{ .n  = "flex1_gclk",
	  .id = 39,
	  .r = { .max = 200000000 },
	  .pp = { PLL_IDS_TO_ARR_ENTRY(SYS, DIV0), PLL_IDS_TO_ARR_ENTRY(BAUD, DIV0), },
	  .pp_mux_table = { 5, 8, },
	  .pp_count = 2,
	  .pp_chg_id = INT_MIN, },

	{ .n  = "flex2_gclk",
	  .id = 40,
	  .r = { .max = 200000000 },
	  .pp = { PLL_IDS_TO_ARR_ENTRY(SYS, DIV0), PLL_IDS_TO_ARR_ENTRY(BAUD, DIV0), },
	  .pp_mux_table = { 5, 8, },
	  .pp_count = 2,
	  .pp_chg_id = INT_MIN, },

	{ .n  = "flex3_gclk",
	  .id = 41,
	  .r = { .max = 200000000 },
	  .pp = { PLL_IDS_TO_ARR_ENTRY(SYS, DIV0), PLL_IDS_TO_ARR_ENTRY(BAUD, DIV0), },
	  .pp_mux_table = { 5, 8, },
	  .pp_count = 2,
	  .pp_chg_id = INT_MIN, },

	{ .n  = "flex4_gclk",
	  .id = 42,
	  .r = { .max = 200000000 },
	  .pp = { PLL_IDS_TO_ARR_ENTRY(SYS, DIV0), PLL_IDS_TO_ARR_ENTRY(BAUD, DIV0), },
	  .pp_mux_table = { 5, 8, },
	  .pp_count = 2,
	  .pp_chg_id = INT_MIN, },

	{ .n  = "flex5_gclk",
	  .id = 43,
	  .r = { .max = 200000000 },
	  .pp = { PLL_IDS_TO_ARR_ENTRY(SYS, DIV0), PLL_IDS_TO_ARR_ENTRY(BAUD, DIV0), },
	  .pp_mux_table = { 5, 8, },
	  .pp_count = 2,
	  .pp_chg_id = INT_MIN, },

	{ .n  = "flex6_gclk",
	  .id = 44,
	  .r = { .max = 200000000 },
	  .pp = { PLL_IDS_TO_ARR_ENTRY(SYS, DIV0), PLL_IDS_TO_ARR_ENTRY(BAUD, DIV0), },
	  .pp_mux_table = { 5, 8, },
	  .pp_count = 2,
	  .pp_chg_id = INT_MIN, },

	{ .n  = "flex7_gclk",
	  .id = 45,
	  .r = { .max = 200000000 },
	  .pp = { PLL_IDS_TO_ARR_ENTRY(SYS, DIV0), PLL_IDS_TO_ARR_ENTRY(BAUD, DIV0), },
	  .pp_mux_table = { 5, 8, },
	  .pp_count = 2,
	  .pp_chg_id = INT_MIN, },

	{ .n  = "flex8_gclk",
	  .id = 46,
	  .r = { .max = 200000000 },
	  .pp = { PLL_IDS_TO_ARR_ENTRY(SYS, DIV0), PLL_IDS_TO_ARR_ENTRY(BAUD, DIV0), },
	  .pp_mux_table = { 5, 8, },
	  .pp_count = 2,
	  .pp_chg_id = INT_MIN, },

	{ .n  = "flex9_gclk",
	  .id = 47,
	  .r = { .max = 200000000 },
	  .pp = { PLL_IDS_TO_ARR_ENTRY(SYS, DIV0), PLL_IDS_TO_ARR_ENTRY(BAUD, DIV0), },
	  .pp_mux_table = { 5, 8, },
	  .pp_count = 2,
	  .pp_chg_id = INT_MIN, },

	{ .n  = "flex10_gclk",
	  .id = 48,
	  .r = { .max = 200000000 },
	  .pp = { PLL_IDS_TO_ARR_ENTRY(SYS, DIV0), PLL_IDS_TO_ARR_ENTRY(BAUD, DIV0), },
	  .pp_mux_table = { 5, 8, },
	  .pp_count = 2,
	  .pp_chg_id = INT_MIN, },

	{ .n  = "flex11_gclk",
	  .id = 49,
	  .r = { .max = 200000000 },
	  .pp = { PLL_IDS_TO_ARR_ENTRY(SYS, DIV0), PLL_IDS_TO_ARR_ENTRY(BAUD, DIV0), },
	  .pp_mux_table = { 5, 8, },
	  .pp_count = 2,
	  .pp_chg_id = INT_MIN, },

	{ .n  = "gmac0_gclk",
	  .id = 51,
	  .r = { .max = 125000000 },
	  .pp = { PLL_IDS_TO_ARR_ENTRY(ETH, DIV0), },
	  .pp_mux_table = { 10, },
	  .pp_count = 1,
	  .pp_chg_id = 3, },

	{ .n  = "gmac1_gclk",
	  .id = 52,
	  .r = { .max = 50000000  },
	  .pp = { PLL_IDS_TO_ARR_ENTRY(ETH, DIV0), },
	  .pp_mux_table = { 10, },
	  .pp_count = 1,
	  .pp_chg_id = INT_MIN, },

	{ .n  = "gmac0_tsu_gclk",
	  .id = 53,
	  .r = { .max = 300000000 },
	  .pp = { PLL_IDS_TO_ARR_ENTRY(AUDIO, DIV0), PLL_IDS_TO_ARR_ENTRY(ETH, DIV0), },
	  .pp_mux_table = { 9, 10, },
	  .pp_count = 2,
	  .pp_chg_id = INT_MIN, },

	{ .n  = "gmac1_tsu_gclk",
	  .id = 54,
	  .r = { .max = 300000000 },
	  .pp = { PLL_IDS_TO_ARR_ENTRY(AUDIO, DIV0), PLL_IDS_TO_ARR_ENTRY(ETH, DIV0), },
	  .pp_mux_table = { 9, 10, },
	  .pp_count = 2,
	  .pp_chg_id = INT_MIN, },

	{ .n  = "i2smcc0_gclk",
	  .id = 57,
	  .r = { .max = 100000000 },
	  .pp = { PLL_IDS_TO_ARR_ENTRY(SYS, DIV0), PLL_IDS_TO_ARR_ENTRY(AUDIO, DIV0), },
	  .pp_mux_table = { 5, 9, },
	  .pp_count = 2,
	  .pp_chg_id = 4, },

	{ .n  = "i2smcc1_gclk",
	  .id = 58,
	  .r = { .max = 100000000 },
	  .pp = { PLL_IDS_TO_ARR_ENTRY(SYS, DIV0), PLL_IDS_TO_ARR_ENTRY(AUDIO, DIV0), },
	  .pp_mux_table = { 5, 9, },
	  .pp_count = 2,
	  .pp_chg_id = 4, },

	{ .n  = "mcan0_gclk",
	  .id = 61,
	  .r = { .max = 200000000 },
	  .pp = { PLL_IDS_TO_ARR_ENTRY(SYS, DIV0), PLL_IDS_TO_ARR_ENTRY(BAUD, DIV0), },
	  .pp_mux_table = { 5, 8, },
	  .pp_count = 2,
	  .pp_chg_id = INT_MIN, },

	{ .n  = "mcan1_gclk",
	  .id = 62,
	  .r = { .max = 200000000 },
	  .pp = { PLL_IDS_TO_ARR_ENTRY(SYS, DIV0), PLL_IDS_TO_ARR_ENTRY(BAUD, DIV0), },
	  .pp_mux_table = { 5, 8, },
	  .pp_count = 2,
	  .pp_chg_id = INT_MIN, },

	{ .n  = "mcan2_gclk",
	  .id = 63,
	  .r = { .max = 200000000 },
	  .pp = { PLL_IDS_TO_ARR_ENTRY(SYS, DIV0), PLL_IDS_TO_ARR_ENTRY(BAUD, DIV0), },
	  .pp_mux_table = { 5, 8, },
	  .pp_count = 2,
	  .pp_chg_id = INT_MIN, },

	{ .n  = "mcan3_gclk",
	  .id = 64,
	  .r = { .max = 200000000 },
	  .pp = { PLL_IDS_TO_ARR_ENTRY(SYS, DIV0), PLL_IDS_TO_ARR_ENTRY(BAUD, DIV0), },
	  .pp_mux_table = { 5, 8, },
	  .pp_count = 2,
	  .pp_chg_id = INT_MIN, },

	{ .n  = "mcan4_gclk",
	  .id = 65,
	  .r = { .max = 200000000 },
	  .pp = { PLL_IDS_TO_ARR_ENTRY(SYS, DIV0), PLL_IDS_TO_ARR_ENTRY(BAUD, DIV0), },
	  .pp_mux_table = { 5, 8, },
	  .pp_count = 2,
	  .pp_chg_id = INT_MIN, },

	{ .n  = "mcan5_gclk",
	  .id = 66,
	  .r = { .max = 200000000 },
	  .pp = { PLL_IDS_TO_ARR_ENTRY(SYS, DIV0), PLL_IDS_TO_ARR_ENTRY(BAUD, DIV0), },
	  .pp_mux_table = { 5, 8, },
	  .pp_count = 2,
	  .pp_chg_id = INT_MIN, },

	{ .n  = "pdmc0_gclk",
	  .id = 68,
	  .r = { .max = 50000000  },
	  .pp = { PLL_IDS_TO_ARR_ENTRY(SYS, DIV0), PLL_IDS_TO_ARR_ENTRY(AUDIO, DIV0), },
	  .pp_mux_table = { 5, 9, },
	  .pp_count = 2,
	  .pp_chg_id = INT_MIN, },

	{ .n  = "pdmc1_gclk",
	  .id = 69,
	  .r = { .max = 50000000, },
	  .pp = { PLL_IDS_TO_ARR_ENTRY(SYS, DIV0), PLL_IDS_TO_ARR_ENTRY(AUDIO, DIV0), },
	  .pp_mux_table = { 5, 9, },
	  .pp_count = 2,
	  .pp_chg_id = INT_MIN, },

	{ .n  = "pit64b0_gclk",
	  .id = 70,
	  .r = { .max = 200000000 },
	  .pp = { PLL_IDS_TO_ARR_ENTRY(SYS, DIV0), PLL_IDS_TO_ARR_ENTRY(IMG, DIV0),
		  PLL_IDS_TO_ARR_ENTRY(BAUD, DIV0), PLL_IDS_TO_ARR_ENTRY(AUDIO, DIV0),
		  PLL_IDS_TO_ARR_ENTRY(ETH, DIV0), },
	  .pp_mux_table = { 5, 7, 8, 9, 10, },
	  .pp_count = 5,
	  .pp_chg_id = INT_MIN, },

	{ .n  = "pit64b1_gclk",
	  .id = 71,
	  .r = { .max = 200000000 },
	  .pp = { PLL_IDS_TO_ARR_ENTRY(SYS, DIV0), PLL_IDS_TO_ARR_ENTRY(IMG, DIV0),
		  PLL_IDS_TO_ARR_ENTRY(BAUD, DIV0), PLL_IDS_TO_ARR_ENTRY(AUDIO, DIV0),
		  PLL_IDS_TO_ARR_ENTRY(ETH, DIV0), },
	  .pp_mux_table = { 5, 7, 8, 9, 10, },
	  .pp_count = 5,
	  .pp_chg_id = INT_MIN, },

	{ .n  = "pit64b2_gclk",
	  .id = 72,
	  .r = { .max = 200000000 },
	  .pp = { PLL_IDS_TO_ARR_ENTRY(SYS, DIV0), PLL_IDS_TO_ARR_ENTRY(IMG, DIV0),
		  PLL_IDS_TO_ARR_ENTRY(BAUD, DIV0), PLL_IDS_TO_ARR_ENTRY(AUDIO, DIV0),
		  PLL_IDS_TO_ARR_ENTRY(ETH, DIV0), },
	  .pp_mux_table = { 5, 7, 8, 9, 10, },
	  .pp_count = 5,
	  .pp_chg_id = INT_MIN, },

	{ .n  = "pit64b3_gclk",
	  .id = 73,
	  .r = { .max = 200000000 },
	  .pp = { PLL_IDS_TO_ARR_ENTRY(SYS, DIV0), PLL_IDS_TO_ARR_ENTRY(IMG, DIV0),
		  PLL_IDS_TO_ARR_ENTRY(BAUD, DIV0), PLL_IDS_TO_ARR_ENTRY(AUDIO, DIV0),
		  PLL_IDS_TO_ARR_ENTRY(ETH, DIV0), },
	  .pp_mux_table = { 5, 7, 8, 9, 10, },
	  .pp_count = 5,
	  .pp_chg_id = INT_MIN, },

	{ .n  = "pit64b4_gclk",
	  .id = 74,
	  .r = { .max = 200000000 },
	  .pp = { PLL_IDS_TO_ARR_ENTRY(SYS, DIV0), PLL_IDS_TO_ARR_ENTRY(IMG, DIV0),
		  PLL_IDS_TO_ARR_ENTRY(BAUD, DIV0), PLL_IDS_TO_ARR_ENTRY(AUDIO, DIV0),
		  PLL_IDS_TO_ARR_ENTRY(ETH, DIV0), },
	  .pp_mux_table = { 5, 7, 8, 9, 10, },
	  .pp_count = 5,
	  .pp_chg_id = INT_MIN, },

	{ .n  = "pit64b5_gclk",
	  .id = 75,
	  .r = { .max = 200000000 },
	  .pp = { PLL_IDS_TO_ARR_ENTRY(SYS, DIV0), PLL_IDS_TO_ARR_ENTRY(IMG, DIV0),
		  PLL_IDS_TO_ARR_ENTRY(BAUD, DIV0), PLL_IDS_TO_ARR_ENTRY(AUDIO, DIV0),
		  PLL_IDS_TO_ARR_ENTRY(ETH, DIV0), },
	  .pp_mux_table = { 5, 7, 8, 9, 10, },
	  .pp_count = 5,
	  .pp_chg_id = INT_MIN, },

	{ .n  = "qspi0_gclk",
	  .id = 78,
	  .r = { .max = 200000000 },
	  .pp = { PLL_IDS_TO_ARR_ENTRY(SYS, DIV0), PLL_IDS_TO_ARR_ENTRY(BAUD, DIV0), },
	  .pp_mux_table = { 5, 8, },
	  .pp_count = 2,
	  .pp_chg_id = INT_MIN, },

	{ .n  = "qspi1_gclk",
	  .id = 79,
	  .r = { .max = 200000000 },
	  .pp = { PLL_IDS_TO_ARR_ENTRY(SYS, DIV0), PLL_IDS_TO_ARR_ENTRY(BAUD, DIV0), },
	  .pp_mux_table = { 5, 8, },
	  .pp_count = 2,
	  .pp_chg_id = INT_MIN, },

	{ .n  = "sdmmc0_gclk",
	  .id = 80,
	  .r = { .max = 208000000 },
	  .pp = { PLL_IDS_TO_ARR_ENTRY(SYS, DIV0), PLL_IDS_TO_ARR_ENTRY(BAUD, DIV0), },
	  .pp_mux_table = { 5, 8, },
	  .pp_count = 2,
	  .pp_chg_id = 4, },

	{ .n  = "sdmmc1_gclk",
	  .id = 81,
	  .r = { .max = 208000000 },
	  .pp = { PLL_IDS_TO_ARR_ENTRY(SYS, DIV0), PLL_IDS_TO_ARR_ENTRY(BAUD, DIV0), },
	  .pp_mux_table = { 5, 8, },
	  .pp_count = 2,
	  .pp_chg_id = 4, },

	{ .n  = "sdmmc2_gclk",
	  .id = 82,
	  .r = { .max = 208000000 },
	  .pp = { PLL_IDS_TO_ARR_ENTRY(SYS, DIV0), PLL_IDS_TO_ARR_ENTRY(BAUD, DIV0), },
	  .pp_mux_table = { 5, 8, },
	  .pp_count = 2,
	  .pp_chg_id = 4, },

	{ .n  = "spdifrx_gclk",
	  .id = 84,
	  .r = { .max = 150000000 },
	  .pp = { PLL_IDS_TO_ARR_ENTRY(SYS, DIV0), PLL_IDS_TO_ARR_ENTRY(AUDIO, DIV0), },
	  .pp_mux_table = { 5, 9, },
	  .pp_count = 2,
	  .pp_chg_id = 4, },

	{ .n = "spdiftx_gclk",
	  .id = 85,
	  .r = { .max = 25000000  },
	  .pp = { PLL_IDS_TO_ARR_ENTRY(SYS, DIV0), PLL_IDS_TO_ARR_ENTRY(AUDIO, DIV0), },
	  .pp_mux_table = { 5, 9, },
	  .pp_count = 2,
	  .pp_chg_id = 4, },

	{ .n  = "tcb0_ch0_gclk",
	  .id = 88,
	  .r = { .max = 200000000 },
	  .pp = { PLL_IDS_TO_ARR_ENTRY(SYS, DIV0), PLL_IDS_TO_ARR_ENTRY(IMG, DIV0),
		  PLL_IDS_TO_ARR_ENTRY(BAUD, DIV0), PLL_IDS_TO_ARR_ENTRY(AUDIO, DIV0),
		  PLL_IDS_TO_ARR_ENTRY(ETH, DIV0), },
	  .pp_mux_table = { 5, 7, 8, 9, 10, },
	  .pp_count = 5,
	  .pp_chg_id = INT_MIN, },

	{ .n  = "tcb1_ch0_gclk",
	  .id = 91,
	  .r = { .max = 200000000 },
	  .pp = { PLL_IDS_TO_ARR_ENTRY(SYS, DIV0), PLL_IDS_TO_ARR_ENTRY(IMG, DIV0),
		  PLL_IDS_TO_ARR_ENTRY(BAUD, DIV0), PLL_IDS_TO_ARR_ENTRY(AUDIO, DIV0),
		  PLL_IDS_TO_ARR_ENTRY(ETH, DIV0), },
	  .pp_mux_table = { 5, 7, 8, 9, 10, },
	  .pp_count = 5,
	  .pp_chg_id = INT_MIN, },

	{ .n  = "tcpca_gclk",
	  .id = 94,
	  .r = { .max = 32768, },
	  .pp_chg_id = INT_MIN, },

	{ .n  = "tcpcb_gclk",
	  .id = 95,
	  .r = { .max = 32768, },
	  .pp_chg_id = INT_MIN, },
};

/* MCK0 characteristics. */
static const struct clk_master_characteristics mck0_characteristics = {
	.output = { .min = 32768, .max = 200000000 },
	.divisors = { 1, 2, 4, 3, 5 },
	.have_div3_pres = 1,
};

/* MCK0 layout. */
static const struct clk_master_layout mck0_layout = {
	.mask = 0x773,
	.pres_shift = 4,
	.offset = 0x28,
};

/* Programmable clock layout. */
static const struct clk_programmable_layout programmable_layout = {
	.pres_mask = 0xff,
	.pres_shift = 8,
	.css_mask = 0x1f,
	.have_slck_mck = 0,
	.is_pres_direct = 1,
};

/* Peripheral clock layout. */
static const struct clk_pcr_layout sama7g5_pcr_layout = {
	.offset = 0x88,
	.cmd = BIT(31),
	.gckcss_mask = GENMASK(12, 8),
	.pid_mask = GENMASK(6, 0),
};

enum clock_count {
	CLK_CNT_SYSTEM = 15 + 1,
	CLK_CNT_PERIPH = 106 + 1,
	CLK_CNT_GCK = 95 + 1,
};

static struct device *pmc_table[PMC_MCK1 + 1 + CLK_CNT_SYSTEM + CLK_CNT_PERIPH + CLK_CNT_GCK + 8];

void sam_pmc_setup(const struct device *dev)
{
	const struct sam_pmc_cfg *cfg = dev->config;
	struct sam_pmc_data *data = dev->data;
	const struct device *main_xtal = cfg->main_xtal;
	const struct device *td_slck = cfg->td_slck;
	const struct device *md_slck = cfg->md_slck;

	void *const regmap = cfg->reg;
	const struct device *parents[10];
	struct pmc_data *sama7g5_pmc;
	struct device *clk, *main_rc, *main_osc;
	uint32_t mux_table[8];

	int ret = 0;
	uint32_t rate, bypass;
	int i, j;


	if (!td_slck || !md_slck || !main_xtal || !regmap) {
		LOG_ERR("Incorrect parameters.");
		return;
	}

	if (CLK_CNT_SYSTEM != nck(sama7g5_systemck) ||
	    CLK_CNT_PERIPH != nck(sama7g5_periphck) ||
	    CLK_CNT_GCK != nck(sama7g5_gck)) {
		LOG_ERR("Incorrect definitions could make array for pmc clocks not enough");
		return;
	}
	sama7g5_pmc = pmc_data_allocate(PMC_MCK1 + 1,
					nck(sama7g5_systemck),
					nck(sama7g5_periphck),
					nck(sama7g5_gck), 8, &pmc_table[0]);
	if (!sama7g5_pmc) {
		LOG_ERR("allocate PMC data failed.");
		return;
	}
	data->pmc = sama7g5_pmc;

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

	sama7g5_pmc->chws[PMC_MAIN] = clk;

	for (i = 0; i < PLL_ID_MAX; i++) {
		for (j = 0; j < PLL_COMPID_MAX; j++) {
			const struct device *parent_hw;

			if (!sama7g5_plls[i][j].n) {
				continue;
			}

			switch (sama7g5_plls[i][j].t) {
			case PLL_TYPE_FRAC:
				switch (sama7g5_plls[i][j].p) {
				case SAMA7G5_PLL_PARENT_MAINCK:
					parent_hw = sama7g5_pmc->chws[PMC_MAIN];
					break;
				case SAMA7G5_PLL_PARENT_MAIN_XTAL:
					parent_hw = main_xtal;
					break;
				default:
					/* Should not happen. */
					parent_hw = NULL;
					break;
				}

				ret = sam9x60_clk_register_frac_pll(regmap,
								    &pmc_pll_lock,
								    sama7g5_plls[i][j].n,
								    parent_hw, i,
								    sama7g5_plls[i][j].c,
								    sama7g5_plls[i][j].l,
								    &clk);
				break;

			case PLL_TYPE_DIV:
				ret = sam9x60_clk_register_div_pll(regmap,
								   &pmc_pll_lock,
								   sama7g5_plls[i][j].n,
								   sama7g5_plls[i][0].clk, i,
								   sama7g5_plls[i][j].c,
								   sama7g5_plls[i][j].l,
								   &clk);
				break;

			default:
				continue;
			}

			if (ret) {
				LOG_ERR("Register clock %s failed.", sama7g5_plls[i][j].n);
				return;
			}

			sama7g5_plls[i][j].clk = clk;
			if (sama7g5_plls[i][j].eid) {
				sama7g5_pmc->chws[sama7g5_plls[i][j].eid] = clk;
			}
		}
	}

	ret = clk_register_master_div(regmap, "mck0",
				      sama7g5_plls[PLL_ID_CPU][1].clk,
				      &mck0_layout, &mck0_characteristics,
				      &pmc_mck0_lock, 5, &clk);
	if (ret) {
		LOG_ERR("Register MCK0 clock failed.");
		return;
	}
	sama7g5_mckx[PCK_PARENT_HW_MCK0].clk = sama7g5_pmc->chws[PMC_MCK] = clk;

	parents[0] = md_slck;
	parents[1] = td_slck;
	parents[2] = sama7g5_pmc->chws[PMC_MAIN];
	for (i = PCK_PARENT_HW_MCK1; i < ARRAY_SIZE(sama7g5_mckx); i++) {
		uint8_t num_parents = 3 + sama7g5_mckx[i].ep_count;
		struct device *tmp_parent_hws[8];

		if (num_parents > ARRAY_SIZE(mux_table)) {
			LOG_ERR("Array for mux table not enough");
			return;
		}

		PMC_INIT_TABLE(mux_table, 3);
		PMC_FILL_TABLE(&mux_table[3], sama7g5_mckx[i].ep_mux_table,
			       sama7g5_mckx[i].ep_count);
		for (j = 0; j < sama7g5_mckx[i].ep_count; j++) {
			uint8_t pll_id = sama7g5_mckx[i].ep[j].pll_id;
			uint8_t pll_compid = sama7g5_mckx[i].ep[j].pll_compid;

			tmp_parent_hws[j] = sama7g5_plls[pll_id][pll_compid].clk;
		}
		PMC_FILL_TABLE(&parents[3], tmp_parent_hws,
			       sama7g5_mckx[i].ep_count);

		ret = clk_register_master(regmap, sama7g5_mckx[i].n,
					  num_parents, parents,
					  mux_table,
					  &pmc_mckX_lock, sama7g5_mckx[i].id,
					  &clk);
		if (ret) {
			LOG_ERR("Register MCK%d clock failed.", i);
			return;
		}

		sama7g5_mckx[i].clk = clk;
		if (sama7g5_mckx[i].eid) {
			sama7g5_pmc->chws[sama7g5_mckx[i].eid] = clk;
		}
	}

	ret = clk_register_utmi(regmap, "utmick", main_xtal, &clk);
	if (ret) {
		LOG_ERR("Register UTMI clock failed.");
		return;
	}

	sama7g5_pmc->chws[PMC_UTMI] = clk;

	parents[0] = md_slck;
	parents[1] = td_slck;
	parents[2] = sama7g5_pmc->chws[PMC_MAIN];
	parents[3] = sama7g5_plls[PLL_ID_SYS][PLL_COMPID_DIV0].clk;
	parents[4] = sama7g5_plls[PLL_ID_DDR][PLL_COMPID_DIV0].clk;
	parents[5] = sama7g5_plls[PLL_ID_IMG][PLL_COMPID_DIV0].clk;
	parents[6] = sama7g5_plls[PLL_ID_BAUD][PLL_COMPID_DIV0].clk;
	parents[7] = sama7g5_plls[PLL_ID_AUDIO][PLL_COMPID_DIV0].clk;
	parents[8] = sama7g5_plls[PLL_ID_ETH][PLL_COMPID_DIV0].clk;
	for (i = 0; i < 8; i++) {
		ret = clk_register_programmable(regmap, parents,
						9, i,
						&programmable_layout,
						sama7g5_prog_mux_table, &clk);
		if (ret) {
			LOG_ERR("Register programmable clock %d failed.", i);
			return;
		}

		sama7g5_pmc->pchws[i] = clk;
	}

	for (i = 0; i < ARRAY_SIZE(sama7g5_systemck); i++) {
		ret = clk_register_system(regmap, sama7g5_systemck[i].n,
					  sama7g5_pmc->pchws[i],
					  sama7g5_systemck[i].id, &clk);
		if (ret) {
			LOG_ERR("Register system clock %d failed.", i);
			return;
		}

		sama7g5_pmc->shws[sama7g5_systemck[i].id] = clk;
	}

	for (i = 0; i < ARRAY_SIZE(sama7g5_periphck); i++) {
		clk = sama7g5_mckx[sama7g5_periphck[i].p].clk;
		ret = clk_register_peripheral(regmap, &pmc_pcr_lock,
					      &sama7g5_pcr_layout,
					      sama7g5_periphck[i].n,
					      clk,
					      sama7g5_periphck[i].id,
					      &sama7g5_periphck[i].r,
					      &clk);
		if (ret) {
			LOG_ERR("Register peripheral clock failed.");
			return;
		}
		sama7g5_pmc->phws[sama7g5_periphck[i].id] = clk;
	}

	parents[0] = md_slck;
	parents[1] = td_slck;
	parents[2] = sama7g5_pmc->chws[PMC_MAIN];

	for (i = 0; i < ARRAY_SIZE(sama7g5_gck); i++) {
		uint8_t num_parents = 3 + sama7g5_gck[i].pp_count;
		struct device *tmp_parent_hws[8];

		if (num_parents > ARRAY_SIZE(mux_table)) {
			LOG_ERR("Array for mux table not enough");
			return;
		}

		PMC_INIT_TABLE(mux_table, 3);
		PMC_FILL_TABLE(&mux_table[3], sama7g5_gck[i].pp_mux_table,
			       sama7g5_gck[i].pp_count);
		for (j = 0; j < sama7g5_gck[i].pp_count; j++) {
			uint8_t pll_id = sama7g5_gck[i].pp[j].pll_id;
			uint8_t pll_compid = sama7g5_gck[i].pp[j].pll_compid;

			tmp_parent_hws[j] = sama7g5_plls[pll_id][pll_compid].clk;
		}
		PMC_FILL_TABLE(&parents[3], tmp_parent_hws, sama7g5_gck[i].pp_count);

		ret = clk_register_generated(regmap, &pmc_pcr_lock,
					     &sama7g5_pcr_layout,
					     sama7g5_gck[i].n,
					     parents, mux_table,
					     num_parents,
					     sama7g5_gck[i].id,
					     &sama7g5_gck[i].r,
					     sama7g5_gck[i].pp_chg_id, &clk);
		if (ret) {
			LOG_ERR("Register generated clock failed.");
			return;
		}
		sama7g5_pmc->ghws[sama7g5_gck[i].id] = clk;
	}

	LOG_DBG("Register PMC clocks successfully.");
}
