/*
 * Copyright (c) 2023 Christian Bach
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT worldsemi_ws2812_pio

#include <zephyr/drivers/led_strip.h>
#include <zephyr/drivers/pinctrl.h>

#include <string.h>

#define LOG_LEVEL CONFIG_LED_STRIP_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ws2812_pio);

#include <zephyr/drivers/misc/pio_rpi_pico/pio_rpi_pico.h>

#include <hardware/pio.h>
#include <hardware/clocks.h>
#include <zephyr/kernel.h>
#include <zephyr/dt-bindings/led/led.h>

struct pio_ws2812_config {
	const struct device *piodev;
	const struct pinctrl_dev_config *pcfg;
	const uint32_t tx_pin;
	uint8_t num_colors;
	uint16_t chain_length;
	const uint8_t *color_mapping;
	uint16_t reset_delay;
	uint32_t baudrate;
};

struct pio_ws2812_data {
	size_t tx_sm;
	uint8_t *px_buf;
	size_t px_buf_size;
};

RPI_PICO_PIO_DEFINE_PROGRAM(ws2818_tx, 0, 3,
			    //     .wrap_target
			    0x6221, //  0: out    x, 1      side 0 [2]
			    0x1123, //  1: jmp    !x, 3     side 1 [1]
			    0x1400, //  2: jmp    0         side 1 [4]
			    0xa442, //  3: nop              side 0 [4]
				    //     .wrap
);

#define CYCLES_PER_BIT    10
#define SIDESET_BIT_COUNT 1

static int pio_ws2818_init(PIO pio, uint32_t sm, uint32_t tx_pin, float sm_clock_div, uint bits)
{
	uint32_t offset;
	pio_sm_config sm_config;

	if (!pio_can_add_program(pio, RPI_PICO_PIO_GET_PROGRAM(ws2818_tx))) {
		LOG_ERR("No space left for the program in PIO");
		return -EBUSY;
	}

	offset = pio_add_program(pio, RPI_PICO_PIO_GET_PROGRAM(ws2818_tx));
	sm_config = pio_get_default_sm_config();

	sm_config_set_sideset(&sm_config, SIDESET_BIT_COUNT, false, false);
	sm_config_set_out_shift(&sm_config, false, true, bits);
	sm_config_set_out_pins(&sm_config, tx_pin, 1);
	sm_config_set_sideset_pins(&sm_config, tx_pin);
	sm_config_set_fifo_join(&sm_config, PIO_FIFO_JOIN_TX);
	sm_config_set_clkdiv(&sm_config, sm_clock_div);
	sm_config_set_wrap(&sm_config, offset + RPI_PICO_PIO_GET_WRAP_TARGET(ws2818_tx),
			   offset + RPI_PICO_PIO_GET_WRAP(ws2818_tx));

	pio_sm_set_pins_with_mask(pio, sm, BIT(tx_pin), BIT(tx_pin));
	pio_sm_set_pindirs_with_mask(pio, sm, BIT(tx_pin), BIT(tx_pin));
	pio_sm_init(pio, sm, offset, &sm_config);
	pio_sm_set_enabled(pio, sm, true);

	return 0;
}

static int ws2812_pio_init(const struct device *dev)
{
	const struct pio_ws2812_config *config = dev->config;
	struct pio_ws2812_data *data = dev->data;
	size_t tx_sm;
	float sm_clock_div;
	int retval;
	PIO pio;

	pio = pio_rpi_pico_get_pio(config->piodev);
	retval = pio_rpi_pico_allocate_sm(config->piodev, &tx_sm);

	if (retval < 0) {
		return retval;
	}

	data->tx_sm = tx_sm;

	sm_clock_div = (float)clock_get_hz(clk_sys) / (CYCLES_PER_BIT * config->baudrate);
	retval = pio_ws2818_init(pio, tx_sm, config->tx_pin, sm_clock_div, config->num_colors * 8);
	if (retval < 0) {
		return retval;
	}

	return pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
}

/*
 * Latch current color values on strip and reset its state machines.
 */
static inline void ws2812_reset_delay(uint16_t delay)
{
	k_usleep(delay);
}

