/*
 * Copyright 2024 NXP
 * Copyright 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_management.h>
#include <zephyr/drivers/clock_management/clock_helpers.h>
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

struct lpc55sxx_pll0_data {
	STANDARD_CLK_SUBSYS_DATA_DEFINE
	struct lpc55sxx_pll0_regs *regs;
};

struct lpc55sxx_pll1_data {
	STANDARD_CLK_SUBSYS_DATA_DEFINE
	struct lpc55sxx_pll1_regs *regs;
};

/**
 * Multiplier to enable performing fixed point math for spread spectrum
 * mode on PLL0. The most significant multiplier value we can represent is
 * (1 / (1 << 25)), so if we shift by that we will be in fixed point
 */
#define SSCG_FIXED_POINT_SHIFT (25U)

/* Helper function to wait for PLL lock */
static void syscon_lpc55sxx_pll_waitlock(const struct clk *clk_hw, uint32_t ctrl,
				  int ndec, bool sscg_mode)
{
	struct lpc55sxx_pll0_data *clk_data = clk_hw->hw_data;
	int input_clk;

	/*
	 * Check input reference frequency to VCO. Lock bit is unreliable if
	 * - FREF is below 100KHz or above 20MHz.
	 * - spread spectrum mode is used
	 */

	/* We don't allow setting BYPASSPREDIV bit, input always uses prediv */
	input_clk = clock_management_clk_rate(GET_CLK_PARENT(clk_hw));
	input_clk /= ndec;

	if (sscg_mode || (input_clk > MHZ(20)) || (input_clk < KHZ(100))) {
		/* Spread spectrum mode or out of range input frequency.
		 * RM suggests waiting at least 6ms in this case.
		 */
		SDK_DelayAtLeastUs(6000, SDK_DEVICE_MAXIMUM_CPU_CLOCK_FREQUENCY);
	} else {
		/* Normal mode, use lock bit*/
		while ((clk_data->regs->STAT & SYSCON_PLL0STAT_LOCK_MASK) == 0) {
			/* Spin */
		}
	}
}

static int syscon_lpc55sxx_pll0_onoff(const struct clk *clk_hw, bool on)
{
	if (on) {
		/* Power up PLL */
		PMC->PDRUNCFGCLR0 = (PMC_PDRUNCFG0_PDEN_PLL0_SSCG_MASK |
				     PMC_PDRUNCFG0_PDEN_PLL0_MASK);
	} else {
		/* Power down PLL */
		PMC->PDRUNCFGSET0 = (PMC_PDRUNCFG0_PDEN_PLL0_SSCG_MASK |
				     PMC_PDRUNCFG0_PDEN_PLL0_MASK);
	}

	return 0;
}

static int syscon_lpc55sxx_pll0_configure(const struct clk *clk_hw, const void *data)
{
	struct lpc55sxx_pll0_data *clk_data = clk_hw->hw_data;
	const struct lpc55sxx_pll0_cfg *input = data;
	uint32_t ctrl, ndec;

	/* Power off PLL during setup changes */
	syscon_lpc55sxx_pll0_onoff(clk_hw, false);

	ndec = input->NDEC;
	ctrl = input->CTRL;

	clk_data->regs->CTRL = ctrl;
	/* Request NDEC change */
	clk_data->regs->NDEC = ndec | SYSCON_PLL0NDEC_NREQ_MASK;
	/* Setup SSCG parameters */
	clk_data->regs->SSCG0 = input->SSCG0;
	clk_data->regs->SSCG1 = input->SSCG1;
	/* Request MD change */
	clk_data->regs->SSCG1 = input->SSCG1 |
		(SYSCON_PLL0SSCG1_MD_REQ_MASK | SYSCON_PLL0SSCG1_MREQ_MASK);

	/* Power PLL on */
	syscon_lpc55sxx_pll0_onoff(clk_hw, true);

	syscon_lpc55sxx_pll_waitlock(clk_hw, ctrl, ndec,
		clk_data->regs->SSCG1 & SYSCON_PLL0SSCG1_SEL_EXT_MASK);
	return 0;
}

