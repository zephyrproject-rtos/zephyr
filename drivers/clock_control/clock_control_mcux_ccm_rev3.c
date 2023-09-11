/*
 * Copyight 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_control/clock_control_mcux_ccm_rev3.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ccm_rev3);

/* used for driver binding */
#define DT_DRV_COMPAT nxp_imx_ccm_rev3

/* utility macros */
#define IMX_CCM_REGMAP_IF_EXISTS(nodelabel, idx)		\
	COND_CODE_1(DT_NODE_HAS_PROP(nodelabel, reg),		\
		    (DT_REG_ADDR_BY_IDX(nodelabel, idx)),	\
		    (0))
#define IMX_CCM_REGMAP_SIZE_IF_EXISTS(nodelabel, idx)		\
	COND_CODE_1(DT_NODE_HAS_PROP(nodelabel, reg),		\
		    (DT_REG_SIZE_BY_IDX(nodelabel, idx)),	\
		    (0))

#define APPEND_COMA(...) __VA_ARGS__

#define EXTRACT_CLOCK_ARRAY(node_id, prop)\
	APPEND_COMA(DT_FOREACH_PROP_ELEM_SEP(node_id, prop, DT_PROP_BY_IDX, (,)),)

#define IMX_CCM_FOREACH_ASSIGNED_CLOCK(node_id)				\
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, assigned_clocks),		\
		    (EXTRACT_CLOCK_ARRAY(node_id, assigned_clocks)),	\
		    ())							\

#define IMX_CCM_FOREACH_ASSIGNED_PARENT(node_id)				\
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, assigned_clock_parents),		\
		    (EXTRACT_CLOCK_ARRAY(node_id, assigned_clock_parents)),	\
		    ())								\

#define IMX_CCM_FOREACH_ASSIGNED_RATES(node_id)					\
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, assigned_clock_rates),		\
		    (EXTRACT_CLOCK_ARRAY(node_id, assigned_clock_rates)),	\
		    ())								\

#define IMX_CCM_GET_OPTIONAL_CLOCKS(prop) DT_PROP_OR(DT_NODELABEL(ccm), prop, {})

#define SWAP_UINT_VAR(i, j)	\
do {				\
	uint32_t _tmp = i;	\
	i = j;			\
	j = _tmp;		\
} while (0)			\

static int mcux_ccm_clock_init(const struct device *dev);
static int mcux_ccm_clock_assume_on_init(const struct device *dev);
static int mcux_ccm_ungate_clocks(const struct device *dev);

static int mcux_ccm_on_off(const struct device *dev,
			   struct imx_ccm_clock *clk,
			   bool on)
{
	int ret;

	/* no need to gate/ungate a clock which is already gated/ungated */
	if ((on && clk->state == IMX_CCM_CLOCK_STATE_UNGATED) ||
	    (!on && clk->state == IMX_CCM_CLOCK_STATE_GATED)) {
		return 0;
	}

	ret = imx_ccm_on_off(dev, clk, on);
	if (ret < 0) {
		LOG_ERR("failed to gate/ungate clock %s: %d", clk->name, ret);
		return ret;
	}

	switch (clk->state) {
	case IMX_CCM_CLOCK_STATE_GATED:
		clk->state = IMX_CCM_CLOCK_STATE_UNGATED;
		break;
	case IMX_CCM_CLOCK_STATE_UNGATED:
		clk->state = IMX_CCM_CLOCK_STATE_GATED;
		break;
	default:
		/* this should never happen */
		__ASSERT(false, "invalid clock state: %d", clk->state);
	};

	return 0;
}

static int _mcux_ccm_on(const struct device *dev,
			struct imx_ccm_clock *clk)
{
	int ret;

	LOG_DBG("currently ungating clock %s", clk->name);

	if (!clk->parent) {
		goto out_ungate;
	}

	ret = _mcux_ccm_on(dev, clk->parent);
	if (ret < 0) {
		LOG_ERR("failed ungating operation for clock %s", clk->parent->name);
		return ret;
	}

out_ungate:
	return mcux_ccm_on_off(dev, clk, true);
}

static int mcux_ccm_on(const struct device *dev, clock_control_subsys_t sys)
{
	struct imx_ccm_clock *clk;
	int ret;

	ret = imx_ccm_get_clock(dev, POINTER_TO_UINT(sys), &clk);
	if (ret < 0) {
		LOG_ERR("failed to get clock data for 0x%lx: %d",
			POINTER_TO_UINT(sys), ret);
		return ret;
	}

	/* ungate current clock and its parents */
	return _mcux_ccm_on(dev, clk);
}

