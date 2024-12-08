/*
 * Copyright (c) 2023 FTP Technologies
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_mcp970x

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/sensor.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mcp970x, CONFIG_SENSOR_LOG_LEVEL);

enum ic_family {
	FAMILY_MCP9700_9700A,
	FAMILY_MCP9701_9701A
};

/* Milli degrees C per degree C */
#define MC_PER_C 1000

#define MV_AT_0C_MCP9700_9700A 500
#define MV_AT_0C_MCP9701_9701A 400

#define T_COEFF_MCP9700_9700A 10
#define T_COEFF_MCP9701_9701A 19.5

struct mcp970x_config {
	struct adc_dt_spec adc;
	enum ic_family family;
};

struct mcp970x_data {
	struct adc_sequence sequence;
	int16_t raw;
};

static int fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct mcp970x_config *config = dev->config;
	struct mcp970x_data *data = dev->data;
	int ret;

	if ((chan != SENSOR_CHAN_AMBIENT_TEMP) && (chan != SENSOR_CHAN_ALL)) {
		return -ENOTSUP;
	}

	ret = adc_read_dt(&config->adc, &data->sequence);
	if (ret != 0) {
		LOG_ERR("adc_read: %d", ret);
	}

	return ret;
}

static int get(const struct device *dev, enum sensor_channel chan, struct sensor_value *val)
{
	const struct mcp970x_config *config = dev->config;
	struct mcp970x_data *data = dev->data;
	int32_t raw_val = data->raw;
	int32_t t;
	int ret;

	__ASSERT_NO_MSG(val != NULL);

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	ret = adc_raw_to_millivolts_dt(&config->adc, &raw_val);
	if (ret != 0) {
		LOG_ERR("to_mv: %d", ret);
		return ret;
	}

	if (config->family == FAMILY_MCP9700_9700A) {
		t = (MC_PER_C * (raw_val - MV_AT_0C_MCP9700_9700A)) / T_COEFF_MCP9700_9700A;
	} else {
		int32_t t_coeff = 10 * T_COEFF_MCP9701_9701A; /* float to int */

		t = (MC_PER_C * 10 * (raw_val - MV_AT_0C_MCP9701_9701A)) / t_coeff;
	}

	val->val1 = t / MC_PER_C;
	val->val2 = 1000 * (t % MC_PER_C);

	LOG_DBG("%d of %d, %dmV, %dmC", data->raw, (1 << data->sequence.resolution) - 1, raw_val,
		t);

	return 0;
}

static DEVICE_API(sensor, mcp970x_api) = {
	.sample_fetch = fetch,
	.channel_get = get,
};

static int init(const struct device *dev)
{
	const struct mcp970x_config *config = dev->config;
	struct mcp970x_data *data = dev->data;
	int ret;

	if (!adc_is_ready_dt(&config->adc)) {
		LOG_ERR("ADC is not ready");
		return -ENODEV;
	}

	ret = adc_channel_setup_dt(&config->adc);
	if (ret != 0) {
		LOG_ERR("setup: %d", ret);
		return ret;
	}

	ret = adc_sequence_init_dt(&config->adc, &data->sequence);
	if (ret != 0) {
		LOG_ERR("sequence: %d", ret);
		return ret;
	}

	data->sequence.buffer = &data->raw;
	data->sequence.buffer_size = sizeof(data->raw);

	return 0;
}

#define MCP970X_INIT(inst)                                                                         \
	static struct mcp970x_data mcp970x_##inst##_data = {0};                                    \
                                                                                                   \
	static const struct mcp970x_config mcp970x_##inst##_config = {                             \
		.adc = ADC_DT_SPEC_INST_GET(inst),                                                 \
		.family = DT_INST_ENUM_IDX(inst, family),                                          \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, &init, NULL, &mcp970x_##inst##_data,                    \
				     &mcp970x_##inst##_config, POST_KERNEL,                        \
				     CONFIG_SENSOR_INIT_PRIORITY, &mcp970x_api);

DT_INST_FOREACH_STATUS_OKAY(MCP970X_INIT)
