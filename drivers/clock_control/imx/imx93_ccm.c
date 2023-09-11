/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Soc layer implementation of the CCM Rev3 operations for i.MX93.
 *
 * The following sections provide turotial-like pieces of information which
 * may be useful when working with the CCM Rev3's SoC layer for i.MX93.
 *
 *
 * 1) PLL tree structure
 *	- the following diagram shows how PLLs are generically structured
 *	(not 100% accurate, not applicable to all SoCs, used to merely provide
 *	an intuition)
 *
 *	VCO_PRE_DIV_OUT ----> VCO_POST_DIV_OUT ----> PFD_OUT ----> PFD_DIV2_OUT
 *                                               |             |
 *						 |             |
 *						 |             |
 *						 ----> TO IPs  ----> TO IPs
 *
 *	- out of all of the above clock signals, IPs usually make use of
 *	VCO_POST_DIV_OUT, PFD_OUT and PFD_DIV2_OUT.
 *
 *	- the PLL outputs from the right side depend on the PLLs outputs from
 *	the left side. For example, PFD_DIV2_OUT depends on PFD_OUT, which
 *	depends on VCO_POST_DIV_OUT, which depends on VCO_PRE_DIV_OUT.
 *
 *	- this dependency indicates that a 3-leveled tree-like structure should
 *	be used to represent the PLLs.
 *
 *	- in the case of i.MX93, the only PLLs outputs used by the IPs are
 *	PFD_DIV2_OUT and VCO_POST_DIV_OUT. As such, to avoid making the SoC
 *	layer overly-complicated, a flattened structure is used to represent
 *	the PLLs (see the plls array).
 *
 *	- although the structure is flat (it has only 1 level), this doesn't
 *	mean the dependencies should be ignored. As such, it's mandatory that
 *	the pre-defined PLL configurations be consistent with each other.
 *	We'll take SYSTEM_PLL1 as an example. To configure SYSTEM_PLL1_PFDx
 *	you have to first configure SYSTEM_PLL1_VCO. Since there's multiple
 *	PFD outputs for SYSTEM_PLL1 (from 0 to 2), that means SYSTEM_PLL1_VCO
 *	must have the same configuration. For instance:
 *
 *		We want SYSTEM_PLL1_PFD0 to yield a frequency of 500Mhz and
 *		SYSTEM_PLL1_PFD1 to yield a frequency of 400Mhz. This means
 *		that when configuring the PFD outputs we need to use the same
 *		SYSTEM_PLL1_VCO frequency (basically the vco_cfg should remain
 *		unmodified) such that configuring one PFD clock doesn't
 *		misconfigure the other.
 *
 *	- unfortunately, this is not enforced by the SoC layer. As such, one
 *	must make sure that the vco_cfg stays the same for all PFD
 *	configurations.
 *
 * 2) Adding a new clock
 *	- whenever one needs to add a new clock, the following steps should be
 *	taken:
 *		a) Identify the clock type.
 *			- is the clock an IP clock, a ROOT clock, a PLL or a
 *			FIXED clock?
 *
 *		b) Add an entry in the appropriate clock array.
 *			- during this step, one needs to make sure the fields
 *			of the structure are filled in correctly.
 *			- depending on the clock type, additionally steps may
 *			be necessary:
 *				I) The clock is a ROOT clock.
 *					- apart from adding an entry to the
 *					roots array, one must also specify
 *					the MUX options by filling in the
 *					root_mux array.
 *					- the starting index of the root's
 *					mux options is compute as 4 * index
 *					of the root clock in the roots array.
 *					- if the mux option is not supported,
 *					one needs to set the mux entry to NULL.
 *
 *				II) The clock is an IP clock.
 *					- to allow clock configuration (i.e:
 *					setting its frequency or querying its
 *					frequency) one needs to set the IP
 *					clock's parent which is a root clock.
 *					- if you only care about gating/ungating
 *					the IP clock then you can leave the
 *					parent as NULL (see EDMA2 clock)
 *		c) Add macros in imx93_ccm.h
 *			- to add new macros, please use the util
 *			IMX93_CCM_CLOCK, which takes an index and a clock
 *			type as its parameters.
 *			- the index specified through IMX93_CCM_CLOCK must
 *			match the clock's index in the array.
 *			- for example, if CLOCK_ROOT_DUMMY is at index 5
 *			in the roots array, the macro definition would look
 *			like this:
 *				#define IMX93_CCM_DUMMY_ROOT IMX93_CCM_CLOCK(5, ROOT)
 *
 * 3) Configuration examples
 *
 *	a) Configuring clocks which are already initialized by some other
 *	entity.
 *		ccm: clock-controller {
 *			clocks-assume-on = <IMX93_CCM_CLOCK1 RATE1>,
 *					<IMX93_CCM_CLOCK2 RATE>;
 *		};
 *
 *	b) Ungating clocks upon CCM Rev3 driver initialization
 *		ccm: clock-controller {
 *			assigned-clocks = <IMX93_CCM_CLOCK1_ROOT>;
 *			assigned-clock-parents = <IMX93_CCM_CLOCK1_MUX1>;
 *			assigned-clock-rates = <IMX93_CCM_CLOCK1_ROOT_RATE>;
 *			clocks-init-on = <IMX93_CCM_CLOCK1>;
 *		};
 *
 *	c) Configuring PLLs
 *		ccm: clock-controller {
 *			assigned-clocks = <IMX93_CCM_PLL1>;
 *			assigned-clock-parents = <IMX93_CCM_DUMMY_CLOCK>;
 *			assigned-clock-rates = <IMX93_CCM_PLL1_RATE>;
 *		};
 */
