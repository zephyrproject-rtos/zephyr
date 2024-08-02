/*
 * Copyright (c) 2023 TOKITA Hiroshi
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

#define DT_DRV_COMPAT worldsemi_ws2812_rpi_pico_pio

struct ws2812_led_strip_data {
	uint32_t sm;
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

/*
 * Latch current color values on strip and reset its state machines.
 */
static inline void ws2812_led_strip_reset_delay(uint16_t delay)
{
	k_usleep(delay);
}

static int ws2812_led_strip_update_rgb(const struct device *dev, struct led_rgb *pixels,
				       size_t num_pixels)
{
	const struct ws2812_led_strip_config *config = dev->config;
	struct ws2812_led_strip_data *data = dev->data;
	PIO pio = pio_rpi_pico_get_pio(config->piodev);

	for (size_t i = 0; i < num_pixels; i++) {
		uint32_t color = 0;

		for (size_t j = 0; j < config->num_colors; j++) {
			switch (config->color_mapping[j]) {
			/* White channel is not supported by LED strip API. */
			case LED_COLOR_ID_WHITE:
				color |= 0;
				break;
			case LED_COLOR_ID_RED:
				color |= pixels[i].r << (8 * (2 - j));
				break;
			case LED_COLOR_ID_GREEN:
				color |= pixels[i].g << (8 * (2 - j));
				break;
			case LED_COLOR_ID_BLUE:
				color |= pixels[i].b << (8 * (2 - j));
				break;
			}
		}

		pio_sm_put_blocking(pio, data->sm, color << (config->num_colors == 4 ? 0 : 8));
	}

	ws2812_led_strip_reset_delay(config->reset_delay);

	return 0;
}

static size_t ws2812_led_strip_length(const struct device *dev)
{
	const struct ws2812_led_strip_config *config = dev->config;

	return config->length;
}

static const struct led_strip_driver_api ws2812_led_strip_api = {
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

#define CYCLES_PER_BIT(node)                                                                       \
	(DT_PROP_BY_IDX(node, bit_waveform, 0) + DT_PROP_BY_IDX(node, bit_waveform, 1) +           \
	 DT_PROP_BY_IDX(node, bit_waveform, 2))

#define WS2812_CHILD_INIT(node)                                                                    \
	static const uint8_t ws2812_led_strip_##node##_color_mapping[] =                           \
		DT_PROP(node, color_mapping);                                                      \
	struct ws2812_led_strip_data ws2812_led_strip_##node##_data;                               \
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