/* Recalc helper for PLL0 */
static clock_freq_t syscon_lpc55sxx_pll0_recalc_internal(const struct clk *clk_hw,
						const struct lpc55sxx_pll0_cfg *input,
						clock_freq_t  parent_rate)
{
	uint64_t prediv, multiplier, fout, fin = parent_rate;

	prediv = input->NDEC;
	if (input->SSCG1 & SYSCON_PLL0SSCG1_SEL_EXT_MASK) {
		/*
		 * Non-SSCG mode. PLL output frequency is
		 * Fout = MDEC / NDEC * Fin.
		 */
		/* Read MDEC from SSCG0 */
		multiplier = (input->SSCG1 & SYSCON_PLL0SSCG1_MDIV_EXT_MASK) >>
			      SYSCON_PLL0SSCG1_MDIV_EXT_SHIFT;
		fin /= prediv;
		fout = (multiplier * fin);
	} else {
		/*
		 * Using spread spectrum mode. Frequency is calculated as:
		 * Fout = (md[32:25] + (md[24:0] * 2 ** (-25))) * Fin / NDEC,
		 * where md[32] is stored in SSCG1 reg and md[31:0] == SSCG0.
		 * We use fixed point math to perform the calculation.
		 */
		fin /= prediv;
		/* Set upper bit of md */
		multiplier = ((uint64_t)(input->SSCG1 & SYSCON_PLL0SSCG1_MD_MBS_MASK)) << 32;
		/* Set lower 32 bits of md from SSCG0 */
		multiplier |= (input->SSCG0 & SYSCON_PLL0SSCG0_MD_LBS_MASK);
		/* Calculate output frequency */
		fout = (multiplier * fin) >> SSCG_FIXED_POINT_SHIFT;
	}
	return fout;
}

static clock_freq_t syscon_lpc55sxx_pll0_recalc_rate(const struct clk *clk_hw,
					   clock_freq_t  parent_rate)
{
	struct lpc55sxx_pll0_data *clk_data = clk_hw->hw_data;
	struct lpc55sxx_pll0_regs *regs = clk_data->regs;
	struct lpc55sxx_pll0_cfg input;

	input.CTRL = regs->CTRL;
	input.NDEC = (regs->NDEC & SYSCON_PLL0NDEC_NDIV_MASK);
	input.SSCG0 = regs->SSCG0;
	input.SSCG1 = regs->SSCG1;

	return syscon_lpc55sxx_pll0_recalc_internal(clk_hw, &input, parent_rate);
}

#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME)

static clock_freq_t syscon_lpc55sxx_pll0_configure_recalc(const struct clk *clk_hw,
						const void *data,
						clock_freq_t  parent_rate)
{
	int ret;
	const struct lpc55sxx_pll0_cfg *input = data;

	/* First, make sure that children can gate since PLL will power off
	 * to reconfigure.
	 */
	ret = clock_children_check_rate(clk_hw, 0);
	/*
	 * If SAFEGATE is returned, a "safe mux" in the tree is just indicating
	 * it can't switch to a gated clock source. We can ignore this
	 * because we will be powering on the PLL directly after powering
	 * it off.
	 */
	if ((ret < 0) && (ret != NXP_SYSCON_MUX_ERR_SAFEGATE)) {
		/* Some clock in the tree can't gate */
		return ret;
	}

	return syscon_lpc55sxx_pll0_recalc_internal(clk_hw, input, parent_rate);
}

#endif

#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)

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

