/*
 * Copyright (c) 2026 Peter Johanson
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT worldsemi_ws2812_stm32_timer

#include <zephyr/drivers/led_strip.h>

#define LOG_LEVEL CONFIG_LED_STRIP_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ws2812_stm32_timer);

#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_stm32.h>
#include <zephyr/dt-bindings/led/led.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <stm32_cache.h>

BUILD_ASSERT((!IS_ENABLED(CONFIG_DCACHE) || IS_ENABLED(CONFIG_NOCACHE_MEMORY)),
	     "Need CONFIG_NOCACHE_MEMORY when CONFIG_DCACHE is enabled");

struct ws2812_stm32_timer_cfg {
	struct pwm_dt_spec pwm;
	struct device const *dma_dev;
	TIM_TypeDef *timer_base;
	uint32_t dma_channel;
	size_t tx_buf_bytes;
	struct k_mem_slab *mem_slab;
	uint8_t num_colors;
	size_t length;
	const uint8_t *color_mapping;
	uint16_t zero_high_ns;
	uint16_t one_high_ns;
	uint16_t reset_codes;
};

struct ws2812_stm32_timer_data {
	struct dma_config dma_config;
	struct dma_block_config dma_block_config;

	uint16_t zero_high_cycles;
	uint16_t one_high_cycles;
	struct k_sem sem;
	void *mem_block;
	int64_t last_dma_done;
};

static void ws2812_strip_dma_callback(const struct device *dev, void *user_data, uint32_t channel,
				      int status)
{
	const struct device *ws_dev = (const struct device *)user_data;
	const struct ws2812_stm32_timer_cfg *cfg = ws_dev->config;
	struct ws2812_stm32_timer_data *data = ws_dev->data;
	int ret;

	k_mem_slab_free(cfg->mem_slab, data->mem_block);
	data->mem_block = NULL;

	ret = pwm_set_pulse_dt(&cfg->pwm, 0);
	if (ret) {
		LOG_ERR("Failed to set the period (%d)", ret);
	}

	data->last_dma_done = k_uptime_get();

	k_sem_give(&data->sem);
}

static int ws2812_strip_update_rgb(const struct device *dev, struct led_rgb *pixels,
				   size_t num_pixels)
{
	const struct ws2812_stm32_timer_cfg *cfg = dev->config;
	struct ws2812_stm32_timer_data *data = dev->data;
	uint16_t *tx_buf;
	void *mem_block;
	uint16_t zero_high_cycles = data->zero_high_cycles;
	uint16_t one_high_cycles = data->one_high_cycles;
	int ret;

	/* Acquire memory for the compare value sequence, which we can prepare
	 * while an existing DMA burst is running.
	 */
	ret = k_mem_slab_alloc(cfg->mem_slab, &mem_block, K_SECONDS(10));
	if (ret < 0) {
		LOG_ERR("Unable to allocate mem slab for TX (err %d)", ret);
		return -ENOMEM;
	}

	tx_buf = (uint16_t *)mem_block;

	/* Add a pre-data reset, so the first pixel isn't skipped by the strip. */
	for (uint16_t i = 0; i < cfg->reset_codes; i++) {
		*tx_buf = 0x00;
		tx_buf++;
	}

	for (uint16_t i = 0; i < num_pixels; i++) {
		for (uint16_t j = 0; j < cfg->num_colors; j++) {
			uint8_t pixel;

			switch (cfg->color_mapping[j]) {
			/* White channel is not supported by LED strip API. */
			case LED_COLOR_ID_WHITE:
				pixel = 0;
				break;
			case LED_COLOR_ID_RED:
				pixel = pixels[i].r;
				break;
			case LED_COLOR_ID_GREEN:
				pixel = pixels[i].g;
				break;
			case LED_COLOR_ID_BLUE:
				pixel = pixels[i].b;
				break;
			default:
				return -EINVAL;
			}

			/* High bits are sent first */
			for (int8_t bit = 7; bit >= 0; bit--) {
				*tx_buf =
					IS_BIT_SET(pixel, bit) ? one_high_cycles : zero_high_cycles;
				tx_buf++;
			}
		}
	}

	k_sem_take(&data->sem, K_FOREVER);

	data->mem_block = mem_block;
	data->dma_block_config.source_address = (uint32_t)mem_block;

	if (dma_config(cfg->dma_dev, cfg->dma_channel, &data->dma_config) != 0) {
		LOG_ERR("DMA config failed");
		k_mem_slab_free(cfg->mem_slab, data->mem_block);
		data->mem_block = NULL;
		return -EIO;
	}

	if (dma_start(cfg->dma_dev, cfg->dma_channel) != 0) {
		LOG_ERR("DMA start failed");
		k_mem_slab_free(cfg->mem_slab, data->mem_block);
		data->mem_block = NULL;
		return -EIO;
	}

	return 0;
}

