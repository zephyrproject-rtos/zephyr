/*
 * Copyright (c) 2023 TOKITA Hiroshi
 * Copyright (c) 2025 The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/drivers/misc/pio_rpi_pico/pio_rpi_pico.h>
#include <zephyr/dt-bindings/led/led.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ws2812_rpi_pico_pio, CONFIG_LED_STRIP_LOG_LEVEL);

#if defined(CONFIG_DMA)

#include <zephyr/drivers/dma.h>
#if defined(CONFIG_SOC_SERIES_RP2040)
#include <zephyr/dt-bindings/dma/rpi-pico-dma-rp2040.h>
#elif defined(CONFIG_SOC_SERIES_RP2350)
#include <zephyr/dt-bindings/dma/rpi-pico-dma-rp2350.h>
#endif

#include "hardware/dma.h"

#endif

#define DT_DRV_COMPAT worldsemi_ws2812_rpi_pico_pio

#if defined(CONFIG_DMA)
struct ws2812_led_strip_dma_data {
	struct dma_config dma_config;
	struct dma_block_config dma_block;
	struct k_sem complete_sem;
	uint32_t *pixel_buf;
};

struct ws2812_led_strip_dma_config {
	const struct device *dev;
	uint32_t tx_channel;
};
#endif

struct ws2812_led_strip_data {
	uint32_t sm;
	struct k_timer reset_on_complete_timer;
#if defined(CONFIG_DMA)
	struct ws2812_led_strip_dma_data *dma_data;
#endif
};

struct ws2812_led_strip_config {
	const struct device *piodev;
	const uint8_t gpio_pin;
	uint8_t num_colors;
	size_t length;
	uint32_t frequency;
	const uint8_t *const color_mapping;
	uint16_t reset_delay;
	uint32_t cycles_per_bit;
#if defined(CONFIG_DMA)
	const struct ws2812_led_strip_dma_config dma_config;
#endif
};

struct ws2812_rpi_pico_pio_config {
	const struct device *piodev;
	const struct pinctrl_dev_config *const pcfg;
	struct pio_program program;
};

static int ws2812_led_strip_sm_init(const struct device *dev)
{
	const struct ws2812_led_strip_config *config = dev->config;
	const float clkdiv =
		sys_clock_hw_cycles_per_sec() / (config->cycles_per_bit * config->frequency);
	pio_sm_config sm_config = pio_get_default_sm_config();
	PIO pio;
	int sm;

	pio = pio_rpi_pico_get_pio(config->piodev);

	sm = pio_claim_unused_sm(pio, false);
	if (sm < 0) {
		return -EINVAL;
	}

	sm_config_set_sideset(&sm_config, 1, false, false);
	sm_config_set_sideset_pins(&sm_config, config->gpio_pin);
	sm_config_set_out_shift(&sm_config, false, true, (config->num_colors == 4 ? 32 : 24));
	sm_config_set_fifo_join(&sm_config, PIO_FIFO_JOIN_TX);
	sm_config_set_clkdiv(&sm_config, clkdiv);
	pio_sm_set_consecutive_pindirs(pio, sm, config->gpio_pin, 1, true);
	pio_sm_init(pio, sm, -1, &sm_config);
	pio_sm_set_enabled(pio, sm, true);

	return sm;
}

static inline uint32_t ws2812_led_strip_map_color(const struct device *dev, struct led_rgb *pixel)
{
	const struct ws2812_led_strip_config *config = dev->config;
	uint32_t color = 0;

	for (size_t j = 0; j < config->num_colors; j++) {
		switch (config->color_mapping[j]) {
		case LED_COLOR_ID_RED:
			color |= pixel->r << (8 * (2 - j));
			break;
		case LED_COLOR_ID_GREEN:
			color |= pixel->g << (8 * (2 - j));
			break;
		case LED_COLOR_ID_BLUE:
			color |= pixel->b << (8 * (2 - j));
			break;
		/* White channel is not supported by LED strip API. */
		case LED_COLOR_ID_WHITE:
		default:
			color |= 0;
			break;
		}
	}
	return color << (config->num_colors == 4 ? 0 : 8);
}

#if defined(CONFIG_DMA)

static int ws2812_led_strip_dma_setup(const struct device *dev);