static clock_freq_t syscon_lpc55sxx_pll0_round_rate(const struct clk *clk_hw,
						    clock_freq_t rate_req,
						    clock_freq_t parent_rate)
{
	int ret;
	uint64_t ndec, md, pre_mult;
	uint64_t rate = MIN(MHZ(550), rate_req);
	uint64_t fout;
	clock_freq_t output_rate;

	/* Check if we will be able to gate the PLL for reconfiguration,
	 * by notifying children will are going to change rate
	 */
	ret = clock_children_check_rate(clk_hw, 0);
	if ((ret < 0) && (ret != NXP_SYSCON_MUX_ERR_SAFEGATE)) {
		return ret;
	}

	/* PLL only supports outputs between 275-550 MHZ */
	if (rate_req < MHZ(275)) {
		return MHZ(275);
	} else if (rate_req > MHZ(550)) {
		return MHZ(550);
	}

	/*
	 * PLL0 supports fractional rate setting via the spread
	 * spectrum generator, so we can use this to achieve the
	 * requested rate.
	 * md[32:0] is used to set fractional multiplier, like so:
	 * mult = md[32:25] + (md[24:0] * 2 ** (-25))
	 * Fout = mult * Fin / NDEC
	 */

	/* Input clock for PLL must be between 3 and 5 MHz, per RM. */
	ndec = parent_rate / MHZ(4);
	/* Calculate input clock before multiplier */
	pre_mult = parent_rate / ndec;
	/* Use fixed point division to calculate md */
	rate <<= SSCG_FIXED_POINT_SHIFT;
	md = rate / pre_mult;

	/* Calculate actual output rate */
	fout = md * pre_mult;
	output_rate = (fout >> SSCG_FIXED_POINT_SHIFT);

	/* Fixed point division will round down. If this has occurred,
	 * return the exact frequency the framework requested, since the PLL
	 * will always have some fractional component to its frequency and
	 * fixed clock states expect an exact match
	 */
	if (rate_req - 1 == output_rate) {
		output_rate = rate_req;
	}

	return output_rate;
}

static clock_freq_t syscon_lpc55sxx_pll0_set_rate(const struct clk *clk_hw,
					clock_freq_t rate_req,
					clock_freq_t parent_rate)
{
	int ret;
	uint64_t ndec, md, pre_mult;
	uint64_t fout, rate = rate_req;
	uint32_t ctrl, seli, selp;
	struct lpc55sxx_pll0_data *clk_data = clk_hw->hw_data;
	clock_freq_t output_rate;

	/*
	 * Check if we will be able to gate the PLL for reconfiguration,
	 * by notifying children will are going to change rate
	 */
	ret = clock_children_check_rate(clk_hw, 0);
	if ((ret < 0) && (ret != NXP_SYSCON_MUX_ERR_SAFEGATE)) {
		return ret;
	}

	/*
	 * PLL only supports outputs between 275-550 MHZ Per RM.
	 * Restrict to 1 Hz away from maximum in either direction,
	 * because PLL fails to lock when md is set to produce exactly
	 * 275 MHz
	 */
	if (rate <= MHZ(275)) {
		rate = MHZ(276);
	} else if (rate >= MHZ(550)) {
		rate = MHZ(549);
	}

	/*
	 * PLL0 supports fractional rate setting via the spread
	 * spectrum generator, so we can use this to achieve the
	 * requested rate.
	 * md[32:0] is used to set fractional multiplier, like so:
	 * mult = md[32:25] + (md[24:0] * 2 ** (-25))
	 * Fout = mult * Fin / NDEC
	 */

	/* Input clock for PLL must be between 3 and 5 MHz, per RM. */
	ndec = parent_rate / MHZ(4);
	/* Calculate input clock before multiplier */
	pre_mult = parent_rate / ndec;
	/* Use fixed point division to calculate md */
	rate <<= SSCG_FIXED_POINT_SHIFT;
	md = rate / pre_mult;

	/* Calculate actual output rate */
	fout = md * pre_mult;

	/* Power off PLL during setup changes */
	syscon_lpc55sxx_pll0_onoff(clk_hw, false);

	/* Set prediv and MD values */
	syscon_lpc55sxx_pll_calc_selx(md >> SSCG_FIXED_POINT_SHIFT, &selp, &seli);
	ctrl = SYSCON_PLL0CTRL_LIMUPOFF_MASK | SYSCON_PLL0CTRL_CLKEN_MASK |
		SYSCON_PLL0CTRL_SELI(seli) | SYSCON_PLL0CTRL_SELP(selp);
	clk_data->regs->CTRL = ctrl;
	/* Request ndec change */
	clk_data->regs->NDEC = ndec | SYSCON_PLL0NDEC_NREQ_MASK;
	clk_data->regs->SSCG0 = SYSCON_PLL0SSCG0_MD_LBS(md);
	/* Request MD change */
	clk_data->regs->SSCG1 = SYSCON_PLL0SSCG1_MD_MBS(md >> 32) |
			(SYSCON_PLL0SSCG1_MD_REQ_MASK | SYSCON_PLL0SSCG1_MREQ_MASK);

	/* Power on PLL */
	syscon_lpc55sxx_pll0_onoff(clk_hw, true);

	syscon_lpc55sxx_pll_waitlock(clk_hw, ctrl, ndec, true);

	output_rate = fout >> SSCG_FIXED_POINT_SHIFT;

	/* Fixed point division will round down. If this has occurred,
	 * return the exact frequency the framework requested, since the PLL
	 * will always have some fractional component to its frequency and
	 * fixed clock states expect an exact match
	 */
	if (rate_req - 1 == output_rate) {
		output_rate = rate_req;
	}
	return output_rate;
}