static int mcux_ccm_off(const struct device *dev, clock_control_subsys_t sys)
{
	struct imx_ccm_clock *clk;
	int ret;

	ret = imx_ccm_get_clock(dev, POINTER_TO_UINT(sys), &clk);
	if (ret < 0) {
		LOG_ERR("failed to get clock data for 0x%lx: %d",
			POINTER_TO_UINT(sys), ret);
		return ret;
	}

	return mcux_ccm_on_off(dev, clk, false);
}

static int mcux_ccm_get_rate(const struct device *dev,
			     clock_control_subsys_t sys, uint32_t *rate)
{
	const struct imx_ccm_config *cfg;
	struct imx_ccm_data *data;
	struct imx_ccm_clock *clk;
	int ret;

	cfg = dev->config;
	data = dev->data;

	ret = imx_ccm_get_clock(dev, POINTER_TO_UINT(sys), &clk);
	if (ret < 0) {
		LOG_ERR("failed to get clock data for 0x%lx: %d",
			POINTER_TO_UINT(sys), ret);
		return ret;
	}

	/* clock not configured yet */
	if (!clk->freq) {
		LOG_ERR("can't get rate of unconfigured clock %s: %d",
			clk->name, ret);
		return -EINVAL;
	}

	*rate = clk->freq;

	return 0;
}

static int _mcux_ccm_set_rate(const struct device *dev,
			      struct imx_ccm_clock *clk,
			      uint32_t rate)
{
	uint32_t parent_rate;
	int ret, obtained_rate, clk_state;

	clk_state = clk->state;

	LOG_DBG("trying to set rate %u for clock %s", rate, clk->name);

	/* note: although a set_clock_rate() operation may not
	 * yield a frequency equal to the requested rate, this
	 * will help filter out the cases in which it does.
	 *
	 */
	if (clk->freq == rate) {
		LOG_ERR("clock %s already set to rate %u", clk->name, rate);
		return -EALREADY;
	}

	/* can't go any further in the clock tree */
	if (!clk->parent) {
		goto out_set_rate;
	}

	ret = imx_ccm_get_parent_rate(dev, clk, clk->parent, rate, &parent_rate);
	if (ret == -EPERM || ret == -EALREADY) {
		LOG_DBG("early stop in tree traversal for clock %s", clk->name);
		/* we're not allowed to go up the clock hierarchy */
		goto out_set_rate;
	} else if (ret < 0) {
		LOG_ERR("failed to get parent rate for clock %s: %d",
			clk->name, ret);
		return ret;
	}

	/* go up the clock hierarcy in order to set the parent's rate */
	ret = _mcux_ccm_set_rate(dev, clk->parent, parent_rate);
	if (ret < 0 && ret != -EALREADY) {
		return ret;
	}

out_set_rate:
	/* forcefully gate the clock */
	ret = mcux_ccm_on_off(dev, clk, false);
	if (ret < 0) {
		LOG_ERR("failed to gate clock %s: %d", clk->name, ret);
		return ret;
	}

	obtained_rate = imx_ccm_set_clock_rate(dev, clk, rate);
	if (obtained_rate < 0) {
		LOG_ERR("failed to set rate %u for clock %s: %d",
			rate, clk->name, ret);
		return obtained_rate;
	}

	if (clk_state == IMX_CCM_CLOCK_STATE_UNGATED) {
		ret = mcux_ccm_on_off(dev, clk, false);
		if (ret < 0) {
			LOG_ERR("failed to ungate clock %s: %d", clk->name, ret);
			return ret;
		}
	}

	LOG_DBG("configured rate %u for clock %s", obtained_rate, clk->name);

	return obtained_rate;
}

static int mcux_ccm_set_rate(const struct device *dev,
			     clock_control_subsys_t sys,
			     clock_control_subsys_rate_t sys_rate)
{
	struct imx_ccm_clock *clk;
	uint32_t clk_id, clk_rate;
	int ret;

	clk_id = POINTER_TO_UINT(sys);
	clk_rate = POINTER_TO_UINT(sys_rate);

	ret = imx_ccm_get_clock(dev, clk_id, &clk);
	if (ret < 0) {
		LOG_ERR("failed to get clock data for 0x%x: %d", clk_id, ret);
		return ret;
	}

	if (!clk_rate) {
		LOG_ERR("clock rate should be != 0");
		return -ENOTSUP;
	}

	/* this validation should only be performed
	 * here as the rates passed to set_clock_rate()
	 * during the tree traversal are guaranteed to
	 * be valid as they originate from get_parent_rate()
	 */
	if (!imx_ccm_rate_is_valid(dev, clk, clk_rate)) {
		LOG_ERR("rate %u is not a valid rate for %s", clk_rate, clk->name);
		return -ENOTSUP;
	}

	/* set requested rate */
	return _mcux_ccm_set_rate(dev, clk, clk_rate);
}

