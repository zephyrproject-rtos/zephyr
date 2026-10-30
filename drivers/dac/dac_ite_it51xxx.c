/*
 * Copyright (c) 2026 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it51xxx_dac

#include <zephyr/drivers/dac.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(dac_ite_it51xxx, CONFIG_DAC_LOG_LEVEL);

#define IT51XXX_DAC_CHANNEL_COUNT 2U
#define IT51XXX_DAC_RESOLUTION    8U

/* DAC Power Down Register */
#define IT51XXX_DACPDREG 0x01U
/* DAC Data Channel 0 Register */
#define IT51XXX_DACDAT0  0x02U
/* DAC Data Channel 1 Register */
#define IT51XXX_DACDAT1  0x03U

/* DAC Data Channel Register */
#define IT51XXX_DACDAT(ch)     (IT51XXX_DACDAT0 + (ch))
#define IT51XXX_DAC_POWDN_MASK GENMASK(1, 0)

struct dac_it51xxx_config {
	const struct pinctrl_dev_config *pcfg;
	uintptr_t base;
};

struct dac_it51xxx_data {
	struct k_spinlock lock;
};

static int dac_it51xxx_channel_setup(const struct device *dev,
				     const struct dac_channel_cfg *channel_cfg)
{
	const struct dac_it51xxx_config *config = dev->config;
	struct dac_it51xxx_data *data = dev->data;
	k_spinlock_key_t key;
	uint8_t reg_val;

	if (channel_cfg->channel_id >= IT51XXX_DAC_CHANNEL_COUNT) {
		LOG_ERR("Invalid DAC channel %u", channel_cfg->channel_id);
		return -EINVAL;
	}

	if (channel_cfg->resolution != IT51XXX_DAC_RESOLUTION) {
		LOG_ERR("Unsupported DAC resolution %u; expected %u", channel_cfg->resolution,
			IT51XXX_DAC_RESOLUTION);
		return -ENOTSUP;
	}

	if (channel_cfg->buffered) {
		LOG_ERR("DAC channel %u does not support output buffer", channel_cfg->channel_id);
		return -ENOTSUP;
	}

	if (channel_cfg->internal) {
		LOG_ERR("DAC channel %u does not support internal output", channel_cfg->channel_id);
		return -ENOTSUP;
	}

	/* Preserve the other channel and reserved bits */
	key = k_spin_lock(&data->lock);

	reg_val = sys_read8(config->base + IT51XXX_DACPDREG);
	/* The DAC channel_id is not power-down */
	reg_val &= ~BIT(channel_cfg->channel_id);
	sys_write8(reg_val, config->base + IT51XXX_DACPDREG);

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int dac_it51xxx_write_value(const struct device *dev, uint8_t channel, uint32_t value)
{
	const struct dac_it51xxx_config *config = dev->config;

	if (channel >= IT51XXX_DAC_CHANNEL_COUNT) {
		LOG_ERR("Invalid DAC channel %u", channel);
		return -EINVAL;
	}

	if (value >= BIT(IT51XXX_DAC_RESOLUTION)) {
		LOG_ERR("DAC channel %u value %u exceeds the 8-bit range", channel, value);
		return -EINVAL;
	}

	if ((sys_read8(config->base + IT51XXX_DACPDREG) & BIT(channel)) != 0U) {
		LOG_ERR("DAC channel %u is powered down; call dac_channel_setup() first", channel);
		return -EINVAL;
	}

	/* 8-bit data will be loaded to the DAC for D/A operation */
	sys_write8((uint8_t)value, config->base + IT51XXX_DACDAT(channel));

	return 0;
}

static int dac_it51xxx_init(const struct device *dev)
{
	const struct dac_it51xxx_config *config = dev->config;
	int ret;
	uint8_t reg_val;

	/*
	 * Keep both channels powered down until channel setup, and initialize their
	 * output values before switching the pins to the DAC alternate function.
	 */
	reg_val = sys_read8(config->base + IT51XXX_DACPDREG);
	sys_write8(reg_val | IT51XXX_DAC_POWDN_MASK, config->base + IT51XXX_DACPDREG);
	sys_write8(0U, config->base + IT51XXX_DACDAT0);
	sys_write8(0U, config->base + IT51XXX_DACDAT1);

	/* Set the pin to DAC alternate function */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("%s: Failed to configure DAC pins (%d)", dev->name, ret);
		return ret;
	}

	return 0;
}

static DEVICE_API(dac, dac_it51xxx_api) = {
	.channel_setup = dac_it51xxx_channel_setup,
	.write_value = dac_it51xxx_write_value,
};

#define IT51XXX_DAC_INIT(inst)                                                                     \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	static struct dac_it51xxx_data dac_it51xxx_data_##inst;                                    \
	static const struct dac_it51xxx_config dac_it51xxx_config_##inst = {                       \
		.base = DT_INST_REG_ADDR(inst),                                                    \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, dac_it51xxx_init, NULL, &dac_it51xxx_data_##inst,              \
			      &dac_it51xxx_config_##inst, POST_KERNEL, CONFIG_DAC_INIT_PRIORITY,   \
			      &dac_it51xxx_api);

DT_INST_FOREACH_STATUS_OKAY(IT51XXX_DAC_INIT)