static size_t ws2812_strip_length(const struct device *dev)
{
	const struct ws2812_stm32_timer_cfg *cfg = dev->config;

	return cfg->length;
}

#define TIMER_NS_TO_CYCLES(_cyc_per_sec, _ns) (uint16_t)DIV_ROUND_UP(_cyc_per_sec * _ns, 1000000000)

static int ws2812_stm32_timer_init(const struct device *dev)
{
	const struct ws2812_stm32_timer_cfg *cfg = dev->config;
	struct ws2812_stm32_timer_data *data = dev->data;
	int ret;
	uint64_t cycle_per_sec;

	k_sem_init(&data->sem, 1, 1);

	data->dma_config.head_block = &data->dma_block_config;
	data->dma_config.user_data = (void *)dev;
	data->dma_block_config.dest_address =
		(uint32_t)(&cfg->timer_base->CCR1 + (cfg->pwm.channel - 1));

	ret = pwm_get_cycles_per_sec(cfg->pwm.dev, cfg->pwm.channel, &cycle_per_sec);
	if (ret) {
		LOG_ERR("Failed to get cycles-per-sec for the channel (%d)", ret);
		return -ENODEV;
	}

	data->zero_high_cycles = TIMER_NS_TO_CYCLES(cycle_per_sec, cfg->zero_high_ns);
	data->one_high_cycles = TIMER_NS_TO_CYCLES(cycle_per_sec, cfg->one_high_ns);

	ret = pwm_set_pulse_dt(&cfg->pwm, 0);
	if (ret) {
		LOG_ERR("Failed to set the period (%d)", ret);
		return -ENODEV;
	}

	ret = pwm_enable_dma(cfg->pwm.dev, cfg->pwm.channel);
	if (ret) {
		LOG_ERR("Failed to enable DMA (%d)", ret);
		return -ENODEV;
	}

	return 0;
}

static DEVICE_API(led_strip, ws2812_stm32_timer_api) = {
	.update_rgb = ws2812_strip_update_rgb,
	.length = ws2812_strip_length,
};

#define WS2812_RESET_DELAY_WORDS(idx)      DT_INST_PROP(idx, reset_codes)
#define WS2812_NUM_COLORS(idx)             (DT_INST_PROP_LEN(idx, color_mapping))
#define WS2812_STM32_TIMER_NUM_PIXELS(idx) (DT_INST_PROP(idx, chain_length))

#define PWM_TIMER_BASE(inst)                                                                       \
	((TIM_TypeDef *)DT_REG_ADDR(DT_PARENT(DT_PWMS_CTLR(DT_DRV_INST(inst)))))

#define WS2812_STM32_TIMER_BUFSIZE(idx)                                                            \
	((((WS2812_NUM_COLORS(idx) * WS2812_STM32_TIMER_NUM_PIXELS(idx) * 8) +                     \
	   WS2812_RESET_DELAY_WORDS(idx)) *                                                        \
	  2))

