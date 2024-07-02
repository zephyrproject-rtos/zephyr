/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_mgmt.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <soc.h>

#include "nxp_syscon_internal.h"

/* Registers common to both PLLs */
struct lpc55sxx_pllx_regs {
	volatile uint32_t CTRL;
	volatile uint32_t STAT;
	volatile uint32_t NDEC;
};

struct lpc55sxx_pll0_regs {
	volatile uint32_t CTRL;
	volatile uint32_t STAT;
	volatile uint32_t NDEC;
	volatile uint32_t PDEC;
	volatile uint32_t SSCG0;
	volatile uint32_t SSCG1;
};

struct lpc55sxx_pll1_regs {
	volatile uint32_t CTRL;
	volatile uint32_t STAT;
	volatile uint32_t NDEC;
	volatile uint32_t MDEC;
	volatile uint32_t PDEC;
};

union lpc55sxx_pll_regs {
	struct lpc55sxx_pllx_regs *common;
	struct lpc55sxx_pll0_regs *pll0;
	struct lpc55sxx_pll1_regs *pll1;
};

struct lpc55sxx_pll_data {
	uint32_t output_freq;
	const struct clk *parent;
	const union lpc55sxx_pll_regs regs;
	uint8_t idx;
};

/* Helper function to wait for PLL lock */
static void syscon_lpc55sxx_pll_waitlock(const struct clk *clk_hw, uint32_t ctrl,
				  uint32_t ndec)
{
	struct lpc55sxx_pll_data *clk_data = clk_hw->hw_data;
	int input_clk;

	/*
	 * Check input reference frequency to VCO. Lock bit is unreliable if
	 * - FREF is below 100KHz or above 20MHz.
	 * - spread spectrum mode is used
	 */

	/* We don't allow setting BYPASSPREDIV bit, input always uses prediv */
	input_clk = clock_get_rate(clk_data->parent) / ndec;

	if (((clk_data->idx == 0) &&
	    (clk_data->regs.pll0->SSCG0 & SYSCON_PLL0SSCG1_SEL_EXT_MASK)) ||
	    ((input_clk < MHZ(20)) && (input_clk > KHZ(100)))) {
		/* Normal mode, use lock bit*/
		while ((clk_data->regs.common->STAT & SYSCON_PLL0STAT_LOCK_MASK) == 0) {
			/* Spin */
		}
	} else {
		/* Spread spectrum mode/out of range input frequency.
		 * RM suggests waiting at least 6ms in this case.
		 */
		SDK_DelayAtLeastUs(6000, SDK_DEVICE_MAXIMUM_CPU_CLOCK_FREQUENCY);
	}
}

static int syscon_lpc55sxx_pll_get_rate(const struct clk *clk_hw)
{
	struct lpc55sxx_pll_data *data = clk_hw->hw_data;

	/* Return stored frequency */
	return data->output_freq;
}

