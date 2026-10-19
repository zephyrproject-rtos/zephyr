/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_sdm

#include <errno.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <esp_clk_tree.h>
#include <esp_err.h>
#include <esp_private/esp_clk_tree_common.h>
#include <hal/gpio_ll.h>
#include <hal/sdm_caps.h>
#include <hal/sdm_ll.h>
#include <soc/clk_tree_defs.h>

#include "driver/sdm.h"

LOG_MODULE_REGISTER(esp_sdm, CONFIG_SOC_LOG_LEVEL);

struct esp32_sdm_channel_cfg {
	uint8_t id;
	uint32_t sample_rate_hz;
};

struct esp32_sdm_config {
	const struct pinctrl_dev_config *pcfg;
	const struct esp32_sdm_channel_cfg *channels;
	uint8_t nchannels;
};

static const struct esp32_sdm_channel_cfg *
sdm_find_channel(const struct esp32_sdm_config *cfg, uint8_t channel)
{
	for (uint8_t i = 0U; i < cfg->nchannels; i++) {
		if (cfg->channels[i].id == channel) {
			return &cfg->channels[i];
		}
	}

	return NULL;
}

int esp32_sdm_set_pulse_density(const struct device *dev, uint8_t channel, int8_t density)
{
	const struct esp32_sdm_config *cfg = dev->config;
	gpio_sd_dev_t *hw = SDM_LL_GET_HW(0);
	unsigned int key;

	if (sdm_find_channel(cfg, channel) == NULL) {
		return -EINVAL;
	}

	key = irq_lock();
	sdm_ll_set_pulse_density(hw, channel, density);
	irq_unlock(key);

	return 0;
}

static int sdm_source_hz(uint32_t *src_hz)
{
	soc_module_clk_t clk_src = (soc_module_clk_t)SDM_CLK_SRC_DEFAULT;

	if (esp_clk_tree_enable_src(clk_src, true) != ESP_OK) {
		return -EIO;
	}

#if defined(GPIO_LL_CLK_SRC_SELECTABLE) && GPIO_LL_CLK_SRC_SELECTABLE
	gpio_ll_iomux_set_clk_src(clk_src);
#endif

	if (esp_clk_tree_src_get_freq_hz(clk_src, ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED, src_hz) !=
	    ESP_OK) {
		return -EIO;
	}

	if (*src_hz == 0U) {
		return -EIO;
	}

	return 0;
}

static int sdm_configure_channel(gpio_sd_dev_t *hw, const struct esp32_sdm_config *cfg,
				 uint8_t index, uint32_t src_hz)
{
	const struct esp32_sdm_channel_cfg *channel = &cfg->channels[index];
	uint32_t prescale;
	unsigned int key;

	if (channel->id >= SDM_CAPS_GET(CHANS_PER_INST)) {
		LOG_ERR("channel %u is not present", channel->id);
		return -EINVAL;
	}

	for (uint8_t prev = 0U; prev < index; prev++) {
		if (cfg->channels[prev].id == channel->id) {
			LOG_ERR("channel %u is described more than once", channel->id);
			return -EINVAL;
		}
	}

	if ((channel->sample_rate_hz == 0U) || (channel->sample_rate_hz > src_hz)) {
		LOG_ERR("sample rate %u is out of range", channel->sample_rate_hz);
		return -EINVAL;
	}

	prescale = src_hz / channel->sample_rate_hz;
	if ((prescale == 0U) || (prescale > SDM_LL_PRESCALE_MAX)) {
		LOG_ERR("prescale %u is out of range", prescale);
		return -EINVAL;
	}

	key = irq_lock();
	sdm_ll_set_prescale(hw, channel->id, prescale);
	sdm_ll_set_pulse_density(hw, channel->id, 0);
	irq_unlock(key);

	return 0;
}

static int esp32_sdm_init(const struct device *dev)
{
	const struct esp32_sdm_config *cfg = dev->config;
	gpio_sd_dev_t *hw = SDM_LL_GET_HW(0);
	uint32_t src_hz;
	int ret;

	if (cfg->nchannels == 0U) {
		LOG_ERR("no channels described");
		return -EINVAL;
	}

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		LOG_ERR("pinctrl apply failed (%d)", ret);
		return ret;
	}

	if (hw == NULL) {
		return -ENODEV;
	}

	sdm_ll_enable_clock(hw, true);

	ret = sdm_source_hz(&src_hz);
	if (ret != 0) {
		return ret;
	}

	for (uint8_t i = 0U; i < cfg->nchannels; i++) {
		ret = sdm_configure_channel(hw, cfg, i, src_hz);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

#define SDM_CHANNEL_CFG(node_id)                                                                   \
	{                                                                                          \
		.id = DT_REG_ADDR(node_id),                                                        \
		.sample_rate_hz = DT_PROP(node_id, sample_rate_hz),                                \
	},

#define ESP32_SDM_DEFINE(inst)                                                                 \
	COND_CODE_1(DT_INST_CHILD_NUM_STATUS_OKAY(inst),                                           \
		    (static const struct esp32_sdm_channel_cfg sdm_channels_##inst[] = {       \
			     DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, SDM_CHANNEL_CFG)};),          \
		    ())                                                                            \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	static const struct esp32_sdm_config sdm_config_##inst = {                             \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
		.channels = COND_CODE_1(DT_INST_CHILD_NUM_STATUS_OKAY(inst),                       \
					(sdm_channels_##inst), (NULL)),                            \
		.nchannels = DT_INST_CHILD_NUM_STATUS_OKAY(inst),                                  \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, esp32_sdm_init, NULL, NULL, &sdm_config_##inst,            \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

DT_INST_FOREACH_STATUS_OKAY(ESP32_SDM_DEFINE)
