/*
 * Copyright (c) 2025 Vaisala Oyj
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_dac_emul

#include <zephyr/kernel.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(dac_emul, CONFIG_DAC_LOG_LEVEL);

#define DAC_EMUL_TIMEOUT K_FOREVER

struct dac_emul_channel {
	uint32_t value;
	uint8_t resolution;
};

struct dac_emul_config {
	uint8_t channel_count;
};

struct dac_emul_data {
	struct k_mutex channel_mutex;
	struct dac_emul_channel *channels;
};

static int dac_emul_channel_setup(const struct device *dev,
				  const struct dac_channel_cfg *channel_cfg)
{
	struct dac_emul_data *data = dev->data;
	const struct dac_emul_config *config = dev->config;
	uint8_t channel = channel_cfg->channel_id;
	uint8_t resolution = channel_cfg->resolution;

	if (channel >= config->channel_count) {
		LOG_ERR("Invalid channel %u (max %u)", channel, config->channel_count - 1);
		return -EINVAL;
	}

	if (resolution == 0) {
		LOG_ERR("Resolution cannot be 0");
		return -EINVAL;
	}

	if (resolution > 32) {
		LOG_ERR("Resolution cannot exceed 32 bits");
		return -EINVAL;
	}

	int rc = k_mutex_lock(&data->channel_mutex, DAC_EMUL_TIMEOUT);

	if (rc == 0) {
		struct dac_emul_channel *chan = &data->channels[channel];

		chan->resolution = resolution;
		k_mutex_unlock(&data->channel_mutex);

		LOG_DBG("Channel %u configured: %u-bit resolution", channel, resolution);

		return 0;
	}

	LOG_ERR("Could not acquire channel lock (%d)", rc);
	return (rc == -EAGAIN) ? -EBUSY : rc;
}

static inline int write_value_locked(struct dac_emul_channel *chan, uint8_t channel_id,
				     uint32_t value)
{
	if (chan->resolution == 0) {
		LOG_ERR("Channel %u not configured", channel_id);
		return -ENXIO;
	}

	if (chan->resolution > 32) {
		LOG_ERR("Channel %u has invalid resolution %u", channel_id, chan->resolution);
		return -EINVAL;
	}

	uint32_t max_value = (uint32_t)((1ULL << chan->resolution) - 1);

	if (value > max_value) {
		LOG_ERR("Value is out of range (%u > %u)", value, max_value);
		return -EINVAL;
	}

	chan->value = value;
	LOG_DBG("Channel %u value set to %u", channel_id, value);

	return 0;
}

static int dac_emul_write_value(const struct device *dev, uint8_t channel, uint32_t value)
{
	struct dac_emul_data *data = dev->data;
	const struct dac_emul_config *config = dev->config;

	if (channel >= config->channel_count) {
		LOG_ERR("Invalid channel %u (max %u)", channel, config->channel_count - 1);
		return -EINVAL;
	}

	struct dac_emul_channel *chan = &data->channels[channel];

	int rc = k_mutex_lock(&data->channel_mutex, DAC_EMUL_TIMEOUT);

	if (rc == 0) {
		rc = write_value_locked(chan, channel, value);

		k_mutex_unlock(&data->channel_mutex);
		return rc;
	}

	LOG_ERR("Could not acquire channel lock (%d)", rc);
	return (rc == -EAGAIN) ? -EBUSY : rc;
}

static inline int value_get_locked(const struct dac_emul_channel *chan, uint8_t channel_id,
				   uint32_t *value)
{
	if (chan->resolution == 0) {
		LOG_WRN("Channel %u not configured, returning 0", channel_id);
		*value = 0;
		return -ENXIO;
	}

	*value = chan->value;
	LOG_DBG("Channel %u value read: %u", channel_id, *value);

	return 0;
}

int dac_emul_value_get(const struct device *dev, uint8_t channel, uint32_t *value)
{
	struct dac_emul_data *data = dev->data;
	const struct dac_emul_config *config = dev->config;

	if (channel >= config->channel_count) {
		LOG_ERR("Invalid channel %u (max %u)", channel, config->channel_count - 1);
		return -EINVAL;
	}

	if (value == NULL) {
		LOG_ERR("Null pointer provided");
		return -EINVAL;
	}

	int rc = k_mutex_lock(&data->channel_mutex, DAC_EMUL_TIMEOUT);

	if (rc == 0) {
		const struct dac_emul_channel *chan = &data->channels[channel];

		rc = value_get_locked(chan, channel, value);
		k_mutex_unlock(&data->channel_mutex);

		return rc;
	}

	LOG_ERR("Could not acquire channel lock (%d)", rc);
	return (rc == -EAGAIN) ? -EBUSY : rc;
}

static int dac_emul_init(const struct device *dev)
{
	struct dac_emul_data *data = dev->data;
	const struct dac_emul_config *config = dev->config;

	k_mutex_init(&data->channel_mutex);
	LOG_DBG("DAC emulator %s initialized with %u channels", dev->name, config->channel_count);

	return 0;
}

static DEVICE_API(dac, dac_emul_driver_api) = {
	.channel_setup = dac_emul_channel_setup,
	.write_value = dac_emul_write_value,
};

#define DAC_EMUL_INIT(inst)                                                                        \
	BUILD_ASSERT(DT_INST_PROP(inst, nchannels) > 0,                                            \
		     "DAC emulator must have at least one channel");                               \
	static struct dac_emul_channel dac_emul##inst##_channels[DT_INST_PROP(inst, nchannels)];   \
	static struct dac_emul_data data##inst = {                                                 \
		.channels = dac_emul##inst##_channels,                                             \
	};                                                                                         \
	static const struct dac_emul_config config##inst = {                                       \
		.channel_count = DT_INST_PROP(inst, nchannels),                                    \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, dac_emul_init, NULL, &data##inst, &config##inst, POST_KERNEL,  \
			      CONFIG_DAC_INIT_PRIORITY, &dac_emul_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DAC_EMUL_INIT)