static int syscon_lpc55sxx_pll_configure(const struct clk *clk_hw, const void *data)
{
	struct lpc55sxx_pll_data *clk_data = clk_hw->hw_data;
	const struct lpc55sxx_pll_config_input *input = data;
	uint32_t ctrl, ndec;
	int ret;

	/* Copy configured frequency and PLL settings */
	clk_data->output_freq = input->output_freq;
	/* Notify children clock is about to gate */
	ret = clock_notify_children(clk_hw, 0);
	if (ret == NXP_SYSCON_MUX_ERR_SAFEGATE) {
		if (clk_data->output_freq == 0) {
			/* Safe mux is using this source, so we cannot
			 * gate the PLL safely. Note that if the
			 * output frequency is nonzero, we can safely gate
			 * and then reenable the PLL.
			 */
			return -ENOTSUP;
		}
	} else if (ret < 0) {
		return ret;
	}

	/* Power off PLL during setup changes */
	if (clk_data->idx == 0) {
		PMC->PDRUNCFGSET0 = PMC_PDRUNCFG0_PDEN_PLL0_SSCG_MASK;
		PMC->PDRUNCFGSET0 = PMC_PDRUNCFG0_PDEN_PLL0_MASK;
	} else {
		PMC->PDRUNCFGSET0 = PMC_PDRUNCFG0_PDEN_PLL1_MASK;
	}

	if (clk_data->output_freq == 0) {
		/* Keep PLL powered off, return here */
		return 0;
	}

	/* Notify children of new clock frequency we will set */
	ret = clock_notify_children(clk_hw, input->output_freq);
	if (ret < 0) {
		return ret;
	}

	ctrl = input->cfg.common->CTRL;
	ndec = input->cfg.common->NDEC;

	clk_data->regs.common->CTRL = ctrl;
	clk_data->regs.common->NDEC = ndec;
	/* Request NDEC change */
	clk_data->regs.common->NDEC = ndec | SYSCON_PLL0NDEC_NREQ_MASK;
	if (clk_data->idx == 0) {
		/* Setup SSCG parameters */
		clk_data->regs.pll0->SSCG0 = input->cfg.pll0->SSCG0;
		clk_data->regs.pll0->SSCG1 = input->cfg.pll0->SSCG1;
		/* Request MD change */
		clk_data->regs.pll0->SSCG1 = input->cfg.pll0->SSCG1 |
			(SYSCON_PLL0SSCG1_MD_REQ_MASK | SYSCON_PLL0SSCG1_MREQ_MASK);
	} else {
		clk_data->regs.pll1->MDEC = input->cfg.pll1->MDEC;
		/* Request MDEC change */
		clk_data->regs.pll1->MDEC = input->cfg.pll1->MDEC |
					SYSCON_PLL1MDEC_MREQ_MASK;
	}

	/* Power PLL on */
	if (clk_data->idx == 0) {
		PMC->PDRUNCFGCLR0 = PMC_PDRUNCFG0_PDEN_PLL0_SSCG_MASK;
		PMC->PDRUNCFGCLR0 = PMC_PDRUNCFG0_PDEN_PLL0_MASK;
	} else {
		PMC->PDRUNCFGCLR0 = PMC_PDRUNCFG0_PDEN_PLL1_MASK;
	}

	syscon_lpc55sxx_pll_waitlock(clk_hw, ctrl, ndec);
	return 0;
}

#if defined(CONFIG_CLOCK_MGMT_NOTIFY)

static int syscon_lpc55sxx_pll_notify(const struct clk *clk_hw, const struct clk *parent,
				uint32_t parent_rate)
{
	struct lpc55sxx_pll_data *clk_data = clk_hw->hw_data;
	int ret;
	/*
	 * Reuse current output rate. This
	 * may not be correct if the parent clock has been reconfigured,
	 * but we are able to avoid runtime rate calculations via this
	 * method
	 */
	ret = clock_notify_children(clk_hw, clk_data->output_freq);
	if (ret == CLK_NO_CHILDREN) {
		/* We can power down the PLL */
		if (clk_data->idx == 0) {
			PMC->PDRUNCFGSET0 = PMC_PDRUNCFG0_PDEN_PLL0_SSCG_MASK;
			PMC->PDRUNCFGSET0 = PMC_PDRUNCFG0_PDEN_PLL0_MASK;
		} else {
			PMC->PDRUNCFGSET0 = PMC_PDRUNCFG0_PDEN_PLL1_MASK;
		}
	}
	return 0;
}

#endif

#if defined(CONFIG_CLOCK_MGMT_SET_RATE)

/* Helper function to calculate SELP and SELI values */
static void syscon_lpc55sxx_pll_calc_selx(uint32_t mdiv, uint32_t *selp,
				   uint32_t *seli)
{
	*selp = MIN(((mdiv / 4) + 1), 31);
	if (mdiv >= 8000) {
		*seli = 1;
	} else if (mdiv >= 122) {
		*seli = 8000/mdiv;
	} else {
		*seli = (2 * (mdiv / 4)) + 3;
	}
	*seli = MIN(*seli, 63);
}