#endif

const struct clock_management_standard_api nxp_syscon_pll0_api = {
	.shared.on_off = syscon_lpc55sxx_pll0_onoff,
	.shared.configure = syscon_lpc55sxx_pll0_configure,
	.recalc_rate = syscon_lpc55sxx_pll0_recalc_rate,
#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME)
	.configure_recalc = syscon_lpc55sxx_pll0_configure_recalc,
#endif
#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)
	.round_rate = syscon_lpc55sxx_pll0_round_rate,
	.set_rate = syscon_lpc55sxx_pll0_set_rate,
#endif
};

/* PLL0 driver */
#define DT_DRV_COMPAT nxp_lpc55sxx_pll0

#define NXP_LPC55SXX_PLL0_DEFINE(inst)                                         \
	const struct lpc55sxx_pll0_data nxp_lpc55sxx_pll0_data_##inst = {      \
		STANDARD_CLK_SUBSYS_DATA_INIT(CLOCK_DT_GET(DT_INST_PARENT(inst))) \
		.regs = ((struct lpc55sxx_pll0_regs *)                         \
			DT_INST_REG_ADDR(inst)),                               \
	};                                                                     \
	                                                                       \
	CLOCK_DT_INST_DEFINE(inst, &nxp_lpc55sxx_pll0_data_##inst,             \
			     &nxp_syscon_pll0_api);


DT_INST_FOREACH_STATUS_OKAY(NXP_LPC55SXX_PLL0_DEFINE)

/* PLL1 driver */
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT nxp_lpc55sxx_pll1

static int syscon_lpc55sxx_pll1_onoff(const struct clk *clk_hw, bool on)
{
	if (on) {
		/* Power up PLL */
		PMC->PDRUNCFGCLR0 = PMC_PDRUNCFG0_PDEN_PLL1_MASK;
	} else {
		/* Power down PLL */
		PMC->PDRUNCFGSET0 = PMC_PDRUNCFG0_PDEN_PLL1_MASK;
	}

	return 0;
}

static int syscon_lpc55sxx_pll1_configure(const struct clk *clk_hw, const void *data)
{
	struct lpc55sxx_pll1_data *clk_data = clk_hw->hw_data;
	const struct lpc55sxx_pll1_cfg *input = data;

	/* Power off PLL during setup changes */
	syscon_lpc55sxx_pll1_onoff(clk_hw, false);

	clk_data->regs->CTRL = input->CTRL;
	/* Request MDEC change */
	clk_data->regs->MDEC = input->MDEC | SYSCON_PLL1MDEC_MREQ_MASK;

	/* Request NDEC change */
	clk_data->regs->NDEC = input->NDEC | SYSCON_PLL1NDEC_NREQ_MASK;

	/* Power PLL on */
	syscon_lpc55sxx_pll1_onoff(clk_hw, true);

	syscon_lpc55sxx_pll_waitlock(clk_hw, input->CTRL, input->NDEC, false);
	return 0;
}