#include <zephyr/drivers/clock_control/clock_control_mcux_ccm_rev3.h>
#include <zephyr/dt-bindings/clock/imx93_ccm.h>
#include <fsl_clock.h>
#include <errno.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(imx93_ccm);

#define IMX93_CCM_SRC_NUM 4
#define IMX93_CCM_DIV_MAX 255
#define IMX93_CCM_PLL_MAX_CFG 1
#define IMX93_CCM_PFD_INVAL 0xdeadbeef
#define IMX93_CCM_ERROR_THR MHZ(5)

enum imx93_ccm_pll_type {
	IMX93_CCM_PLL_FRACN = 0,
	IMX93_CCM_PLL_INT,
};

struct imx93_ccm_pll_config {
	/** VCO-specific configuration. */
	fracn_pll_init_t vco_cfg;
	/** PFD-specific configuration. */
	fracn_pll_pfd_init_t pfd_cfg;
	/** Frequency the configuration yields. */
	uint32_t freq;
};

struct imx93_ccm_pll {
	/** Clock data. */
	struct imx_ccm_clock clk;
	/** Offset from PLL base. */
	uint32_t offset;
	/** PFD number. Should be IMX93_CCM_PFD_INVAL if PFD is not used. */
	uint32_t pfd;
	/** Number of pre-defined configurations */
	uint32_t config_num;
	/** Type of PLL. Either integer or fractional. */
	enum imx93_ccm_pll_type type;
	/** Array of pre-defined configurations */
	struct imx93_ccm_pll_config configs[IMX93_CCM_PLL_MAX_CFG];
};

static struct imx93_ccm_pll plls[] = {
	/* SYSTEM_PLL1 PFD0 divided by 2 output */
	{
		.clk.id = kCLOCK_SysPll1Pfd0Div2,
		.clk.name = "sys_pll1_pfd0_div2",
		.offset = 0x1100,
		.pfd = 0,
		.configs = {
			{
				.vco_cfg.rdiv = 1,
				.vco_cfg.mfi = 166,
				.vco_cfg.mfn = 2,
				.vco_cfg.mfd = 3,
				.vco_cfg.odiv = 4,

				.pfd_cfg.mfi = 4,
				.pfd_cfg.mfn = 0,
				.pfd_cfg.div2_en = true,
				.freq = MHZ(500),
			},
		},
	},
	/* SYSTEM_PLL1 PFD1 divided by 2 output */
	{
		.clk.id = kCLOCK_SysPll1Pfd1Div2,
		.clk.name = "sys_pll1_pfd1_div2",
		.offset = 0x1100,
		.pfd = 1,
		.configs = {
			{
				.vco_cfg.rdiv = 1,
				.vco_cfg.mfi = 166,
				.vco_cfg.mfn = 2,
				.vco_cfg.mfd = 3,
				.vco_cfg.odiv = 4,

				.pfd_cfg.mfi = 5,
				.pfd_cfg.mfn = 0,
				.pfd_cfg.div2_en = true,
				.freq = MHZ(400),
			},
		},
		.config_num = 1,
	},
	/* AUDIO_PLL VCO post-divider output */
	{
		.clk.id = kCLOCK_AudioPll1Out,
		.clk.name = "audio_pll",
		.offset = 0x1200,
		.pfd = IMX93_CCM_PFD_INVAL,
		.configs = {
			{
				.vco_cfg.rdiv = 1,
				.vco_cfg.mfi = 81,
				.vco_cfg.mfn = 92,
				.vco_cfg.mfd = 100,
				.vco_cfg.odiv = 5,
				.freq = 393216000,
			},
		},
		.config_num = 1,
	},
};