static int syscon_lpc55sxx_pll0_round_rate(const struct clk *clk_hw, uint32_t rate)
{
	struct lpc55sxx_pll_data *clk_data = clk_hw->hw_data;
	int ret;
	uint32_t mdiv_int, mdiv_frac;
	float mdiv, prediv_clk;

	/* Check if we will be able to gate the PLL for reconfiguration,
	 * by notifying children will are going to change rate
	 */
	ret = clock_notify_children(clk_hw, 0);
	if ((ret < 0) && (ret != NXP_SYSCON_MUX_ERR_SAFEGATE)) {
		/* Return current frequency */
		return clk_data->output_freq;
	}

	/* PLL only supports outputs between 275-550 MHZ */
	if (rate < MHZ(275)) {
		return MHZ(275);
	} else if (rate > MHZ(550)) {
		return MHZ(550);
	}

	/* PLL0 supports fractional rate setting via the spread
	 * spectrum generator, so we can use this to achieve the
	 * requested rate.
	 * MD[32:0] is used to set fractional multiplier, like so:
	 * mult = MD[32:25] + (MD[24:0] * 2 ** (-25))
	 *
	 * Input clock for PLL must be between 3 and 5 MHz per RM.
	 * Request input clock of 16 MHz, we can divide this to 4 MHz.
	 */
	ret = clock_round_rate(clk_data->parent, MHZ(16), clk_hw);
	if (ret <= 0) {
		return ret;
	}
	/* Calculate actual clock after prediv */
	prediv_clk = ((float)ret) / ((float)(ret / MHZ(4)));
	/* Desired multiplier value */
	mdiv = ((float)rate) / prediv_clk;
	/* MD integer portion */
	mdiv_int = (uint32_t)mdiv;
	/* MD factional portion */
	mdiv_frac = (uint32_t)((mdiv - mdiv_int) * ((float)(1 << 25)));
	/* Calculate actual output rate */
	return prediv_clk * mdiv_int +
		(prediv_clk * (((float)mdiv_frac) / ((float)(1 << 25))));
}