static int mcux_ccm_init(const struct device *dev)
{
	const struct imx_ccm_config *cfg;
	struct imx_ccm_data *data;
	int ret;

	cfg = dev->config;
	data = dev->data;

	if (cfg->regmap_phys) {
		device_map(&data->regmap, cfg->regmap_phys,
			   cfg->regmap_size, K_MEM_CACHE_NONE);
	}

	if (cfg->pll_regmap_phys) {
		device_map(&data->pll_regmap, cfg->pll_regmap_phys,
			   cfg->pll_regmap_size, K_MEM_CACHE_NONE);
	}

	/* perform SoC-specific initalization */
	ret = imx_ccm_init(dev);
	if (ret < 0) {
		return ret;
	}

	/* initialize clocks that are assumed to be on */
	ret = mcux_ccm_clock_assume_on_init(dev);
	if (ret < 0) {
		return ret;
	}

	/* initialize clocks specified through assigned-clock* properties */
	ret = mcux_ccm_clock_init(dev);
	if (ret < 0) {
		return ret;
	}

	/* ungate clocks passed through the clocks-init-on property */
	return mcux_ccm_ungate_clocks(dev);
}

static const struct clock_control_driver_api mcux_ccm_api = {
	.on = mcux_ccm_on,
	.off = mcux_ccm_off,
	.get_rate = mcux_ccm_get_rate,
	.set_rate = mcux_ccm_set_rate,
};

static uint32_t clocks[] = { DT_FOREACH_NODE(IMX_CCM_FOREACH_ASSIGNED_CLOCK) };
static uint32_t parents[] = { DT_FOREACH_NODE(IMX_CCM_FOREACH_ASSIGNED_PARENT) };
static uint32_t rates[] = { DT_FOREACH_NODE(IMX_CCM_FOREACH_ASSIGNED_RATES) };
static uint32_t clocks_on[] = IMX_CCM_GET_OPTIONAL_CLOCKS(clocks_assume_on);
static uint32_t clocks_init_on[] = IMX_CCM_GET_OPTIONAL_CLOCKS(clocks_init_on);

/* if present, the number of clocks, parents and rates should be equal.
 * If not, we should throw a build error letting the user know the module has
 * been misconfigured.
 */
BUILD_ASSERT(ARRAY_SIZE(clocks) == ARRAY_SIZE(rates),
	     "number of clocks needs to match number of rates");
BUILD_ASSERT(!ARRAY_SIZE(parents) || ARRAY_SIZE(clocks) == ARRAY_SIZE(parents),
	     "number of clocks needs to match number of parents");
BUILD_ASSERT(!(ARRAY_SIZE(clocks_on) % 2),
	     "malformed clocks-assume-on property");

static int get_clock_level(const struct device *dev,
			   uint32_t clock_id, uint32_t *clock_level)
{
	struct imx_ccm_clock *clk;
	int ret;
	uint32_t level;

	ret = imx_ccm_get_clock(dev, clock_id, &clk);
	if (ret < 0) {
		return ret;
	}

	level = 0;

	while (clk) {
		clk = clk->parent;
		level++;
	}

	*clock_level = level;

	return 0;
}

static int sort_clocks_by_level(const struct device *dev)
{
	int i, j, ret, clock_num;
	uint32_t level_i, level_j;

	/* note: clocks, rates and parents are globally defined arrays */
	clock_num = ARRAY_SIZE(clocks);

	for (i = 0; i < clock_num; i++) {
		ret = get_clock_level(dev, clocks[i], &level_i);
		if (ret < 0) {
			return ret;
		}
		for (j = i + 1; j < clock_num; j++) {
			ret = get_clock_level(dev, clocks[j], &level_j);
			if (ret < 0) {
				return ret;
			}

			if (level_i > level_j) {
				SWAP_UINT_VAR(clocks[i], clocks[j]);
				SWAP_UINT_VAR(parents[i], parents[j]);
				SWAP_UINT_VAR(rates[i], rates[j]);
			}
		}
	}

	return 0;
}

