/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_management/clock_driver.h>

#define DT_DRV_COMPAT vnd_emul_clock_div

struct emul_clock_div {
	uint8_t div_max;
	uint8_t div_val;
	const struct clk *parent;
};

static int emul_clock_div_get_rate(const struct clk *clk_hw)
{
	struct emul_clock_div *data = clk_hw->hw_data;
	int parent_rate = clock_get_rate(data->parent);

	if (parent_rate <= 0) {
		return parent_rate;
	}

	return (parent_rate / (data->div_val + 1));
}

static int emul_clock_div_configure(const struct clk *clk_hw, const void *div_cfg)
{
	struct emul_clock_div *data = clk_hw->hw_data;
	int ret, parent_rate;
	uint32_t div_val = (uint32_t)div_cfg;

	if ((div_val < 1) || (div_val > (data->div_max + 1))) {
		return -EINVAL;
	}

	parent_rate = clock_get_rate(data->parent);
	if (parent_rate <= 0) {
		return parent_rate;
	}

	ret = clock_children_check_rate(clk_hw, parent_rate / div_val);
	if (ret < 0) {
		return ret;
	}

	ret = clock_children_notify_pre_change(clk_hw,
					       parent_rate / (data->div_val + 1),
					       parent_rate / div_val);
	if (ret < 0) {
		return ret;
	}

	/* Apply div selection */
	data->div_val = div_val - 1;

	return clock_children_notify_post_change(clk_hw,
						 parent_rate / (data->div_val + 1),
						 parent_rate / div_val);
}

#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME)
static int emul_clock_div_notify(const struct clk *clk_hw, const struct clk *parent,
				 const struct clock_management_event *event)
{
	struct emul_clock_div *data = clk_hw->hw_data;
	struct clock_management_event notify_event;

	notify_event.type = event->type;
	notify_event.old_rate = event->old_rate / (data->div_val + 1);
	notify_event.new_rate = event->new_rate / (data->div_val + 1);

	return clock_notify_children(clk_hw, &notify_event);
}
#endif

#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)
static int emul_clock_div_round_rate(const struct clk *clk_hw,
				     uint32_t req_rate)

{
	struct emul_clock_div *data = clk_hw->hw_data;
	int parent_rate = clock_round_rate(data->parent, req_rate);
	int div_val = CLAMP((parent_rate / req_rate), 1, (data->div_max + 1));
	uint32_t output_rate = parent_rate / div_val;
	int ret;

	/* Raise div value until we are in range */
	while (output_rate > req_rate) {
		div_val++;
		output_rate = parent_rate / div_val;
	}

	if (output_rate > req_rate) {
		return -ENOENT;
	}

	ret = clock_children_check_rate(clk_hw, output_rate);
	if (ret < 0) {
		return ret;
	}

	return output_rate;
}

static int emul_clock_div_set_rate(const struct clk *clk_hw,
				   uint32_t req_rate)
{
	struct emul_clock_div *data = clk_hw->hw_data;
	int parent_rate = clock_set_rate(data->parent, req_rate);
	int div_val = CLAMP((parent_rate / req_rate), 1, (data->div_max + 1));
	uint32_t output_rate = parent_rate / div_val;
	uint32_t current_rate = parent_rate / (data->div_val + 1);
	int ret;

	/* Raise div value until we are in range */
	while (output_rate > req_rate) {
		div_val++;
		output_rate = parent_rate / div_val;
	}

	if (output_rate > req_rate) {
		return -ENOENT;
	}

	ret = clock_children_notify_pre_change(clk_hw, current_rate, output_rate);
	if (ret < 0) {
		return ret;
	}

	data->div_val = div_val - 1;

	ret = clock_children_notify_post_change(clk_hw, current_rate, output_rate);
	if (ret < 0) {
		return ret;
	}

	return output_rate;
}
#endif

const struct clock_management_driver_api emul_div_api = {
	.get_rate = emul_clock_div_get_rate,
	.configure = emul_clock_div_configure,
#if defined(CONFIG_CLOCK_MANAGEMENT_RUNTIME)
	.notify = emul_clock_div_notify,
#endif
#if defined(CONFIG_CLOCK_MANAGEMENT_SET_RATE)
	.round_rate = emul_clock_div_round_rate,
	.set_rate = emul_clock_div_set_rate,
#endif
};

#define EMUL_CLOCK_DEFINE(inst)                                                \
	struct emul_clock_div emul_clock_div_##inst = {                        \
		.parent = CLOCK_DT_GET(DT_INST_PARENT(inst)),                  \
		.div_max = DT_INST_PROP(inst, max_div) - 1,                    \
		.div_val = 0,                                                  \
	};                                                                     \
	                                                                       \
	CLOCK_DT_INST_DEFINE(inst,                                             \
			     &emul_clock_div_##inst,                           \
			     &emul_div_api);

DT_INST_FOREACH_STATUS_OKAY(EMUL_CLOCK_DEFINE)