static int syscon_lpc55sxx_pll0_set_rate(const struct clk *clk_hw, uint32_t rate)
{
	struct lpc55sxx_pll_data *clk_data = clk_hw->hw_data;
	int input_clk, output_clk, ret;
	uint32_t mdiv_int, mdiv_frac, prediv_val, seli, selp, ctrl;
	float mdiv, prediv_clk;

	/* PLL only supports outputs between 275-550 MHZ */
	if (rate < MHZ(275)) {
		return -ENOTSUP;
	} else if (rate > MHZ(550)) {
		return -ENOTSUP;
	}

	if (rate == clk_data->output_freq) {
		/* Return current frequency */
		return clk_data->output_freq;
	}

	/* PLL0 supports fractional rate setting via the spread
	 * spectrum generator, so we can use this to achieve the
	 * requested rate.
	 * MD[32:0] is used to set fractional multiplier, like so:
	 * mult = MD[32:25] + (MD[24:0] * 2 ** (-25))
	 *
	 * Input clock for PLL must be between 3 and 5 MHz per RM.
	 * Request input clock of 16 MHz, we can divide this to 4 MHz.
	 */
	input_clk = clock_set_rate(clk_data->parent, MHZ(16), clk_hw);
	if (input_clk <= 0) {
		return input_clk;
	}
	/* Calculate prediv value */
	prediv_val = (input_clk / MHZ(4));
	/* Calculate actual clock after prediv */
	prediv_clk = ((float)input_clk) / ((float)prediv_val);
	/* Desired multiplier value */
	mdiv = ((float)rate) / prediv_clk;
	/* MD integer portion */
	mdiv_int = (uint32_t)mdiv;
	/* MD factional portion */
	mdiv_frac = (uint32_t)((mdiv - mdiv_int) * ((float)(1 << 25)));
	/* Calculate actual output rate */
	output_clk = prediv_clk * mdiv_int +
		(prediv_clk * (((float)mdiv_frac) / ((float)(1 << 25))));
	/* Notify children clock is about to gate */
	ret = clock_notify_children(clk_hw, 0);
	if (ret == NXP_SYSCON_MUX_ERR_SAFEGATE) {
		if (output_clk == 0) {
			/* Safe mux is using this source, so we cannot
			 * gate the PLL safely. Note that if the
			 * output frequency is nonzero, we can safely gate
			 * and then reenable the PLL.
			 */
			return -ENOTSUP;
		}
	} else if (ret < 0) {
		return ret;
	}
	/* Power off PLL before setup changes */
	PMC->PDRUNCFGSET0 = PMC_PDRUNCFG0_PDEN_PLL0_SSCG_MASK;
	PMC->PDRUNCFGSET0 = PMC_PDRUNCFG0_PDEN_PLL0_MASK;
	/* Notify children of new frequency */
	ret = clock_notify_children(clk_hw, output_clk);
	if (ret < 0) {
		return ret;
	}
	/* Set prediv and MD values */
	syscon_lpc55sxx_pll_calc_selx(mdiv_int, &selp, &seli);
	ctrl = SYSCON_PLL0CTRL_LIMUPOFF_MASK | SYSCON_PLL0CTRL_CLKEN_MASK |
		SYSCON_PLL0CTRL_SELI(seli) | SYSCON_PLL0CTRL_SELP(selp);
	clk_data->regs.common->CTRL = ctrl;
	clk_data->regs.common->NDEC = prediv_val | SYSCON_PLL0NDEC_NREQ_MASK;
	clk_data->regs.pll0->SSCG0 = SYSCON_PLL0SSCG0_MD_LBS((mdiv_int << 25) | mdiv_frac);
	clk_data->regs.pll0->SSCG1 = SYSCON_PLL0SSCG1_MD_MBS(mdiv_int >> 7);
	clk_data->output_freq = output_clk;
	/* Power on PLL */
	PMC->PDRUNCFGCLR0 = PMC_PDRUNCFG0_PDEN_PLL0_SSCG_MASK;
	PMC->PDRUNCFGCLR0 = PMC_PDRUNCFG0_PDEN_PLL0_MASK;
	syscon_lpc55sxx_pll_waitlock(clk_hw, ctrl, prediv_val);
	return output_clk;
}

#endif

const struct clock_driver_api nxp_syscon_pll0_api = {
	.get_rate = syscon_lpc55sxx_pll_get_rate,
	.configure = syscon_lpc55sxx_pll_configure,
#if defined(CONFIG_CLOCK_MGMT_NOTIFY)
	.notify = syscon_lpc55sxx_pll_notify,
#endif
#if defined(CONFIG_CLOCK_MGMT_SET_RATE)
	.round_rate = syscon_lpc55sxx_pll0_round_rate,
	.set_rate = syscon_lpc55sxx_pll0_set_rate,
#endif
};

/* PLL0 driver */
#define DT_DRV_COMPAT nxp_lpc55sxx_pll0

#define NXP_LPC55SXX_PLL0_DEFINE(inst)                                         \
	struct lpc55sxx_pll_data nxp_lpc55sxx_pll0_data_##inst = {             \
		.parent = CLOCK_DT_GET(DT_INST_PARENT(inst)),                  \
		.regs.pll0 = ((struct lpc55sxx_pll0_regs *)                    \
			DT_INST_REG_ADDR(inst)),                               \
		.idx = 0,                                                      \
	};                                                                     \
	                                                                       \
	CLOCK_DT_INST_DEFINE(inst, &nxp_lpc55sxx_pll0_data_##inst,             \
			     &nxp_syscon_pll0_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_LPC55SXX_PLL0_DEFINE)

