/*
 * Copyright (c) 2024 Analog Devices Inc.
 * Copyright (c) 2024 Baylibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/dac.h>

#include <zephyr/drivers/mfd/max22017.h>
#include <zephyr/logging/log.h>

#define DT_DRV_COMPAT adi_max22017_dac
LOG_MODULE_REGISTER(dac_max22017, CONFIG_DAC_LOG_LEVEL);

struct dac_adi_max22017_config {
	const struct device *parent;
	uint8_t resolution;
	uint8_t nchannels;
	const struct gpio_dt_spec gpio_ldac;
	const struct gpio_dt_spec gpio_busy;
	uint8_t latch_mode[MAX22017_MAX_CHANNEL];
	uint8_t polarity_mode[MAX22017_MAX_CHANNEL];
	uint8_t dac_mode[MAX22017_MAX_CHANNEL];
	uint8_t ovc_mode[MAX22017_MAX_CHANNEL];
	uint16_t timeout;
};

static int max22017_channel_setup(const struct device *dev,
				  const struct dac_channel_cfg *channel_cfg)
{
	int ret;
	uint16_t ao_cnfg, gen_cnfg;
	uint8_t chan = channel_cfg->channel_id;
	const struct dac_adi_max22017_config *config = dev->config;
	const struct device *parent = config->parent;
	struct max22017_data *data = parent->data;

	if (chan > config->nchannels - 1) {
		LOG_ERR("Unsupported channel %d", chan);
		return -ENOTSUP;
	}

	if (channel_cfg->resolution != config->resolution) {
		LOG_ERR("Unsupported resolution %d", chan);
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = max22017_reg_read(parent, MAX22017_AO_CNFG_OFF, &ao_cnfg);
	if (ret) {
		goto fail;
	}

	ao_cnfg |= FIELD_PREP(MAX22017_AO_CNFG_AO_EN, BIT(chan));

	if (!config->latch_mode[chan]) {
		ao_cnfg |= FIELD_PREP(MAX22017_AO_CNFG_AO_LD_CNFG, BIT(chan));
	}

	if (config->polarity_mode[chan]) {
		ao_cnfg |= FIELD_PREP(MAX22017_AO_CNFG_AO_UNI, BIT(chan));
	}

	if (config->dac_mode[chan]) {
		ao_cnfg |= FIELD_PREP(MAX22017_AO_CNFG_AO_MODE, BIT(chan));
	}

	ret = max22017_reg_write(parent, MAX22017_AO_CNFG_OFF, ao_cnfg);
	if (ret) {
		goto fail;
	}

	ret = max22017_reg_read(parent, MAX22017_GEN_CNFG_OFF, &gen_cnfg);
	if (ret) {
		goto fail;
	}

	if (config->ovc_mode[chan]) {
		gen_cnfg |= FIELD_PREP(MAX22017_GEN_CNFG_OVC_CNFG, BIT(chan));
		/* Over current shutdown mode */
		if (config->ovc_mode[chan] == 2) {
			gen_cnfg |= FIELD_PREP(MAX22017_GEN_CNFG_OVC_SHDN_CNFG, BIT(chan));
		}
	}

	ret = max22017_reg_write(parent, MAX22017_GEN_CNFG_OFF, gen_cnfg);
fail:
	k_mutex_unlock(&data->lock);
	return ret;
}