static struct imx_ccm_clock fixed[] = {
	/* 24MHz XTAL */
	{
		.id = kCLOCK_Osc24M,
		.freq = MHZ(24),
		.name = "osc_24m",
	},
};

static struct imx_ccm_clock *root_mux[] = {
	/* LPUART1 root clock sources */
	&fixed[0],
	(struct imx_ccm_clock *)&plls[0],
	(struct imx_ccm_clock *)&plls[1],
	NULL, /* note: VIDEO_PLL currently not supported */

	/* LPUART1 root clock sources */
	&fixed[0],
	(struct imx_ccm_clock *)&plls[0],
	(struct imx_ccm_clock *)&plls[1],
	NULL, /* note: VIDEO_PLL currently not supported */

	/* SAI3 root clock sources */
	&fixed[0],
	(struct imx_ccm_clock *)&plls[2],
	NULL, /* note: VIDEO_PLL currently not supported */
	NULL, /* note: EXT_CLK currently not supported */
};

static struct imx_ccm_clock roots[] = {
	{
		.id = kCLOCK_Root_Lpuart1,
		.name = "lpuart1_root",
	},
	{
		.id = kCLOCK_Root_Lpuart2,
		.name = "lpuart2_root",
	},
	{
		.id = kCLOCK_Root_Sai3,
		.name = "sai3_root",
	},
};

static struct imx_ccm_clock clocks[] = {
	{
		.id = kCLOCK_Lpuart1,
		.parent = &roots[0],
		.name = "lpuart1",
	},
	{
		.id = kCLOCK_Lpuart2,
		.parent = &roots[1],
		.name = "lpuart2",
	},
	{
		.id = kCLOCK_Edma2,
		.name = "edma2",
		/* TODO: add parent if clock requires configuration */
	},
	{
		.id = kCLOCK_Sai3,
		.parent = &roots[2],
		.name = "sai3",
	},
};

static struct imx_ccm_clock dummy_clock = {
	.name = "dummy_clock",
};

static struct imx93_ccm_pll_config
	*imx93_ccm_get_pll_config(struct imx93_ccm_pll *pll, uint32_t rate)
{
	int i;

	for (i = 0; i < pll->config_num; i++) {
		if (pll->configs[i].freq == rate) {
			return &pll->configs[i];
		}
	}

	return NULL;
}

static int imx93_ccm_get_clock_type(struct imx_ccm_clock *clk)
{
	if (PART_OF_ARRAY(clocks, clk)) {
		return IMX93_CCM_TYPE_IP;
	} else if (PART_OF_ARRAY(roots, clk)) {
		return IMX93_CCM_TYPE_ROOT;
	} else if (PART_OF_ARRAY(fixed, clk)) {
		return IMX93_CCM_TYPE_FIXED;
	} else if (PART_OF_ARRAY(plls, clk)) {
		return IMX93_CCM_TYPE_PLL;
	}

	__ASSERT(false, "invalid clock type for clock %s", clk->name);

	/* this should never be reached. Despite this, we'll keep the
	 * return statement to avoid complaints from the compiler.
	 */
	return -EINVAL;
}