#if defined(CONFIG_CLOCK_MGMT_SET_RATE)
/* PLL1 specific implementations */

static int syscon_lpc55sxx_pll1_round_rate(const struct clk *clk_hw, uint32_t rate)
{
	struct lpc55sxx_pll_data *clk_data = clk_hw->hw_data;
	int ret, output_rate, target_rate;
	uint32_t best_div, best_mult, best_diff, best_out, test_div, test_mult;
	float postdiv_clk;

	/* Check if we will be able to gate the PLL for reconfiguration,
	 * by notifying children will are going to change rate
	 */
	ret = clock_notify_children(clk_hw, 0);
	if ((ret < 0) && (ret != NXP_SYSCON_MUX_ERR_SAFEGATE)) {
		/* Return current frequency */
		return clk_data->output_freq;
	}

	/* PLL only supports outputs between 275-550 MHZ */
	if (rate < MHZ(275)) {
		return MHZ(275);
	} else if (rate > MHZ(550)) {
		return MHZ(550);
	}

	/* Request the same frequency from the parent. We likely won't get
	 * the requested frequency, but this handles the case where the
	 * requested frequency is low (and the best output is the 32KHZ
	 * oscillator)
	 */
	ret = clock_round_rate(clk_data->parent, rate, clk_hw);
	if (ret <= 0) {
		return ret;
	}
	/* In order to get the best output, we will test with each PLL
	 * prediv value. If we can achieve the requested frequency within
	 * 1%, we will return immediately. Otherwise, we will keep
	 * searching to find the best possible output frequency.
	 */
	best_div = best_mult = best_out = 0;
	best_diff = UINT32_MAX;
	target_rate = rate;
	for (test_div = 1; test_div < SYSCON_PLL0NDEC_NDIV_MASK; test_div++) {
		/* Find the best multiplier value for this div */
		postdiv_clk = ((float)ret)/((float)test_div);
		test_mult = ((float)rate)/postdiv_clk;
		output_rate = postdiv_clk * test_mult;

		if (abs(output_rate - target_rate) <= (rate / 100)) {
			/* 1% or better match found, break */
			best_div = test_div;
			best_mult = test_mult;
			best_out = output_rate;
			break;
		} else if (abs(output_rate - target_rate) < best_diff) {
			best_diff = abs(output_rate - target_rate);
			best_div = test_div;
			best_mult = test_mult;
			best_out = output_rate;
		}
	}

	/* Return best output rate */
	return output_rate;
}