static clock_freq_t syscon_lpc55sxx_pll1_recalc_rate(const struct clk *clk_hw,
						     clock_freq_t parent_rate)
{
	struct lpc55sxx_pll1_data *clk_data = clk_hw->hw_data;
	struct lpc55sxx_pll1_regs *regs = clk_data->regs;
	uint32_t mdec = regs->MDEC & SYSCON_PLL1MDEC_MDIV_MASK;
	uint32_t ndec = regs->NDEC & SYSCON_PLL1NDEC_NDIV_MASK;

	if (ndec == 0) {
		/* PLL isn't configured yet */
		return -ENOTCONN;
	}
	return (parent_rate * mdec) / ndec;
}

#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME)

static clock_freq_t syscon_lpc55sxx_pll1_configure_recalc(const struct clk *clk_hw,
							  const void *data,
							  clock_freq_t parent_rate)
{
	int ret;
	const struct lpc55sxx_pll1_cfg *input = data;

	/* First, make sure that children can gate since PLL will power off
	 * to reconfigure.
	 */
	ret = clock_children_check_rate(clk_hw, 0);
	/*
	 * If SAFEGATE is returned, a "safe mux" in the tree is just indicating
	 * it can't switch to a gated clock source. We can ignore this
	 * because we will be powering on the PLL directly after powering
	 * it off.
	 */
	if ((ret < 0) && (ret != NXP_SYSCON_MUX_ERR_SAFEGATE)) {
		/* Some clock in the tree can't gate */
		return ret;
	}

	return (parent_rate * input->MDEC) / input->NDEC;
}

#endif /* CONFIG_CLOCK_MANAGEMENT_RUNTIME */

#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)
/* PLL1 specific implementations */

static clock_freq_t syscon_lpc55sxx_pll1_round_rate(const struct clk *clk_hw,
						    clock_freq_t rate_req,
						    clock_freq_t parent_rate)
{
	int ret;
	uint32_t best_div, best_mult, best_diff, test_div, test_mult;
	float postdiv_clk;
	clock_freq_t best_out, cand_rate, target_rate;

	/* Check if we will be able to gate the PLL for reconfiguration,
	 * by notifying children will are going to change rate
	 */
	ret = clock_children_check_rate(clk_hw, 0);
	if ((ret < 0) && (ret != NXP_SYSCON_MUX_ERR_SAFEGATE)) {
		return ret;
	}

	/* PLL only supports outputs between 275-550 MHZ */
	if (rate_req < MHZ(275)) {
		return MHZ(275);
	} else if (rate_req > MHZ(550)) {
		return MHZ(550);
	}

	/* In order to get the best output, we will test with each PLL
	 * prediv value. If we can achieve the requested frequency within
	 * 1%, we will return immediately. Otherwise, we will keep
	 * searching to find the best possible output frequency.
	 */
	best_div = best_mult = best_out = 0;
	best_diff = UINT32_MAX;
	target_rate = MIN(MHZ(550), rate_req);
	for (test_div = 1; test_div < SYSCON_PLL1NDEC_NDIV_MASK; test_div++) {
		/* Find the best multiplier value for this div */
		postdiv_clk = ((float)parent_rate)/((float)test_div);
		test_mult = ((float)target_rate)/postdiv_clk;
		cand_rate = postdiv_clk * test_mult;

		if (abs(cand_rate - target_rate) <= (target_rate / 100)) {
			/* 1% or better match found, break */
			best_div = test_div;
			best_mult = test_mult;
			best_out = cand_rate;
			break;
		} else if (abs(cand_rate - target_rate) < best_diff) {
			best_diff = abs(cand_rate - target_rate);
			best_div = test_div;
			best_mult = test_mult;
			best_out = cand_rate;
		}
	}

	/* Return best output rate */
	return best_out;
}