static int mcux_ccm_clock_init(const struct device *dev)
{
	const struct imx_ccm_config *cfg;
	int i, ret;
	uint32_t clk_id, parent_id, rate;
	uint32_t clock_num, parent_num, rate_num;
	struct imx_ccm_clock *clk, *parent;

	cfg = dev->config;
	clock_num = ARRAY_SIZE(clocks);
	parent_num = ARRAY_SIZE(parents);
	rate_num = ARRAY_SIZE(rates);

	/* to make sure there's no dependency issues, clocks should be sorted
	 * based on their levels in the clock tree. Usually, a clock which is
	 * found on a lower level should be initialized before a clock which
	 * is found on a higher level as the higher level clock will most likely
	 * depend in some way on the lower level clock (if they are relatives).
	 *
	 * note: this way of taking care of dependencies is very bad and yields
	 * a time complexity of O(n * n), where n = ARRAY_SIZE(clocks).
	 */
	ret = sort_clocks_by_level(dev);
	if (ret < 0) {
		return ret;
	}

	for (i = 0; i < clock_num; i++) {
		clk_id = clocks[i];
		rate = rates[i];

		ret = imx_ccm_get_clock(dev, clk_id, &clk);
		if (ret < 0) {
			LOG_ERR("failed to get clock data for 0x%x: %d",
				clk_id, ret);
			return ret;
		}

		/* although it's assumed by the driver that all clocks
		 * are initially gated this may not always be true. As
		 * such, make sure that at least the clocks we're working
		 * with are gated before performing critical operations
		 * such as parent assignment.
		 *
		 * It's imporant that we use the raw on_off function as
		 * this allows us to bypass the clock state check that
		 * would otherwise forbid us from gating the clocks.
		 */
		ret = imx_ccm_on_off(dev, clk, false);
		if (ret < 0) {
			LOG_ERR("failed to gate clock %s: %d", clk->name, ret);
			return ret;
		}

		LOG_DBG("gated clock %s", clk->name);

		if (parent_num) {
			parent_id = parents[i];

			ret = imx_ccm_get_clock(dev, parent_id, &parent);
			if (ret < 0) {
				LOG_ERR("failed to get clock data for 0x%x: %d",
					parent_id, ret);
				return ret;
			}

			LOG_DBG("assigned parent %s to clock %s",
				parent->name, clk->name);

			ret = imx_ccm_assign_parent(dev, clk, parent);
			if (ret < 0) {
				LOG_ERR("failed to assign %s as parent to %s: %d",
					parent->name, clk->name, ret);
				return ret;
			}
		}

		ret = mcux_ccm_set_rate(dev, UINT_TO_POINTER(clk_id),
					UINT_TO_POINTER(rate));
		if (ret < 0) {
			LOG_ERR("failed to set rate %u for clock %s: %d",
				rate, clk->name, ret);
			return ret;
		}

		LOG_DBG("set rate %u to clock %s (requested rate was %u)",
			ret, clk->name, rate);
	}

	return 0;
}

static int mcux_ccm_clock_assume_on_init(const struct device *dev)
{
	uint32_t clock_num, rate, clock_id;
	struct imx_ccm_clock *clk;
	int i, ret;

	clock_num = ARRAY_SIZE(clocks_on);

	for (i = 0; i < clock_num; i += 2) {
		clock_id = clocks_on[i];
		rate = clocks_on[i + 1];

		ret = imx_ccm_get_clock(dev, clock_id, &clk);
		if (ret < 0) {
			LOG_ERR("failed to get clock data for 0x%x: %d",
				clock_id, ret);
			return ret;
		}

		LOG_DBG("initializing assumed on clock: %s", clk->name);

		clk->state = IMX_CCM_CLOCK_STATE_UNGATED;
		clk->freq = rate;
	}

	return 0;
}

static int mcux_ccm_ungate_clocks(const struct device *dev)
{
	uint32_t clock_num, clock_id;
	int i, ret;

	clock_num = ARRAY_SIZE(clocks_init_on);

	for (i = 0; i < clock_num; i++) {
		clock_id = clocks_init_on[i];

		ret = mcux_ccm_on(dev, UINT_TO_POINTER(clock_id));
		if (ret < 0) {
			LOG_ERR("failed to ungate clock 0x%x", clock_id);
			return ret;
		}
	}
	return 0;
}

struct imx_ccm_data mcux_ccm_data;

struct imx_ccm_config mcux_ccm_config = {
	.regmap_phys = IMX_CCM_REGMAP_IF_EXISTS(DT_NODELABEL(ccm), 0),
	.pll_regmap_phys = IMX_CCM_REGMAP_IF_EXISTS(DT_NODELABEL(ccm), 1),

	.regmap_size = IMX_CCM_REGMAP_SIZE_IF_EXISTS(DT_NODELABEL(ccm), 0),
	.pll_regmap_size = IMX_CCM_REGMAP_SIZE_IF_EXISTS(DT_NODELABEL(ccm), 1),
};

/* there's only 1 CCM instance per SoC */
DEVICE_DT_INST_DEFINE(0,
		      &mcux_ccm_init,
		      NULL,
		      &mcux_ccm_data, &mcux_ccm_config,
		      PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		      &mcux_ccm_api);