static int syscon_lpc55sxx_pll1_set_rate(const struct clk *clk_hw, uint32_t rate)
{
	struct lpc55sxx_pll_data *clk_data = clk_hw->hw_data;
	int output_rate, ret, target_rate;
	uint32_t best_div, best_mult, best_diff, best_out, test_div, test_mult;
	uint32_t seli, selp, ctrl;
	float postdiv_clk;

	/* PLL only supports outputs between 275-550 MHZ */
	if (rate < MHZ(275)) {
		return -ENOTSUP;
	} else if (rate > MHZ(550)) {
		return -ENOTSUP;
	}

	if (rate == clk_data->output_freq) {
		/* Return current frequency */
		return clk_data->output_freq;
	}

	/* Request the same frequency from the parent. We likely won't get
	 * the requested frequency, but this handles the case where the
	 * requested frequency is low (and the best output is the 32KHZ
	 * oscillator)
	 */
	ret = clock_set_rate(clk_data->parent, rate, clk_hw);
	if (ret <= 0) {
		return ret;
	}
	/* In order to get the best output, we will test with each PLL
	 * prediv value. If we can achieve the requested frequency within
	 * 1%, we will return immediately. Otherwise, we will keep
	 * searching to find the best possible output frequency.
	 */
	best_div = best_mult = best_out = 0;
	best_diff = UINT32_MAX;
	target_rate = rate;
	for (test_div = 1; test_div < SYSCON_PLL0NDEC_NDIV_MASK; test_div++) {
		/* Find the best multiplier value for this div */
		postdiv_clk = ((float)ret)/((float)test_div);
		test_mult = ((float)rate)/postdiv_clk;
		output_rate = postdiv_clk * test_mult;

		if (abs(output_rate - target_rate) <= (rate / 100)) {
			/* 1% or better match found, break */
			best_div = test_div;
			best_mult = test_mult;
			best_out = output_rate;
			break;
		} else if (abs(output_rate - target_rate) < best_diff) {
			best_diff = abs(output_rate - target_rate);
			best_div = test_div;
			best_mult = test_mult;
			best_out = output_rate;
		}
	}

	syscon_lpc55sxx_pll_calc_selx(best_mult, &selp, &seli);
	/* Notify children clock is about to gate */
	ret = clock_notify_children(clk_hw, 0);
	if (ret == NXP_SYSCON_MUX_ERR_SAFEGATE) {
		if (output_rate == 0) {
			/* Safe mux is using this source, so we cannot
			 * gate the PLL safely. Note that if the
			 * output frequency is nonzero, we can safely gate
			 * and then reenable the PLL.
			 */
			return -ENOTSUP;
		}
	} else if (ret < 0) {
		return ret;
	}
	/* Power off PLL during setup changes */
	PMC->PDRUNCFGSET0 = PMC_PDRUNCFG0_PDEN_PLL1_MASK;
	/* Notify children of new rate */
	ret = clock_notify_children(clk_hw, output_rate);
	if (ret < 0) {
		return ret;
	}
	/* Program PLL settings */
	ctrl = SYSCON_PLL0CTRL_CLKEN_MASK | SYSCON_PLL0CTRL_SELI(seli) |
		SYSCON_PLL0CTRL_SELP(selp);
	clk_data->regs.common->CTRL = ctrl;
	/* Request NDEC change */
	clk_data->regs.common->NDEC = best_div;
	clk_data->regs.common->NDEC = best_div | SYSCON_PLL0NDEC_NREQ_MASK;
	clk_data->regs.pll1->MDEC = best_mult;
	/* Request MDEC change */
	clk_data->regs.pll1->MDEC = best_mult | SYSCON_PLL1MDEC_MREQ_MASK;
	clk_data->output_freq = output_rate;
	/* Power PLL on */
	PMC->PDRUNCFGCLR0 = PMC_PDRUNCFG0_PDEN_PLL1_MASK;
	syscon_lpc55sxx_pll_waitlock(clk_hw, ctrl, best_div);

	return output_rate;
}

#endif

const struct clock_driver_api nxp_syscon_pll1_api = {
	.get_rate = syscon_lpc55sxx_pll_get_rate,
	.configure = syscon_lpc55sxx_pll_configure,
#if defined(CONFIG_CLOCK_MGMT_NOTIFY)
	.notify = syscon_lpc55sxx_pll_notify,
#endif
#if defined(CONFIG_CLOCK_MGMT_SET_RATE)
	.round_rate = syscon_lpc55sxx_pll1_round_rate,
	.set_rate = syscon_lpc55sxx_pll1_set_rate,
#endif
};


#define NXP_LPC55SXX_PLL1_DEFINE(inst)                                         \
	struct lpc55sxx_pll_data nxp_lpc55sxx_pll1_data_##inst = {             \
		.parent = CLOCK_DT_GET(DT_INST_PARENT(inst)),                  \
		.regs.pll1 = ((struct lpc55sxx_pll1_regs *)                    \
			DT_INST_REG_ADDR(inst)),                               \
		.idx = 1,                                                      \
	};                                                                     \
	                                                                       \
	CLOCK_DT_INST_DEFINE(inst, &nxp_lpc55sxx_pll1_data_##inst,             \
			     &nxp_syscon_pll1_api);

/* PLL1 driver */
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT nxp_lpc55sxx_pll1

