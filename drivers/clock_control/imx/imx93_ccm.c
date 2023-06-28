/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_control/clock_control_mcux_ccm_rev3.h>
#include <fsl_clock.h>
#include <errno.h>

#define IMX93_CCM_MAX_DIV 256
#define IMX93_INVALID_MUX -1

/* taken from imx_ccm_clock_tree.c */
extern struct imx_ccm_clock_config clock_config;

struct imx_ccm_data mcux_ccm_data;

struct imx_ccm_config mcux_ccm_config = {
	.clock_config = &clock_config,

	.regmap_phys = DT_REG_ADDR_BY_IDX(DT_NODELABEL(ccm), 0),
	.regmap_size = DT_REG_SIZE_BY_IDX(DT_NODELABEL(ccm), 0),

	.pll_regmap_phys = DT_REG_ADDR_BY_IDX(DT_NODELABEL(ccm), 1),
	.pll_regmap_size = DT_REG_SIZE_BY_IDX(DT_NODELABEL(ccm), 1),
};

int imx_ccm_init(const struct device *dev)
{
	const struct imx_ccm_config *cfg;
	struct imx_ccm_data *data;

	cfg = dev->config;
	data = dev->data;

	device_map(&data->regmap, cfg->regmap_phys,
			cfg->regmap_size, K_MEM_CACHE_NONE);
	device_map(&data->pll_regmap, cfg->pll_regmap_phys,
			cfg->pll_regmap_size, K_MEM_CACHE_NONE);

	return 0;
}

int imx_ccm_clock_on_off(const struct device *dev, struct imx_ccm_clock *clk, bool on)
{
	struct imx_ccm_data *data = dev->data;

	/* TODO: root clocks are turned on by default. Is it okay to
	 * move forward with this assumption?
	 *
	 *	 IP clocks are not gated by default. Is the uncertain approach really
	 *	 necessary?
	 */

	/* although according to the TRM all of the clocks are supposed to be gated
	 * we won't make any assumptions here.
	 *
	 * as such, in the initial state a clock may be either gated or not. To make
	 * sure that the clocks reach a known state, when in init state a clock_{on\off}
	 * operation will not take the clock's state into account.
	 *
	 * afterwards, a clock will be gated only if currently not gated and so on. This
	 * way, we won't have to perform unnecessary operations.
	 */
	switch (clk->state) {
		case IMX_CCM_CLOCK_STATE_INIT:
			if (on) {
				CLOCK_EnableClockMapped((uint32_t *)data->regmap, clk->id);
				clk->state = IMX_CCM_CLOCK_STATE_UNGATED;
			} else {
				CLOCK_DisableClockMapped((uint32_t *)data->regmap, clk->id);
				clk->state = IMX_CCM_CLOCK_STATE_GATED;
			}
			break;
		case IMX_CCM_CLOCK_STATE_GATED:
			if (on) {
				CLOCK_EnableClockMapped((uint32_t *)data->regmap, clk->id);
				clk->state = IMX_CCM_CLOCK_STATE_UNGATED;
			}
			break;
		case IMX_CCM_CLOCK_STATE_UNGATED:
			if (!on) {
				CLOCK_DisableClockMapped((uint32_t *)data->regmap, clk->id);
				clk->state = IMX_CCM_CLOCK_STATE_GATED;
			}
			break;
	}

	return 0;
}

int imx_ccm_clock_get_rate(const struct device *dev, struct imx_ccm_clock *clk)
{
	struct imx_ccm_data *data;
	uint32_t mux, div;

	data = dev->data;
	mux = CLOCK_GetRootClockMuxMapped((uint32_t *)data->regmap, clk->root.id);
	/* the queried DIV value is increased by 1 by CLOCK_GetRootClockDivMapped */
	div = CLOCK_GetRootClockDivMapped((uint32_t *)data->regmap, clk->root.id);

	/* TODO: add support for PLLs */
	if (clk->root.sources[mux].type == IMX_CCM_TYPE_FIXED)
		return IMX_CCM_FIXED_FREQ(clk->root.sources[mux]) / div;

	return 0;
}

int imx_ccm_clock_set_rate(const struct device *dev, struct imx_ccm_clock *clk, uint32_t rate)
{
	struct imx_ccm_data *data;
	uint32_t clk_state, crt_rate, div, obtained_rate;
	uint32_t error, min_error, min_mux, min_div, min_rate;
	int i;

	data = dev->data;
	/* save clock's state for later usage */
	clk_state = clk->state;
	min_error = UINT32_MAX;
	min_mux = IMX93_INVALID_MUX;

	/* sanity checks */

	/* set_rate will want to keep the clock's state so if
	 * the clock is in uncertain state this can't be done.
	 * As such, don't allow clocks in INIT state to proceed.
	 */
	if (clk_state == IMX_CCM_CLOCK_STATE_INIT)
		return -EINVAL;

	/* can't request a frequency of 0 */
	if (!rate)
		return -EINVAL;

	/* clock already set to requested rate */
	if (imx_ccm_clock_get_rate(dev, clk) == rate)
		return -EALREADY;

	/* go through each clock source to find the best configuration for
	 * the requested rate.
	 */
	/* TODO: this algorithm is very simplistic and needs much more work.
	 * Also, PLLs should be supported. Also, also, changing a root's
	 * frequency may affect other subsystems so this needs to be treated
	 * with extra care.
	 */
	for (i = 0; i < clk->root.source_num; i++) {
		/* can't get requested rate from current clock if it's
		 * higher than the rate limit.
		 */
		if (rate > IMX_CCM_RATE_LIMIT(clk->root.sources[i]))
			continue;

		if (clk->root.sources[i].type == IMX_CCM_TYPE_FIXED) {
			/* get the frequency of the current fixed clock */
			crt_rate = IMX_CCM_FIXED_FREQ(clk->root.sources[i]);

			/* compute the targeted DIV value */
			div = DIV_ROUND_UP(crt_rate, rate);

			if (div > IMX93_CCM_MAX_DIV)
				continue;

			/* compute the rate obtained using targeted DIV value */
			obtained_rate = crt_rate / div;

			/* compute error */
			error = abs(obtained_rate - rate);

			if (error < min_error) {
				min_error = error;
				min_mux = i;
				min_div = div;
				min_rate = obtained_rate;

				/* it's pointless to go any further if we've found a
				 * configuration yielding an error of 0.
				 */
				if (!error)
					break;
			}
		}

	}

	/* could not find a suitable configuration for requested rate */
	if (min_mux == IMX93_INVALID_MUX)
		return -ENOTSUP;

	/* gate the clock */
	imx_ccm_clock_on_off(dev, clk, false);

	CLOCK_SetRootClockMuxMapped((uint32_t *)data->regmap, clk->root.id, min_mux);
	CLOCK_SetRootClockDivMapped((uint32_t *)data->regmap, clk->root.id, min_div);

	/* ungate clock only if it was initially ungated */
	if (clk_state == IMX_CCM_CLOCK_STATE_UNGATED)
		imx_ccm_clock_on_off(dev, clk, true);

	return min_rate;
}