static int ws2812_led_strip_start_dma_put(const struct device *dev, struct led_rgb *pixels,
					  size_t num_pixels)
{
	const struct ws2812_led_strip_config *dev_cfg = dev->config;
	struct ws2812_led_strip_data *data = dev->data;
	int err;

	for (size_t i = 0; i < num_pixels; i++) {
		data->dma_data->pixel_buf[i] = ws2812_led_strip_map_color(dev, &pixels[i]);
	}

	err = ws2812_led_strip_dma_setup(dev);

	if (err < 0) {
		dma_stop(dev_cfg->dma_config.dev, dev_cfg->dma_config.tx_channel);

		return err;
	}

	k_sem_take(&data->dma_data->complete_sem, K_FOREVER);

	k_timer_start(&data->reset_on_complete_timer, K_USEC(dev_cfg->reset_delay), K_NO_WAIT);

	return 0;
}

static void ws2812_led_strip_dma_callback(const struct device *dma_dev, void *arg, uint32_t channel,
					  int status)
{
	const struct device *dev = (const struct device *)arg;
	struct ws2812_led_strip_data *data = dev->data;
	const struct ws2812_led_strip_config *dev_cfg = dev->config;

	if (dma_dev != dev_cfg->dma_config.dev) {
		/* rpi pico has only one dma controller so far
		 * so this branch will never run in theory.
		 */
		return;
	}

	if (status < 0) {
		LOG_ERR("dma:%p ch:%d callback gets error: %d", dma_dev, channel, status);

		return;
	}

	if (channel == dev_cfg->dma_config.tx_channel) {
		k_timer_start(&data->reset_on_complete_timer, K_USEC(dev_cfg->reset_delay),
			      K_NO_WAIT);

		dma_stop(dev_cfg->dma_config.dev, dev_cfg->dma_config.tx_channel);
		k_sem_give(&data->dma_data->complete_sem);
	}
}

static int ws2812_led_strip_dma_setup(const struct device *dev)
{
	struct ws2812_led_strip_data *data = dev->data;
	const struct ws2812_led_strip_config *dev_cfg = dev->config;
	struct dma_config *dma_cfg = &data->dma_data->dma_config;
	struct dma_block_config *block_cfg = &data->dma_data->dma_block;
	uint32_t dma_channel = dev_cfg->dma_config.tx_channel;
	PIO pio = pio_rpi_pico_get_pio(dev_cfg->piodev);
	int ret;

	memset(dma_cfg, 0, sizeof(struct dma_config));
	memset(block_cfg, 0, sizeof(struct dma_block_config));

	dma_cfg->source_burst_length = 1;
	dma_cfg->dest_burst_length = 1;
	dma_cfg->user_data = (void *)dev;
	dma_cfg->block_count = 1U;
	dma_cfg->head_block = block_cfg;
	dma_cfg->channel_direction = MEMORY_TO_PERIPHERAL;
	dma_cfg->source_data_size = 4;
	dma_cfg->dest_data_size = 4;

	/* in pio_get_dreq, true => tx */
	dma_cfg->dma_slot = RPI_PICO_DMA_DREQ_TO_SLOT(pio_get_dreq(pio, data->sm, true));

	block_cfg->block_size = dev_cfg->length;
	dma_cfg->dma_callback = ws2812_led_strip_dma_callback;

	block_cfg->dest_address = (uint32_t)&pio->txf[data->sm];
	block_cfg->dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	block_cfg->source_address = (uint32_t)data->dma_data->pixel_buf;
	block_cfg->source_addr_adj = DMA_ADDR_ADJ_INCREMENT;

	ret = dma_config(dev_cfg->dma_config.dev, dma_channel, dma_cfg);
	if (ret < 0) {
		LOG_ERR("dma ctrl %p: dma_config failed with %d", dev_cfg->dma_config.dev, ret);
		return ret;
	}

	ret = dma_start(dev_cfg->dma_config.dev, dma_channel);
	if (ret < 0) {
		LOG_ERR("dma ctrl %p: dma_start failed with %d", dev_cfg->dma_config.dev, ret);
		return ret;
	}

	return 0;
}

static inline bool ws2812_led_strip_use_dma(const struct device *dev)
{
	const struct ws2812_led_strip_config *config = dev->config;

	return config->dma_config.dev;
}

#endif /* defined(CONFIG_DMA) */