static clock_freq_t syscon_lpc55sxx_pll1_set_rate(const struct clk *clk_hw,
					 clock_freq_t rate_req,
					 clock_freq_t parent_rate)
{
	struct lpc55sxx_pll1_data *clk_data = clk_hw->hw_data;
	int ret;
	uint32_t best_div, best_mult, best_diff, test_div, test_mult;
	uint32_t seli, selp, ctrl;
	float postdiv_clk;
	clock_freq_t target_rate, cand_rate, best_out;

	/* PLL only supports outputs between 275-550 MHZ */
	if (rate_req < MHZ(275)) {
		rate_req = MHZ(275);
	} else if (rate_req > MHZ(550)) {
		rate_req = MHZ(550);
	}

	/* Check if we will be able to gate the PLL for reconfiguration,
	 * by notifying children will are going to change rate
	 */
	ret = clock_children_check_rate(clk_hw, 0);
	if ((ret < 0) && (ret != NXP_SYSCON_MUX_ERR_SAFEGATE)) {
		return ret;
	}

	/* In order to get the best output, we will test with each PLL
	 * prediv value. If we can achieve the requested frequency within
	 * 1%, we will return immediately. Otherwise, we will keep
	 * searching to find the best possible output frequency.
	 */
	best_div = best_mult = best_out = 0;
	best_diff = UINT32_MAX;
	target_rate = MIN(MHZ(550), rate_req);
	for (test_div = 1; test_div < SYSCON_PLL1NDEC_NDIV_MASK; test_div++) {
		/* Find the best multiplier value for this div */
		postdiv_clk = ((float)parent_rate)/((float)test_div);
		test_mult = ((float)target_rate)/postdiv_clk;
		cand_rate = postdiv_clk * test_mult;

		if (abs(cand_rate - target_rate) <= (target_rate / 100)) {
			/* 1% or better match found, break */
			best_div = test_div;
			best_mult = test_mult;
			best_out = cand_rate;
			break;
		} else if (abs(cand_rate - target_rate) < best_diff) {
			best_diff = abs(cand_rate - target_rate);
			best_div = test_div;
			best_mult = test_mult;
			best_out = cand_rate;
		}
	}

	syscon_lpc55sxx_pll_calc_selx(best_mult, &selp, &seli);

	/* Power off PLL during setup changes */
	syscon_lpc55sxx_pll1_onoff(clk_hw, false);

	/* Program PLL settings */
	ctrl = SYSCON_PLL0CTRL_CLKEN_MASK | SYSCON_PLL0CTRL_SELI(seli) |
		SYSCON_PLL0CTRL_SELP(selp);
	clk_data->regs->CTRL = ctrl;
	/* Request NDEC change */
	clk_data->regs->NDEC = best_div;
	clk_data->regs->NDEC = best_div | SYSCON_PLL1NDEC_NREQ_MASK;
	clk_data->regs->MDEC = best_mult;
	/* Request MDEC change */
	clk_data->regs->MDEC = best_mult | SYSCON_PLL1MDEC_MREQ_MASK;
	/* Power PLL on */
	syscon_lpc55sxx_pll1_onoff(clk_hw, true);
	syscon_lpc55sxx_pll_waitlock(clk_hw, ctrl, best_div, false);

	return best_out;
}

#endif

const struct clock_management_standard_api nxp_syscon_pll1_api = {
	.shared.on_off = syscon_lpc55sxx_pll1_onoff,
	.shared.configure = syscon_lpc55sxx_pll1_configure,
	.recalc_rate = syscon_lpc55sxx_pll1_recalc_rate,
#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME)
	.configure_recalc = syscon_lpc55sxx_pll1_configure_recalc,
#endif
#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)
	.round_rate = syscon_lpc55sxx_pll1_round_rate,
	.set_rate = syscon_lpc55sxx_pll1_set_rate,
#endif
};