static int imx93_ccm_get_clock(uint32_t clk_id, struct imx_ccm_clock **clk)
{
	uint32_t clk_type, clk_idx;

	if (clk_id == IMX93_CCM_DUMMY_CLOCK) {
		*clk = &dummy_clock;
		return 0;
	}

	clk_idx = clk_id & ~IMX93_CCM_TYPE_MASK;
	clk_type = clk_id & IMX93_CCM_TYPE_MASK;

	switch (clk_type) {
	case IMX93_CCM_TYPE_IP:
		if (clk_idx >= ARRAY_SIZE(clocks)) {
			return -EINVAL;
		}
		*clk = (struct imx_ccm_clock *)&clocks[clk_idx];
		break;
	case IMX93_CCM_TYPE_ROOT:
		if (clk_idx >= ARRAY_SIZE(roots)) {
			return -EINVAL;
		}
		*clk = (struct imx_ccm_clock *)&roots[clk_idx];
		break;
	case IMX93_CCM_TYPE_FIXED:
		if (clk_idx >= ARRAY_SIZE(fixed)) {
			return -EINVAL;
		}
		*clk = &fixed[clk_idx];
		break;
	case IMX93_CCM_TYPE_PLL:
		if (clk_idx >= ARRAY_SIZE(plls)) {
			return -EINVAL;
		}
		*clk = (struct imx_ccm_clock *)&plls[clk_idx];
		break;
	default:
		return -EINVAL;
	};

	return 0;
}

static bool imx93_ccm_rate_is_valid(const struct device *dev,
				    struct imx_ccm_clock *clk,
				    uint32_t rate)
{
	struct imx_ccm_data *data;
	int clk_type;

	data = dev->data;

	clk_type = imx93_ccm_get_clock_type(clk);
	if (clk_type < 0) {
		return clk_type;
	}

	switch (clk_type) {
	case IMX93_CCM_TYPE_IP:
		__ASSERT(clk->parent,
			 "IP clock %s doesn't have a root parent", clk->name);

		clk = clk->parent;
	case IMX93_CCM_TYPE_ROOT:
		if (!clk->parent) {
			return false;
		}

		clk = clk->parent;

		/* since we don't want to allow PLL configuration
		 * through tree traversal from higher levels, we
		 * need to check if root's source has been confiugred.
		 * If not, then we're not allowed to configure the root
		 * clock either.
		 */
		if (!clk->freq) {
			return false;
		}

		return rate <= clk->freq &&
			DIV_ROUND_UP(clk->freq, rate) < IMX93_CCM_DIV_MAX;
	case IMX93_CCM_TYPE_FIXED:
		/* you're not allowed to set a fixed clock's frequency */
		return false;
	case IMX93_CCM_TYPE_PLL:
		/* requested rate is valid only if the PLL contains a config
		 * such that the yielded rate is equal to the requested rate
		 */
		return imx93_ccm_get_pll_config((struct imx93_ccm_pll *)clk,
						rate) ? true : false;
	default:
		/* this should never happen */
		__ASSERT(false, "invalid clock type: 0x%x", clk_type);
	}

	return false;
}

static int imx93_ccm_on_off(const struct device *dev,
			    struct imx_ccm_clock *clk, bool on)
{
	int clk_type;

	clk_type = imx93_ccm_get_clock_type(clk);
	if (clk_type < 0) {
		return clk_type;
	}

	switch (clk_type) {
	case IMX93_CCM_TYPE_IP:
		if (on) {
			CLOCK_EnableClock(clk->id);
		} else {
			CLOCK_DisableClock(clk->id);
		}
		return 0;
	case IMX93_CCM_TYPE_ROOT:
		if (on) {
			CLOCK_PowerOnRootClock(clk->id);
		} else {
			CLOCK_PowerOffRootClock(clk->id);
		}
		return 0;
	case IMX93_CCM_TYPE_PLL:
	case IMX93_CCM_TYPE_FIXED:
		return 0;
	default:
		/* this should never happen */
		__ASSERT(false, "invalid clock type: 0x%x", clk_type);
	}

	return -EINVAL;
}

static struct imx_ccm_clock *get_root_child(struct imx_ccm_clock *root)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(clocks); i++) {
		if (root == clocks[i].parent) {
			return &clocks[i];
		}
	}

	return NULL;
}