static int ws2812_led_strip_update_rgb(const struct device *dev, struct led_rgb *pixels,
				       size_t num_pixels)
{
	const struct ws2812_led_strip_config *config = dev->config;
	struct ws2812_led_strip_data *data = dev->data;
	PIO pio = pio_rpi_pico_get_pio(config->piodev);

	/* Wait for the delay needed to latch current color values on
	 * WS2812 and reset its state machine.
	 */
	k_timer_status_sync(&data->reset_on_complete_timer);

#if defined(CONFIG_DMA)
	if (ws2812_led_strip_use_dma(dev)) {
		return ws2812_led_strip_start_dma_put(dev, pixels, num_pixels);
	}
#endif

	for (size_t i = 0; i < num_pixels; i++) {
		pio_sm_put_blocking(pio, data->sm, ws2812_led_strip_map_color(dev, &pixels[i]));
	}

	k_timer_start(&data->reset_on_complete_timer, K_USEC(config->reset_delay), K_NO_WAIT);

	return 0;
}

static size_t ws2812_led_strip_length(const struct device *dev)
{
	const struct ws2812_led_strip_config *config = dev->config;

	return config->length;
}

static DEVICE_API(led_strip, ws2812_led_strip_api) = {
	.update_rgb = ws2812_led_strip_update_rgb,
	.length = ws2812_led_strip_length,
};

/*
 * Retrieve the channel to color mapping (e.g. RGB, BGR, GRB, ...) from the
 * "color-mapping" DT property.
 */
static int ws2812_led_strip_init(const struct device *dev)
{
	const struct ws2812_led_strip_config *config = dev->config;
	struct ws2812_led_strip_data *data = dev->data;
	int sm;

	if (!device_is_ready(config->piodev)) {
		LOG_ERR("%s: PIO device not ready", dev->name);
		return -ENODEV;
	}

	for (uint32_t i = 0; i < config->num_colors; i++) {
		switch (config->color_mapping[i]) {
		case LED_COLOR_ID_WHITE:
		case LED_COLOR_ID_RED:
		case LED_COLOR_ID_GREEN:
		case LED_COLOR_ID_BLUE:
			break;
		default:
			LOG_ERR("%s: invalid channel to color mapping."
				" Check the color-mapping DT property",
				dev->name);
			return -EINVAL;
		}
	}

	sm = ws2812_led_strip_sm_init(dev);
	if (sm < 0) {
		return sm;
	}

	data->sm = sm;

	k_timer_init(&data->reset_on_complete_timer, NULL, NULL);

#if defined(CONFIG_DMA)
	if (ws2812_led_strip_use_dma(dev)) {
		k_sem_init(&data->dma_data->complete_sem, 0, 1);
	}
#endif

	return 0;
}

static int ws2812_rpi_pico_pio_init(const struct device *dev)
{
	const struct ws2812_rpi_pico_pio_config *config = dev->config;
	PIO pio;

	if (!device_is_ready(config->piodev)) {
		LOG_ERR("%s: PIO device not ready", dev->name);
		return -ENODEV;
	}

	pio = pio_rpi_pico_get_pio(config->piodev);

	pio_add_program(pio, &config->program);

	return pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
}

#define DMA_ENABLED(node) COND_CODE_1(CONFIG_DMA, (DT_DMAS_HAS_NAME(node, tx)), (false))

#define DMA_DATA_NAME(node) ws2812_led_strip_##node##_dma_data

#define DMA_DATA_DECL(node)                                                                        \
	uint32_t ws2812_led_strip_##node##_pixel_buf[DT_PROP(node, chain_length)];                 \
	static struct ws2812_led_strip_dma_data DMA_DATA_NAME(node) = {                            \
		.pixel_buf = ws2812_led_strip_##node##_pixel_buf,                                  \
	};

#define DMA_CFG_INITIALIZER(node)                                                                  \
	{                                                                                          \
		.dev = DEVICE_DT_GET(DT_DMAS_CTLR_BY_NAME(node, tx)),                              \
		.tx_channel = DT_DMAS_CELL_BY_NAME(node, tx, channel),                             \
	}

#define DMA_CFG_DECL(node) COND_CODE_1(DMA_ENABLED(node), (DMA_CFG_INITIALIZER(node)), ({0}))

#define CYCLES_PER_BIT(node)                                                                       \
	(DT_PROP_BY_IDX(node, bit_waveform, 0) + DT_PROP_BY_IDX(node, bit_waveform, 1) +           \
	 DT_PROP_BY_IDX(node, bit_waveform, 2))