#define NXP_LPC55SXX_PLL1_DEFINE(inst)                                         \
	const struct lpc55sxx_pll1_data nxp_lpc55sxx_pll1_data_##inst = {      \
		STANDARD_CLK_SUBSYS_DATA_INIT(CLOCK_DT_GET(DT_INST_PARENT(inst))) \
		.regs = ((struct lpc55sxx_pll1_regs *)                         \
			DT_INST_REG_ADDR(inst)),                               \
	};                                                                     \
	                                                                       \
	CLOCK_DT_INST_DEFINE(inst, &nxp_lpc55sxx_pll1_data_##inst,             \
			     &nxp_syscon_pll1_api);


DT_INST_FOREACH_STATUS_OKAY(NXP_LPC55SXX_PLL1_DEFINE)

/* PLL PDEC divider driver */
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT nxp_lpc55sxx_pll_pdec

struct lpc55sxx_pll_pdec_config {
	STANDARD_CLK_SUBSYS_DATA_DEFINE
	volatile uint32_t *reg;
};

static clock_freq_t syscon_lpc55sxx_pll_pdec_recalc_rate(const struct clk *clk_hw,
							clock_freq_t parent_rate)
{
	const struct lpc55sxx_pll_pdec_config *config = clk_hw->hw_data;
	int div_val = (((*config->reg) & SYSCON_PLL0PDEC_PDIV_MASK) * 2);

	if (div_val == 0) {
		return -ENOTCONN;
	}

	return parent_rate / div_val;
}

static int syscon_lpc55sxx_pll_pdec_configure(const struct clk *clk_hw, const void *data)

{
	const struct lpc55sxx_pll_pdec_config *config = clk_hw->hw_data;
	uint32_t div_val = FIELD_PREP(SYSCON_PLL0PDEC_PDIV_MASK, (((uint32_t)data) / 2));

	*config->reg = div_val | SYSCON_PLL0PDEC_PREQ_MASK;

	return 0;
}

#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME)
static clock_freq_t syscon_lpc55sxx_pll_pdec_configure_recalc(const struct clk *clk_hw,
						     const void *data,
						     clock_freq_t parent_rate)
{
	int div_val = (uint32_t)data;

	if (div_val == 0) {
		return -EINVAL;
	}

	return parent_rate / div_val;
}
#endif

#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)
static clock_freq_t syscon_lpc55sxx_pll_pdec_round_rate(const struct clk *clk_hw,
					       clock_freq_t rate_req,
					       clock_freq_t parent_rate)
{
	uint32_t best_div, best_diff, test_div;
	clock_freq_t parent_req = rate_req;
	clock_freq_t output_clk, last_clk, input_clk, target_rate, best_clk;

	/* First attempt to request double the requested freq from the parent
	 * If the parent's frequency plus our divider setting can't satisfy
	 * the request, increase the requested frequency and try again with
	 * a higher divider target
	 */
	target_rate = rate_req;
	best_diff = UINT32_MAX;
	best_div = 0;
	last_clk = 0;
	/* PLL cannot output rate under 275 MHz, so raise requested rate
	 * by factor of 2 until we hit that minimum
	 */
	while (parent_req < MHZ(275)) {
		parent_req = parent_req * 2;
	}
	do {
		/* Request input clock */
		input_clk = clock_management_round_rate(GET_CLK_PARENT(clk_hw), parent_req);
		if (input_clk < 0) {
			return input_clk;
		}

		/* Check rate we can produce with the input clock */
		test_div = (CLAMP(((input_clk + target_rate / 2) / target_rate), 2, 62) & ~BIT(0));
		output_clk = input_clk / test_div;

		if (abs(output_clk - target_rate) <= (target_rate / 100)) {
			/* 1% or better match found, break */
			best_div = test_div;
			best_clk = output_clk;
			break;
		} else if (abs(output_clk - target_rate) < best_diff) {
			best_diff = abs(output_clk - target_rate);
			best_div = test_div;
			best_clk = output_clk;
		}
		if (input_clk == last_clk) {
			/* Parent clock is locked */
			break;
		}

		/* Raise parent request by factor of 2,
		 * as we can only divide by factors of 2.
		 */
		parent_req = parent_req * 2;
		last_clk = input_clk;
	} while ((test_div < 62) && (last_clk < MHZ(550))); /* Max divider possible */

	return best_clk;
}