static int imx93_ccm_set_root_clock_rate(struct imx_ccm_clock *root,
					 uint32_t rate)
{
	uint32_t divider, mux;
	uint32_t obtained_rate;
	struct imx_ccm_clock *child;

	if (!root->parent) {
		return -EINVAL;
	}

	/* unfortunately, although computed during
	 * get_parent_rate(), the DIV value needs to be
	 * computed again here as there's no way we can
	 * transmit it to the ROOT clock.
	 *
	 * TODO: should we transmit the DIV value directly?
	 */
	divider = DIV_ROUND_UP(root->parent->freq, rate);

	/* the resulting DIV value should have already been validated by
	 * imx_ccm_rate_is_valid() or imx93_ccm_set_root_clock_rate().
	 */
	__ASSERT(divider < IMX93_CCM_DIV_MAX, "invalid DIV value: %d", divider);


	/* TODO: what is better: reporting a smaller rate
	 * or a bigger rate? Should the user be warned that
	 * the obtained rate will be different from the
	 * requested rate?
	 */
	obtained_rate = root->parent->freq / divider;

	if (abs(obtained_rate - rate) > IMX93_CCM_ERROR_THR) {
		LOG_WRN("rate error for clock %s exceeds threshold", root->name);
	}

	/* this should never happen as imx_ccm_get_parent_rate()
	 * should have already performed this check.
	 */
	__ASSERT(obtained_rate != root->freq,
		 "root clock %s already set to rate %u",
		 root->name, obtained_rate);

	if (obtained_rate == root->freq) {
		return -EALREADY;
	}

	child = get_root_child(root);

	mux = CLOCK_GetRootClockMux(root->id);

	CLOCK_SetRootClockDiv(root->id, divider);

	/* note: we also want to set the IP clock child's
	 * frequency here because we don't want to have to
	 * also initialize IP clocks through the assigned-clock*
	 * properties. Usually, one configures the root clock
	 * through said properties and in the drivers for
	 * the peripherals it's expected that the IP clock
	 * will have the frequency of the root clock.
	 */
	if (child) {
		child->freq = obtained_rate;
	}

	root->freq = obtained_rate;

	return obtained_rate;
}

static int imx93_ccm_set_pll_rate(const struct device *dev,
				  struct imx93_ccm_pll *pll,
				  uint32_t rate)
{
	struct imx93_ccm_pll_config *config;
	struct imx_ccm_data *data;

	data = dev->data;

	config = imx93_ccm_get_pll_config(pll, rate);

	/* setting a PLL's rate is done directly with a
	 * mcux_ccm_set_rate(PLL_ID) call. As such, the
	 * imx_ccm_rate_is_valid() call assures us that
	 * imx_ccm_set_clock_rate() will receive a valid PLL rate.
	 * Thanks to this, config should never be NULL.
	 */
	__ASSERT(config, "no configuration for PLL requested rate: %u", rate);

	switch (pll->type) {
	case IMX93_CCM_PLL_INT:
		/* TODO: add support for integer PLLs */
		return -ENOTSUP;
	case IMX93_CCM_PLL_FRACN:
		CLOCK_PllInit((PLL_Type *)(data->pll_regmap + pll->offset),
			      &config->vco_cfg);

		if (pll->pfd != IMX93_CCM_PFD_INVAL) {
			/* we're dealing with a PLL's PFD output */
			CLOCK_PllPfdInit((PLL_Type *)(data->pll_regmap + pll->offset),
					 pll->pfd, &config->pfd_cfg);
		}

		return pll->clk.freq = rate;
	default:
		/* this should never be reached */
		__ASSERT(false, "invalid PLL type: 0x%x", pll->type);
	}

	/* althogh this should never be reached, the compiler keeps complaining
	 * if we remove this return. As such, keep it.
	 */
	return -EINVAL;
}

static int imx93_ccm_set_clock_rate(const struct device *dev,
				    struct imx_ccm_clock *clk,
				    uint32_t rate)
{
	struct imx_ccm_data *data;
	int clk_type;

	data = dev->data;

	clk_type = imx93_ccm_get_clock_type(clk);
	if (clk_type < 0) {
		return clk_type;
	}

	switch (clk_type) {
	case IMX93_CCM_TYPE_IP:
		__ASSERT(clk->parent, "IP clock %s has no parent", clk->name);

		/* this assert should only fail if one tries to configure
		 * an IP clock using the raw imx_ccm_set_clock_rate() which
		 * is wrong.
		 */
		__ASSERT(clk->parent->freq,
			 "unconfigured root clock for IP clock %s", clk->name);

		/* IP's frequency is set during set_clock_rate(ROOT[IP]) */
		return clk->freq;
	case IMX93_CCM_TYPE_ROOT:
		return imx93_ccm_set_root_clock_rate(clk, rate);
	case IMX93_CCM_TYPE_FIXED:
		/* can't set a fixed clock's frequency */
		return -EINVAL;
	case IMX93_CCM_TYPE_PLL:
		return imx93_ccm_set_pll_rate(dev, (struct imx93_ccm_pll *)clk, rate);
	default:
		/* this should never be reached */
		__ASSERT(false, "unexpected clock type: %u", clock_type);
	}

	return -EINVAL;
}