DT_INST_FOREACH_STATUS_OKAY(NXP_LPC55SXX_PLL1_DEFINE)

/* PLL PDEC divider driver */
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT nxp_lpc55sxx_pll_pdec

struct lpc55sxx_pll_pdec_config {
	const struct clk *parent;
	volatile uint32_t *reg;
};

static int syscon_lpc55sxx_pll_pdec_get_rate(const struct clk *clk_hw)
{
	const struct lpc55sxx_pll_pdec_config *config = clk_hw->hw_data;
	int parent_rate = clock_get_rate(config->parent);
	int div_val = (((*config->reg) & SYSCON_PLL0PDEC_PDIV_MASK)) * 2;

	if (parent_rate <= 0) {
		return parent_rate;
	}

	if (div_val == 0) {
		return -EIO;
	}

	return parent_rate / div_val;
}

static int syscon_lpc55sxx_pll_pdec_configure(const struct clk *clk_hw, const void *data)

{
	const struct lpc55sxx_pll_pdec_config *config = clk_hw->hw_data;
	int parent_rate = clock_get_rate(config->parent);
	uint32_t div_val = FIELD_PREP(SYSCON_PLL0PDEC_PDIV_MASK, (((uint32_t)data) / 2));
	int ret;

	ret = clock_notify_children(clk_hw, parent_rate / ((uint32_t)data));
	if (ret < 0) {
		return ret;
	}
	*config->reg = div_val | SYSCON_PLL0PDEC_PREQ_MASK;

	return 0;
}

#if defined(CONFIG_CLOCK_MGMT_NOTIFY)
static int syscon_lpc55sxx_pll_pdec_notify(const struct clk *clk_hw, const struct clk *parent,
				    uint32_t parent_rate)
{
	const struct lpc55sxx_pll_pdec_config *config = clk_hw->hw_data;
	int div_val = (((*config->reg) & SYSCON_PLL0PDEC_PDIV_MASK)) * 2;

	if (div_val == 0) {
		/* PDEC isn't configured yet, don't notify children */
		return -ENOTCONN;
	}

	return clock_notify_children(clk_hw, parent_rate / div_val);
}
#endif

#if defined(CONFIG_CLOCK_MGMT_SET_RATE)
static int syscon_lpc55sxx_pll_pdec_round_rate(const struct clk *clk_hw, uint32_t rate)
{
	const struct lpc55sxx_pll_pdec_config *config = clk_hw->hw_data;
	int input_clk, last_clk, output_clk, parent_req, target_rate;
	uint32_t best_div, best_diff, best_out, test_div;

	/* First attempt to request double the requested freq from the parent
	 * If the parent's frequency plus our divider setting can't satisfy
	 * the request, increase the requested frequency and try again with
	 * a higher divider target
	 */
	parent_req = rate * 2;
	target_rate = rate;
	best_diff = UINT32_MAX;
	best_div = 0;
	best_out = 0;
	last_clk = 0;
	/* PLL cannot output rate under 275 MHz, so raise requested rate
	 * by factor of 2 until we hit that minimum
	 */
	while (parent_req < MHZ(275)) {
		parent_req *= 2;
	}
	do {
		/* Request input clock */
		input_clk = clock_round_rate(config->parent, parent_req, clk_hw);
		if (input_clk == last_clk) {
			/* Parent clock rate is locked */
			return input_clk / 2;
		}
		/* Check rate we can produce with the input clock */
		test_div = (CLAMP((input_clk / rate), 2, 62) & ~BIT(0));
		output_clk = input_clk / test_div;

		if (abs(output_clk - target_rate) <= (rate / 100)) {
			/* 1% or better match found, break */
			best_div = test_div;
			best_out = output_clk;
			break;
		} else if (abs(output_clk - target_rate) < best_diff) {
			best_diff = abs(output_clk - target_rate);
			best_div = test_div;
			best_out = output_clk;
		}

		/* Raise parent request by factor of 2,
		 * as we can only divide by factors of 2.
		 */
		parent_req *= 2;
		last_clk = input_clk;
	} while ((test_div < 62) && (last_clk < MHZ(550))); /* Max divider possible */

	return best_out;
}