static clock_freq_t syscon_lpc55sxx_pll_pdec_set_rate(const struct clk *clk_hw,
					     clock_freq_t rate_req,
					     clock_freq_t parent_rate)
{
	const struct lpc55sxx_pll_pdec_config *config = clk_hw->hw_data;
	uint32_t best_div, best_diff, test_div;
	clock_freq_t parent_req = rate_req;
	clock_freq_t output_clk, last_clk, input_clk, target_rate, best_clk;

	/* First attempt to request double the requested freq from the parent
	 * If the parent's frequency plus our divider setting can't satisfy
	 * the request, increase the requested frequency and try again with
	 * a higher divider target
	 */
	target_rate = rate_req;
	best_diff = UINT32_MAX;
	best_div = 0;
	last_clk = 0;
	/* PLL cannot output rate under 275 MHz, so raise requested rate
	 * by factor of 2 until we hit that minimum
	 */
	while (parent_req < MHZ(275)) {
		parent_req = parent_req * 2;
	}
	do {
		/* Request input clock */
		input_clk = clock_management_round_rate(GET_CLK_PARENT(clk_hw), parent_req);
		if (input_clk < 0) {
			return input_clk;
		}

		/* Check rate we can produce with the input clock */
		test_div = (CLAMP(((input_clk + target_rate / 2) / target_rate), 2, 62) & ~BIT(0));
		output_clk = input_clk / test_div;

		if (abs(output_clk - target_rate) <= (target_rate / 100)) {
			/* 1% or better match found, break */
			best_div = test_div;
			best_clk = output_clk;
			break;
		} else if (abs(output_clk - target_rate) < best_diff) {
			best_diff = abs(output_clk - target_rate);
			best_div = test_div;
			best_clk = output_clk;
		}
		if (input_clk == last_clk) {
			/* Parent clock is locked */
			break;
		}

		/* Raise parent request by factor of 2,
		 * as we can only divide by factors of 2.
		 */
		parent_req = parent_req * 2;
		last_clk = input_clk;
	} while ((test_div < 62) && (last_clk < MHZ(550))); /* Max divider possible */

	/* Set rate for parent */
	input_clk = clock_management_set_rate(GET_CLK_PARENT(clk_hw), parent_req);
	if (input_clk < 0) {
		return input_clk;
	}

	*config->reg = (best_div / 2) | SYSCON_PLL0PDEC_PREQ_MASK;

	return best_clk;
}
#endif


const struct clock_management_standard_api nxp_syscon_pdec_api = {
	.shared.configure = syscon_lpc55sxx_pll_pdec_configure,
	.recalc_rate = syscon_lpc55sxx_pll_pdec_recalc_rate,
#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME)
	.configure_recalc = syscon_lpc55sxx_pll_pdec_configure_recalc,
#endif
#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)
	.round_rate = syscon_lpc55sxx_pll_pdec_round_rate,
	.set_rate = syscon_lpc55sxx_pll_pdec_set_rate,
#endif
};

#define NXP_LPC55SXX_PDEC_DEFINE(inst)                                         \
	const struct lpc55sxx_pll_pdec_config lpc55sxx_pdec_cfg_##inst = {     \
		STANDARD_CLK_SUBSYS_DATA_INIT(CLOCK_DT_GET(DT_INST_PARENT(inst))) \
		.reg = (volatile uint32_t *)DT_INST_REG_ADDR(inst),            \
	};                                                                     \
	                                                                       \
	CLOCK_DT_INST_DEFINE(inst, &lpc55sxx_pdec_cfg_##inst,                  \
			     &nxp_syscon_pdec_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_LPC55SXX_PDEC_DEFINE)
