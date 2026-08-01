/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 TOKITA Hiroshi
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_emul.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(clock_control_emul, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

typedef bool (*clock_control_emul_subsys_match_t)(clock_control_subsys_t sys, const uint32_t *cells,
						  size_t num_cells);
typedef int (*clock_control_emul_rate_to_value_t)(clock_control_subsys_rate_t rate,
						  uint32_t *value);

struct clock_control_emul_config {
	bool loose_check;
	const uint32_t *clock_id_cells;
	size_t num_clocks;
	size_t num_cells;
	clock_control_emul_subsys_match_t subsys_match;
	clock_control_emul_rate_to_value_t rate_to_value;
};

struct clock_control_emul_data {
	uint32_t *rates;
	bool *started;
};

static int clock_control_emul_find_clock_idx(const struct device *dev, clock_control_subsys_t sys,
					     size_t *index)
{
	const struct clock_control_emul_config *config = dev->config;

	for (size_t i = 0; i < config->num_clocks; i++) {
		const uint32_t *cells = &config->clock_id_cells[i * config->num_cells];

		if (config->subsys_match(sys, cells, config->num_cells)) {
			*index = i;
			return 0;
		}
	}

	return -EINVAL;
}

static int clock_control_emul_start(const struct device *dev, clock_control_subsys_t sys)
{
	const struct clock_control_emul_config *config = dev->config;
	struct clock_control_emul_data *data = dev->data;
	size_t index;
	int ret;

	LOG_INF("start %p", dev);

	ret = clock_control_emul_find_clock_idx(dev, sys, &index);

	if (ret != 0) {
		LOG_INF("start: clock-id not found: %d", ret);
		return config->loose_check ? 0 : ret;
	}

	data->started[index] = true;

	return 0;
}

static int clock_control_emul_stop(const struct device *dev, clock_control_subsys_t sys)
{
	const struct clock_control_emul_config *config = dev->config;
	struct clock_control_emul_data *data = dev->data;
	size_t index;
	int ret;

	LOG_INF("stop %p", dev);

	ret = clock_control_emul_find_clock_idx(dev, sys, &index);
	if (ret != 0) {
		LOG_INF("stop: clock-id not found: %d", ret);
		return config->loose_check ? 0 : ret;
	}

	data->started[index] = false;

	return 0;
}

static int clock_control_emul_get_rate(const struct device *dev, clock_control_subsys_t sys,
				       uint32_t *rate)
{
	const struct clock_control_emul_config *config = dev->config;
	struct clock_control_emul_data *data = dev->data;
	size_t index;
	int ret;

	if (rate == NULL) {
		LOG_INF("get_rate: rate is null");
		return config->loose_check ? 0 : -EINVAL;
	}

	ret = clock_control_emul_find_clock_idx(dev, sys, &index);
	if (ret < 0) {
		LOG_INF("get_rate: clock-id not found: %d", ret);
		return config->loose_check ? 0 : ret;
	}

	*rate = data->rates[index];

	return 0;
}

static enum clock_control_status clock_control_emul_get_status(const struct device *dev,
							       clock_control_subsys_t sys)
{
	const struct clock_control_emul_config *config = dev->config;
	struct clock_control_emul_data *data = dev->data;
	size_t index;

	if (clock_control_emul_find_clock_idx(dev, sys, &index) < 0) {
		return config->loose_check ? CLOCK_CONTROL_STATUS_OFF
					   : CLOCK_CONTROL_STATUS_UNKNOWN;
	}

	return data->started[index] ? CLOCK_CONTROL_STATUS_ON : CLOCK_CONTROL_STATUS_OFF;
}

static int clock_control_emul_set_rate(const struct device *dev, clock_control_subsys_t sys,
				       clock_control_subsys_rate_t rate)
{
	const struct clock_control_emul_config *config = dev->config;
	struct clock_control_emul_data *data = dev->data;
	size_t index;
	int ret;

	ret = clock_control_emul_find_clock_idx(dev, sys, &index);
	if (ret < 0) {
		LOG_INF("set_rate: clock-id not found: %d", ret);
		return config->loose_check ? 0 : ret;
	}

	ret = config->rate_to_value(rate, &data->rates[index]);
	if (ret < 0) {
		LOG_INF("set_rate: invalid rate: %d", ret);
		return config->loose_check ? 0 : ret;
	}

	return 0;
}

static int clock_control_emul_configure(const struct device *dev, clock_control_subsys_t sys,
					void *data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(sys);
	ARG_UNUSED(data);

	return 0;
}

static DEVICE_API(clock_control, clock_control_emul_api) = {
	.on = clock_control_emul_start,
	.off = clock_control_emul_stop,
	.get_rate = clock_control_emul_get_rate,
	.get_status = clock_control_emul_get_status,
	.set_rate = clock_control_emul_set_rate,
	.configure = clock_control_emul_configure,
};

#define CLOCK_CONTROL_EMUL_INIT(node_id, cell_count)                                               \
	static const uint32_t clock_control_emul_cells_##node_id[] =                               \
		DT_PROP_OR(node_id, clock_ids, {});                                                \
	uint32_t rates_##node_id[] = DT_PROP_OR(node_id, clock_initial_values, {});                \
	bool started_##node_id[ARRAY_SIZE(rates_##node_id)];                                       \
	static const struct clock_control_emul_config clock_control_emul_config_##node_id = {      \
		.loose_check = DT_PROP(node_id, loose_check),                                      \
		.clock_id_cells = clock_control_emul_cells_##node_id,                              \
		.num_clocks = DT_PROP_LEN_OR(node_id, clock_ids, 0) / cell_count,                  \
		.num_cells = cell_count,                                                           \
		.subsys_match =                                                                    \
			UTIL_CAT(DT_STRING_TOKEN_BY_IDX(node_id, compatible, 0), _subsys_match),   \
		.rate_to_value =                                                                   \
			UTIL_CAT(DT_STRING_TOKEN_BY_IDX(node_id, compatible, 0), _rate_to_value),  \
	};                                                                                         \
	static struct clock_control_emul_data clock_control_emul_data_##node_id = {                \
		.rates = rates_##node_id,                                                          \
		.started = started_##node_id,                                                      \
	};                                                                                         \
	DEVICE_DT_DEFINE(node_id, NULL, NULL, &clock_control_emul_data_##node_id,                  \
			 &clock_control_emul_config_##node_id, PRE_KERNEL_1,                       \
			 CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &clock_control_emul_api);

DT_FOREACH_STATUS_OKAY_VARGS(zephyr_clock_controller_emul_clkid, CLOCK_CONTROL_EMUL_INIT, 1)
DT_FOREACH_STATUS_OKAY_VARGS(zephyr_clock_controller_emul_clk_id, CLOCK_CONTROL_EMUL_INIT, 1)
DT_FOREACH_STATUS_OKAY_VARGS(zephyr_clock_controller_emul_id, CLOCK_CONTROL_EMUL_INIT, 1)