static int syscon_lpc55sxx_pll_pdec_set_rate(const struct clk *clk_hw, uint32_t rate)
{
	const struct lpc55sxx_pll_pdec_config *config = clk_hw->hw_data;
	int input_clk, last_clk, output_clk, parent_req, ret, target_rate;
	uint32_t best_div, best_diff, best_out, best_parent, test_div;

	/* First attempt to request double the requested freq from the parent
	 * If the parent's frequency plus our divider setting can't satisfy
	 * the request, increase the requested frequency and try again with
	 * a higher divider target
	 */
	parent_req = rate * 2;
	best_diff = UINT32_MAX;
	best_div = 0;
	best_out = 0;
	last_clk = 0;
	target_rate = rate;
	/* PLL cannot output rate under 275 MHz, so raise requested rate
	 * by factor of 2 until we hit that minimum
	 */
	while (parent_req < MHZ(275)) {
		parent_req *= 2;
	}
	do {
		/* Request input clock */
		input_clk = clock_round_rate(config->parent, parent_req, clk_hw);
		if (input_clk == last_clk) {
			/* Parent clock rate is locked */
			return input_clk / 2;
		}
		/* Check rate we can produce with the input clock */
		test_div = (CLAMP((input_clk / rate), 2, 62) & ~BIT(0));
		output_clk = input_clk / test_div;

		if (abs(output_clk - target_rate) <= (rate / 100)) {
			/* 1% or better match found, break */
			best_div = test_div;
			best_out = output_clk;
			best_parent = input_clk;
			break;
		} else if (abs(output_clk - target_rate) < best_diff) {
			best_diff = abs(output_clk - target_rate);
			best_div = test_div;
			best_out = output_clk;
			best_parent = input_clk;
		}

		/* Raise parent request by factor of 2,
		 * as we can only divide by factors of 2.
		 */
		parent_req *= 2;
		last_clk = input_clk;
	} while ((test_div < 62) && (last_clk < MHZ(550))); /* Max divider possible */

	/* Set rate for parent */
	input_clk = clock_set_rate(config->parent, best_parent, clk_hw);
	if (input_clk <= 0) {
		return input_clk;
	}

	ret = clock_notify_children(clk_hw, best_out);
	if (ret < 0) {
		return ret;
	}
	*config->reg = (best_div / 2) | SYSCON_PLL0PDEC_PREQ_MASK;

	return best_out;
}
#endif


const struct clock_driver_api nxp_syscon_pdec_api = {
	.get_rate = syscon_lpc55sxx_pll_pdec_get_rate,
	.configure = syscon_lpc55sxx_pll_pdec_configure,
#if defined(CONFIG_CLOCK_MGMT_NOTIFY)
	.notify = syscon_lpc55sxx_pll_pdec_notify,
#endif
#if defined(CONFIG_CLOCK_MGMT_SET_RATE)
	.round_rate = syscon_lpc55sxx_pll_pdec_round_rate,
	.set_rate = syscon_lpc55sxx_pll_pdec_set_rate,
#endif
};

#define NXP_LPC55SXX_PDEC_DEFINE(inst)                                         \
	const struct lpc55sxx_pll_pdec_config lpc55sxx_pdec_cfg_##inst = {     \
		.parent = CLOCK_DT_GET(DT_INST_PARENT(inst)),                  \
		.reg = (volatile uint32_t *)DT_INST_REG_ADDR(inst),            \
	};                                                                     \
	                                                                       \
	CLOCK_DT_INST_DEFINE(inst, &lpc55sxx_pdec_cfg_##inst,                  \
			     &nxp_syscon_pdec_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_LPC55SXX_PDEC_DEFINE)