#define WS2812_CHILD_INIT(node)                                                                    \
	static const uint8_t ws2812_led_strip_##node##_color_mapping[] =                           \
		DT_PROP(node, color_mapping);                                                      \
                                                                                                   \
	IF_ENABLED(DMA_ENABLED(node), (DMA_DATA_DECL(node)));                                      \
                                                                                                   \
	static struct ws2812_led_strip_data ws2812_led_strip_##node##_data = {                     \
		IF_ENABLED(DMA_ENABLED(node), (.dma_data = &DMA_DATA_NAME(node),))                 \
	};                                                                                         \
                                                                                                   \
	static const struct ws2812_led_strip_config ws2812_led_strip_##node##_config = {           \
		.piodev = DEVICE_DT_GET(DT_PARENT(DT_PARENT(node))),                               \
		.gpio_pin = DT_GPIO_PIN_BY_IDX(node, gpios, 0),                                    \
		.num_colors = DT_PROP_LEN(node, color_mapping),                                    \
		.length = DT_PROP(node, chain_length),                                             \
		.color_mapping = ws2812_led_strip_##node##_color_mapping,                          \
		.reset_delay = DT_PROP(node, reset_delay),                                         \
		.frequency = DT_PROP(node, frequency),                                             \
		.cycles_per_bit = CYCLES_PER_BIT(DT_PARENT(node)),                                 \
		IF_ENABLED(CONFIG_DMA, (.dma_config = DMA_CFG_DECL(node),))                        \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_DEFINE(node, &ws2812_led_strip_init, NULL, &ws2812_led_strip_##node##_data,      \
			 &ws2812_led_strip_##node##_config, POST_KERNEL,                           \
			 CONFIG_LED_STRIP_INIT_PRIORITY, &ws2812_led_strip_api);

#define SET_DELAY(op, inst, i)                                                                     \
	(op | (((DT_INST_PROP_BY_IDX(inst, bit_waveform, i) - 1) & 0xF) << 8))

/*
 * This pio program runs [T0+T1+T2] cycles per 1 loop.
 * The first `out` instruction outputs 0 by [T2] times to the sideset pin.
 * These zeros are padding. Here is the start of actual data transmission.
 * The second `jmp` instruction output 1 by [T0] times to the sideset pin.
 * This `jmp` instruction jumps to line 3 if the value of register x is true.
 * Otherwise, jump to line 4.
 * The third `jmp` instruction outputs 1 by [T1] times to the sideset pin.
 * After output, return to the first line.
 * The fourth `jmp` instruction outputs 0 by [T1] times.
 * After output, return to the first line and output 0 by [T2] times.
 *
 * In the case of configuration, T0=3, T1=3, T2 =4,
 * the final output is 1110000000 in case register x is false.
 * It represents code 0, defined in the datasheet.
 * And outputs 1111110000 in case of x is true. It represents code 1.
 */
#define WS2812_RPI_PICO_PIO_INIT(inst)                                                             \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
                                                                                                   \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, WS2812_CHILD_INIT);                                \
                                                                                                   \
	static const uint16_t rpi_pico_pio_ws2812_instructions_##inst[] = {                        \
		SET_DELAY(0x6021, inst, 2), /*  0: out    x, 1  side 0 [T2 - 1]  */                \
		SET_DELAY(0x1023, inst, 0), /*  1: jmp    !x, 3 side 1 [T0 - 1]  */                \
		SET_DELAY(0x1000, inst, 1), /*  2: jmp    0     side 1 [T1 - 1]  */                \
		SET_DELAY(0x0000, inst, 1), /*  3: jmp    0     side 0 [T1 - 1]  */                \
	};                                                                                         \
                                                                                                   \
	static const struct ws2812_rpi_pico_pio_config rpi_pico_pio_ws2812_##inst##_config = {     \
		.piodev = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                     \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
		.program =                                                                         \
			{                                                                          \
				.instructions = rpi_pico_pio_ws2812_instructions_##inst,           \
				.length = ARRAY_SIZE(rpi_pico_pio_ws2812_instructions_##inst),     \
				.origin = -1,                                                      \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &ws2812_rpi_pico_pio_init, NULL, NULL,                         \
			      &rpi_pico_pio_ws2812_##inst##_config, POST_KERNEL,                   \
			      CONFIG_LED_STRIP_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(WS2812_RPI_PICO_PIO_INIT)