static int ws2812_strip_update_rgb(const struct device *dev, struct led_rgb *pixels,
				   size_t num_pixels)
{
	const struct pio_ws2812_config *config = dev->config;
	struct pio_ws2812_data *data = dev->data;
	PIO pio = pio_rpi_pico_get_pio(config->piodev);
	size_t i;

	if (data->tx_sm == -1) {
		LOG_ERR("Device is not ready");
		return -EBUSY;
	}

	if (num_pixels > config->chain_length) {
		LOG_ERR("The Chain is not that long! (chain length = %i)", config->chain_length);
		return -ENOMEM;
	}

	/* Convert pixel data into SPI frames. Each frame has pixel data
	 * in color mapping on-wire format (e.g. GRB, GRBW, RGB, etc).
	 */
	for (i = 0; i < num_pixels; i++) {
		uint8_t j;
		uint32_t pixel = 0;

		for (j = 0; j < config->num_colors; j++) {
			pixel <<= 8;
			switch (config->color_mapping[j]) {
			// White channel is not supported by LED strip API. *
			case LED_COLOR_ID_WHITE:
				pixel |= 0;
				break;
			case LED_COLOR_ID_RED:
				pixel |= ((uint32_t)pixels[i].r);
				break;
			case LED_COLOR_ID_GREEN:
				pixel |= ((uint32_t)pixels[i].g);
				break;
			case LED_COLOR_ID_BLUE:
				pixel |= ((uint32_t)pixels[i].b);
				break;
			default:
				LOG_ERR("Invalid Color ID detected");
				return -EINVAL;
			}
		}
		pixel <<= 8 * (sizeof(uint32_t) - j);
		pio_sm_put_blocking(pio, data->tx_sm, pixel);
	}
	ws2812_reset_delay(config->reset_delay);
	return 0;
}

static int ws2812_strip_update_channels(const struct device *dev, uint8_t *channels,
					size_t num_channels)
{
	LOG_ERR("update_channels not implemented");
	return -ENOTSUP;
}

static const struct led_strip_driver_api ws2812_pio_api = {
	.update_rgb = ws2812_strip_update_rgb,
	.update_channels = ws2812_strip_update_channels,
};

#define WS2812_NUM_COLORS(idx) (DT_INST_PROP_LEN(idx, color_mapping))

#define WS2812_PIO_NUM_PIXELS(idx) (DT_INST_PROP(idx, chain_length))
#define WS2812_PIO_BUFSZ(idx)      (WS2812_NUM_COLORS(idx) * WS2812_PIO_NUM_PIXELS(idx))

/*
 * Retrieve the channel to color mapping (e.g. RGB, BGR, GRB, ...) from the
 * "color-mapping" DT property.
 */
#define WS2812_COLOR_MAPPING(idx)                                                                  \
	static const uint8_t ws2812_pio_##idx##_color_mapping[] = DT_INST_PROP(idx, color_mapping)

#define WS2812_PIO_DEVICE(idx)                                                                     \
	static uint8_t ws2812_pio_##idx##_px_buf[WS2812_PIO_BUFSZ(idx)];                           \
	WS2812_COLOR_MAPPING(idx);                                                                 \
	PINCTRL_DT_INST_DEFINE(idx);                                                               \
	static const struct pio_ws2812_config pio_ws2812##idx##_config = {                         \
		.piodev = DEVICE_DT_GET(DT_INST_PARENT(idx)),                                      \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),                                       \
		.tx_pin = DT_INST_RPI_PICO_PIO_PIN_BY_NAME(idx, default, 0, tx_pin, 0),            \
		.chain_length = WS2812_PIO_NUM_PIXELS(idx),                                        \
		.num_colors = WS2812_NUM_COLORS(idx),                                              \
		.color_mapping = ws2812_pio_##idx##_color_mapping,                                 \
		.reset_delay = DT_INST_PROP(idx, reset_delay),                                     \
		.baudrate = 800000,                                                                \
	};                                                                                         \
	static struct pio_ws2812_data pio_ws2812##idx##_data = {                                   \
		.px_buf = ws2812_pio_##idx##_px_buf,                                               \
		.px_buf_size = WS2812_PIO_BUFSZ(idx),                                              \
		.tx_sm = -1,                                                                       \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(idx, ws2812_pio_init, NULL, &pio_ws2812##idx##_data,                 \
			      &pio_ws2812##idx##_config, POST_KERNEL,                              \
			      CONFIG_LED_STRIP_INIT_PRIORITY, &ws2812_pio_api);

DT_INST_FOREACH_STATUS_OKAY(WS2812_PIO_DEVICE)