static int imx93_ccm_assign_parent(const struct device *dev,
				   struct imx_ccm_clock *clk,
				   struct imx_ccm_clock *parent)
{
	int i, clk_type, root_idx;

	clk_type = imx93_ccm_get_clock_type(clk);
	if (clk_type < 0) {
		return clk_type;
	}

	/* the dummy clock can be assigned as any clock's parent */
	if (parent == &dummy_clock) {
		return 0;
	}

	switch (clk_type) {
	case IMX93_CCM_TYPE_ROOT:
		root_idx = ARRAY_INDEX(roots, clk);

		for (i = 0; i < IMX93_CCM_SRC_NUM; i++) {
			if (root_mux[root_idx * IMX93_CCM_SRC_NUM + i] == parent) {
				CLOCK_SetRootClockMux(clk->id, i);
				clk->parent = parent;
				return 0;
			}
		}
		return -EINVAL;
	/* the following clocks don't allow parent assignment
	 *
	 * trying to assign something different from IMX93_CCM_DUMMY_CLOCK
	 * as a clock parent is considered to be an error.
	 */
	case IMX93_CCM_TYPE_IP:
		/* IP clocks are bound by default to an unchangable parent.
		 * Tying to assign a different parent (except for
		 * IMX93_CCM_DUMMY_CLOCK) is a mistake.
		 */
		if (clk->parent != parent) {
			return -EINVAL;
		}
		return 0;
	case IMX93_CCM_TYPE_PLL:
	case IMX93_CCM_TYPE_FIXED:
		/* trying to assign a parent to a source clock is strictly
		 * forbidden (exception: IMX93_CCM_DUMMY_CLOCK).
		 */
		__ASSERT(false, "trying to assign parent to source clock");
	default:
		/* this should never be reached */
		__ASSERT(false, "unexpected clock type: %u", clock_type);
	}

	return -EINVAL;
}


static int imx93_ccm_get_parent_rate(struct imx_ccm_clock *clk,
				     struct imx_ccm_clock *parent,
				     uint32_t rate,
				     uint32_t *parent_rate)
{
	uint32_t divider;
	int clk_type;

	clk_type = imx93_ccm_get_clock_type(clk);
	if (clk_type < 0) {
		return clk_type;
	}

	switch (clk_type) {
	case IMX93_CCM_TYPE_IP:
		/* an IP clock has the same frequency as a root clock */
		clk = parent;
		parent = parent->parent;

		if (!parent->freq) {
			return -EINVAL;
		}

		if (rate > parent->freq) {
			return -ENOTSUP;
		}

		divider = DIV_ROUND_UP(parent->freq, rate);
		if (divider > IMX93_CCM_DIV_MAX) {
			return -ENOTSUP;
		}

		if ((parent->freq / divider) == clk->freq) {
			return -EALREADY;
		}

		/* this is the theoretical rate the set_clock_rate() function
		 * should be called with when coniguring the root clock
		 */
		*parent_rate = rate;

		return 0;
	case IMX93_CCM_TYPE_ROOT:
		/* PLLs should only be configured through the DTS */
		return -EPERM;
	default:
		/* this should never be reached */
		__ASSERT(false, "unexpected clock type: %u", clock_type);
	}

	return -EINVAL;
}

static struct imx_ccm_clock_api clock_api = {
	.on_off = imx93_ccm_on_off,
	.set_clock_rate = imx93_ccm_set_clock_rate,
	.get_clock = imx93_ccm_get_clock,
	.assign_parent = imx93_ccm_assign_parent,
	.rate_is_valid = imx93_ccm_rate_is_valid,
	.get_parent_rate = imx93_ccm_get_parent_rate,
};

int imx_ccm_init(const struct device *dev)
{
	struct imx_ccm_data *data = dev->data;

	data->api = &clock_api;

	CLOCK_Init((CCM_Type *)data->regmap);

	return 0;
}