static int max22017_write_value(const struct device *dev, uint8_t channel, uint32_t value)
{
	int ret;
	uint16_t ao_sta;
	const struct dac_adi_max22017_config *config = dev->config;
	const struct device *parent = config->parent;
	struct max22017_data *data = parent->data;

	if (channel > config->nchannels - 1) {
		LOG_ERR("unsupported channel %d", channel);
		return ENOTSUP;
	}

	if (value >= (1 << config->resolution)) {
		LOG_ERR("Value %d out of range", value);
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	if (config->gpio_busy.port) {
		if (gpio_pin_get_dt(&config->gpio_busy)) {
			ret = -EBUSY;
			goto fail;
		}
	} else {
		ret = max22017_reg_read(parent, MAX22017_AO_STA_OFF, &ao_sta);
		if (ret) {
			goto fail;
		}
		if (FIELD_GET(MAX22017_AO_STA_BUSY_STA, ao_sta)) {
			ret = -EBUSY;
			goto fail;
		}
	}

	ret = max22017_reg_write(parent, MAX22017_AO_DATA_CHn_OFF(channel),
				 FIELD_PREP(MAX22017_AO_DATA_CHn_AO_DATA_CH, value));
	if (ret) {
		goto fail;
	}

	if (config->latch_mode[channel]) {
		if (config->gpio_ldac.port) {
			gpio_pin_set_dt(&config->gpio_ldac, false);
			k_sleep(K_USEC(MAX22017_LDAC_TOGGLE_TIME));
			gpio_pin_set_dt(&config->gpio_ldac, true);
		} else {
			ret = max22017_reg_write(
				parent, MAX22017_AO_CMD_OFF,
				FIELD_PREP(MAX22017_AO_CMD_AO_LD_CTRL, BIT(channel)));
		}
	}
fail:
	k_mutex_unlock(&data->lock);
	return ret;
}

static int max22017_init(const struct device *dev)
{
	int ret;
	uint16_t gen_cnfg = 0, gen_int_en = 0;
	const struct dac_adi_max22017_config *config = dev->config;
	const struct device *parent = config->parent;
	struct max22017_data *data = config->parent->data;

	if (!device_is_ready(config->parent)) {
		LOG_ERR("parent adi_max22017 MFD device '%s' not ready", config->parent->name);
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = max22017_reg_read(parent, MAX22017_GEN_CNFG_OFF, &gen_cnfg);
	if (ret) {
		goto fail;
	}

	ret = max22017_reg_read(parent, MAX22017_GEN_INTEN_OFF, &gen_int_en);
	if (ret) {
		goto fail;
	}

	if (config->timeout) {
		gen_cnfg |= FIELD_PREP(MAX22017_GEN_CNFG_TMOUT_EN, 1) |
			    FIELD_PREP(MAX22017_GEN_CNFG_TMOUT_SEL, (config->timeout / 100) - 1);
		gen_int_en |= FIELD_PREP(MAX22017_GEN_INTEN_TMOUT_INTEN, 1);
	}

	ret = max22017_reg_write(parent, MAX22017_GEN_CNFG_OFF, gen_cnfg);
	if (ret) {
		goto fail;
	}

	ret = max22017_reg_write(parent, MAX22017_GEN_INTEN_OFF, gen_int_en);
	if (ret) {
		goto fail;
	}

	if (config->gpio_ldac.port) {
		ret = gpio_pin_configure_dt(&config->gpio_ldac, GPIO_OUTPUT_ACTIVE);
		if (ret) {
			LOG_ERR("failed to initialize GPIO ldac pin");
			goto fail;
		}
	}

	if (config->gpio_busy.port) {
		ret = gpio_pin_configure_dt(&config->gpio_busy, GPIO_INPUT);
		if (ret) {
			LOG_ERR("failed to initialize GPIO busy pin");
			goto fail;
		}
	}

fail:
	k_mutex_unlock(&data->lock);
	return ret;
}

static DEVICE_API(dac, max22017_driver_api) = {
	.channel_setup = max22017_channel_setup,
	.write_value = max22017_write_value,
};

#define DAC_MAX22017_DEVICE(id)                                                                    \
	static const struct dac_adi_max22017_config dac_adi_max22017_config_##id = {               \
		.parent = DEVICE_DT_GET(DT_INST_PARENT(id)),                                       \
		.resolution = DT_INST_PROP_OR(id, resolution, 16),                                 \
		.nchannels = DT_INST_PROP_OR(id, num_channels, 2),                                 \
		.gpio_busy = GPIO_DT_SPEC_INST_GET_OR(id, busy_gpios, {0}),                        \
		.gpio_ldac = GPIO_DT_SPEC_INST_GET_OR(id, ldac_gpios, {0}),                        \
		.latch_mode = DT_INST_PROP_OR(id, latch_mode, {0}),                                \
		.polarity_mode = DT_INST_PROP_OR(id, polarity_mode, {0}),                          \
		.dac_mode = DT_INST_PROP_OR(id, dac_mode, {0}),                                    \
		.ovc_mode = DT_INST_PROP_OR(id, overcurrent_mode, {0}),                            \
		.timeout = DT_INST_PROP_OR(id, timeout, 0),                                        \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(id, max22017_init, NULL, NULL, &dac_adi_max22017_config_##id,        \
			      POST_KERNEL, CONFIG_DAC_MAX22017_INIT_PRIORITY,                      \
			      &max22017_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DAC_MAX22017_DEVICE);