#define WS2812_STM32_TIMER_DEVICE(idx)                                                             \
	K_MEM_SLAB_DEFINE_IN_SECT_STATIC(                                                          \
		ws2812_stm32_timer_##idx##_slab,                                                   \
		COND_CODE_1(CONFIG_WS2812_STRIP_STM32_TIMER_FORCE_NOCACHE,                         \
			    (__nocache_noinit),                                                    \
			    (__noinit_named(k_mem_slab_buf_ws2812_stm32_timer_##idx##_slab))),     \
		WS2812_STM32_TIMER_BUFSIZE(idx), 2, 2);                                            \
												   \
	static const uint8_t ws2812_stm32_timer_##idx##_color_mapping[] =                          \
		DT_INST_PROP(idx, color_mapping);                                                  \
                                                                                                   \
	static struct ws2812_stm32_timer_data ws2812_stm32_timer_##idx##_data = {                  \
		.dma_config =                                                                      \
			{                                                                          \
				.dma_slot = STM32_DMA_SLOT(idx, tx, slot),                         \
				.channel_direction = STM32_DMA_CONFIG_DIRECTION(                   \
					STM32_DMA_CHANNEL_CONFIG(idx, tx)),                        \
				.channel_priority = STM32_DMA_CONFIG_PRIORITY(                     \
					STM32_DMA_CHANNEL_CONFIG(idx, tx)),                        \
				.source_data_size = STM32_DMA_CONFIG_MEMORY_DATA_SIZE(             \
					STM32_DMA_CHANNEL_CONFIG(idx, tx)),                        \
				.dest_data_size = STM32_DMA_CONFIG_PERIPHERAL_DATA_SIZE(           \
					STM32_DMA_CHANNEL_CONFIG(idx, tx)),                        \
				.source_burst_length = 1, /* SINGLE transfer */                    \
				.dest_burst_length = 1,   /* SINGLE transfer */                    \
				.block_count = 1,                                                  \
				.complete_callback_en = true,                                      \
				.error_callback_dis = false,                                       \
				.dma_callback = ws2812_strip_dma_callback,                         \
			},                                                                         \
		.dma_block_config =                                                                \
			{                                                                          \
				.source_addr_adj = DMA_ADDR_ADJ_INCREMENT,                         \
				.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE,                           \
				.block_size = WS2812_STM32_TIMER_BUFSIZE(idx),                     \
				.source_reload_en = false, /* circular mode */                     \
				.dest_reload_en = false,   /* circular mode */                     \
			},                                                                         \
	};                                                                                         \
	static const struct ws2812_stm32_timer_cfg ws2812_stm32_timer_##idx##_cfg = {              \
		.pwm = PWM_DT_SPEC_INST_GET(idx),                                                  \
		.dma_dev = DEVICE_DT_GET(DT_INST_PHANDLE(idx, dmas)),                              \
		.timer_base = PWM_TIMER_BASE(idx),                                                 \
		.dma_channel = DT_INST_DMAS_CELL_BY_NAME(idx, tx, channel),                        \
		.tx_buf_bytes = WS2812_STM32_TIMER_BUFSIZE(idx),                                   \
		.mem_slab = &ws2812_stm32_timer_##idx##_slab,                                      \
		.num_colors = WS2812_NUM_COLORS(idx),                                              \
		.length = DT_INST_PROP(idx, chain_length),                                         \
		.color_mapping = ws2812_stm32_timer_##idx##_color_mapping,                         \
		.reset_codes = DT_INST_PROP(idx, reset_codes),                                     \
		.zero_high_ns = DT_INST_PROP(idx, zero_high_ns),                                   \
		.one_high_ns = DT_INST_PROP(idx, one_high_ns),                                     \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(idx, ws2812_stm32_timer_init, NULL,                                  \
			      &ws2812_stm32_timer_##idx##_data, &ws2812_stm32_timer_##idx##_cfg,   \
			      POST_KERNEL, CONFIG_LED_STRIP_INIT_PRIORITY,                         \
			      &ws2812_stm32_timer_api);

DT_INST_FOREACH_STATUS_OKAY(WS2812_STM32_TIMER_DEVICE)
