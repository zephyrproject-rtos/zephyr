/*
 * Copyright (c) 2025 Siratul Islam <sirat4757@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * Driver for 32x16 monochrome LED panels with HUB12 interface.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <string.h>

LOG_MODULE_REGISTER(hub12, CONFIG_DISPLAY_LOG_LEVEL);

#define DT_DRV_COMPAT zephyr_hub12

/* Display layout constants */
#define HUB12_ROWS            4
#define HUB12_BYTES_PER_ROW   16
#define HUB12_GROUP_SIZE      4
#define HUB12_NUM_GROUPS      4
#define HUB12_PIXELS_PER_BYTE 8

/* Brightness control parameters */
#define HUB12_PWM_FREQ           1000
#define HUB12_DEFAULT_BRIGHTNESS 5
#define HUB12_MIN_BRIGHTNESS     1
#define HUB12_MAX_BRIGHTNESS     50

struct hub12_config {
	struct gpio_dt_spec pa;
	struct gpio_dt_spec pb;
	struct gpio_dt_spec pe;
	struct gpio_dt_spec plat;
	struct spi_dt_spec spi;
	uint16_t width;
	uint16_t height;
};

struct hub12_data {
	uint8_t *framebuffer;
	uint8_t cache[HUB12_ROWS][HUB12_BYTES_PER_ROW];
	uint8_t current_row;
	struct k_timer scan_timer;
	struct k_work scan_work;
	struct k_sem lock;
	const struct device *dev;
	uint8_t brightness_us;
};

static void hub12_update_cache(struct hub12_data *data, uint8_t row)
{
	const uint8_t *fb = data->framebuffer;

	for (int i = 0; i < HUB12_BYTES_PER_ROW; i++) {
		int group = i / HUB12_GROUP_SIZE;
		int offset = i % HUB12_GROUP_SIZE;
		int reverse_offset = (HUB12_GROUP_SIZE - 1) - offset;
		int fb_idx = reverse_offset * HUB12_NUM_GROUPS * HUB12_ROWS +
			     row * HUB12_NUM_GROUPS + group;

		data->cache[row][i] = fb[fb_idx];
	}
}

static void hub12_scan_row(struct hub12_data *data, const struct hub12_config *config)
{
	uint8_t row = data->current_row;
	int ret;

	struct spi_buf tx_buf = {.buf = data->cache[row], .len = HUB12_BYTES_PER_ROW};
	struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};

	ret = spi_write_dt(&config->spi, &tx);
	if (ret < 0) {
		LOG_ERR("SPI write failed: %d", ret);
		return;
	}

	gpio_pin_set_dt(&config->pe, 0);

	gpio_pin_set_dt(&config->plat, 1);
	k_busy_wait(1);
	gpio_pin_set_dt(&config->plat, 0);

	gpio_pin_set_dt(&config->pa, (row & BIT(0)) ? 1 : 0);
	gpio_pin_set_dt(&config->pb, (row & BIT(1)) ? 1 : 0);

	if (data->brightness_us > 0) {
		gpio_pin_set_dt(&config->pe, 1);
		k_busy_wait(data->brightness_us);
		gpio_pin_set_dt(&config->pe, 0);
	}

	data->current_row = (data->current_row + 1) % HUB12_ROWS;

	hub12_update_cache(data, data->current_row);
}

static void hub12_scan_work_handler(struct k_work *work)
{
	struct hub12_data *data = CONTAINER_OF(work, struct hub12_data, scan_work);
	const struct hub12_config *config = data->dev->config;

	hub12_scan_row(data, config);
}

static void hub12_scan_timer_handler(struct k_timer *timer)
{
	struct hub12_data *data = CONTAINER_OF(timer, struct hub12_data, scan_timer);

	k_work_submit(&data->scan_work);
}

static int hub12_write(const struct device *dev, const uint16_t x, const uint16_t y,
		       const struct display_buffer_descriptor *desc, const void *buf)
{
	struct hub12_data *data = dev->data;
	const struct hub12_config *config = dev->config;
	const uint8_t *src = buf;
	size_t fb_size = config->width * config->height / HUB12_PIXELS_PER_BYTE;

	if (x >= config->width || y >= config->height) {
		return -EINVAL;
	}

	if ((x + desc->width) > config->width || (y + desc->height) > config->height) {
		return -EINVAL;
	}

	if (desc->pitch != desc->width) {
		LOG_ERR("Unsupported pitch");
		return -ENOTSUP;
	}

	if (desc->buf_size < (desc->width * desc->height / HUB12_PIXELS_PER_BYTE)) {
		LOG_ERR("Buffer too small");
		return -EINVAL;
	}

	k_sem_take(&data->lock, K_FOREVER);

	if (x == 0 && y == 0 && desc->width == config->width && desc->height == config->height) {
		memcpy(data->framebuffer, src, fb_size);
	} else {
		/* Partial update */
		size_t src_pitch_bytes = desc->pitch / HUB12_PIXELS_PER_BYTE;
		size_t dest_pitch_bytes = config->width / HUB12_PIXELS_PER_BYTE;

		for (uint16_t j = 0; j < desc->height; j++) {
			uint16_t dest_y = y + j;

			for (uint16_t i = 0; i < desc->width; i++) {
				uint16_t dest_x = x + i;
				size_t src_byte_idx =
					(j * src_pitch_bytes) + (i / HUB12_PIXELS_PER_BYTE);
				uint8_t src_bit_mask = BIT(7 - (i % HUB12_PIXELS_PER_BYTE));
				bool bit_is_set = (src[src_byte_idx] & src_bit_mask);

				size_t dest_byte_idx = (dest_y * dest_pitch_bytes) +
						       (dest_x / HUB12_PIXELS_PER_BYTE);
				uint8_t dest_bit_mask = BIT(7 - (dest_x % HUB12_PIXELS_PER_BYTE));

				if (bit_is_set) {
					data->framebuffer[dest_byte_idx] |= dest_bit_mask;
				} else {
					data->framebuffer[dest_byte_idx] &= ~dest_bit_mask;
				}
			}
		}
	}

	for (int i = 0; i < HUB12_ROWS; i++) {
		hub12_update_cache(data, i);
	}

	k_sem_give(&data->lock);

	return 0;
}

static int hub12_read(const struct device *dev, const uint16_t x, const uint16_t y,
		      const struct display_buffer_descriptor *desc, void *buf)
{
	return -ENOTSUP;
}

static void *hub12_get_framebuffer(const struct device *dev)
{
	struct hub12_data *data = dev->data;

	return data->framebuffer;
}

static int hub12_blanking_off(const struct device *dev)
{
	return 0;
}

static int hub12_blanking_on(const struct device *dev)
{
	return 0;
}

static int hub12_set_brightness(const struct device *dev, const uint8_t brightness)
{
	struct hub12_data *data = dev->data;

	if (brightness == 0) {
		data->brightness_us = 0;
	} else {
		uint32_t range = HUB12_MAX_BRIGHTNESS - HUB12_MIN_BRIGHTNESS;

		data->brightness_us = HUB12_MIN_BRIGHTNESS + (uint8_t)((brightness * range) / 255U);
	}

	LOG_INF("Brightness set to %u us", data->brightness_us);

	return 0;
}

static int hub12_set_contrast(const struct device *dev, const uint8_t contrast)
{
	return -ENOTSUP;
}

static void hub12_get_capabilities(const struct device *dev, struct display_capabilities *caps)
{
	const struct hub12_config *config = dev->config;

	memset(caps, 0, sizeof(*caps));
	caps->x_resolution = config->width;
	caps->y_resolution = config->height;
	caps->supported_pixel_formats = PIXEL_FORMAT_MONO01;
	caps->current_pixel_format = PIXEL_FORMAT_MONO01;
	caps->screen_info = SCREEN_INFO_MONO_MSB_FIRST;
}

static int hub12_set_pixel_format(const struct device *dev, const enum display_pixel_format pf)
{
	if (pf == PIXEL_FORMAT_MONO01) {
		return 0;
	}

	return -ENOTSUP;
}

static int hub12_set_orientation(const struct device *dev,
				 const enum display_orientation orientation)
{
	if (orientation == DISPLAY_ORIENTATION_NORMAL) {
		return 0;
	}

	return -ENOTSUP;
}

static const struct display_driver_api hub12_api = {
	.blanking_on = hub12_blanking_on,
	.blanking_off = hub12_blanking_off,
	.write = hub12_write,
	.read = hub12_read,
	.get_framebuffer = hub12_get_framebuffer,
	.set_brightness = hub12_set_brightness,
	.set_contrast = hub12_set_contrast,
	.get_capabilities = hub12_get_capabilities,
	.set_pixel_format = hub12_set_pixel_format,
	.set_orientation = hub12_set_orientation,
};

static int hub12_init(const struct device *dev)
{
	struct hub12_data *data = dev->data;
	const struct hub12_config *config = dev->config;
	int ret;

	data->dev = dev;

	/* Only supporting single, unchained panels for now */
	if (config->width != 32 || config->height != 16) {
		LOG_ERR("Unsupported dimensions %dx%d. Only 32x16 panels supported", config->width,
			config->height);
		return -ENOTSUP;
	}

	if (!gpio_is_ready_dt(&config->pa) || !gpio_is_ready_dt(&config->pb) ||
	    !gpio_is_ready_dt(&config->pe) || !gpio_is_ready_dt(&config->plat)) {
		LOG_ERR("GPIO devices not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->pa, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_configure_dt(&config->pb, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_configure_dt(&config->pe, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_configure_dt(&config->plat, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		return ret;
	}

	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("SPI device not ready");
		return -ENODEV;
	}

	memset(data->framebuffer, 0, (config->width * config->height) / HUB12_PIXELS_PER_BYTE);
	memset(data->cache, 0, sizeof(data->cache));
	data->current_row = 0;
	data->brightness_us = HUB12_DEFAULT_BRIGHTNESS;

	ret = k_sem_init(&data->lock, 1, 1);
	if (ret < 0) {
		LOG_ERR("Failed to initialize semaphore");
		return ret;
	}

	for (int i = 0; i < HUB12_ROWS; i++) {
		hub12_update_cache(data, i);
	}

	k_work_init(&data->scan_work, hub12_scan_work_handler);
	k_timer_init(&data->scan_timer, hub12_scan_timer_handler, NULL);
	k_timer_start(&data->scan_timer, K_MSEC(1), K_MSEC(1));

	LOG_INF("HUB12 display initialized: %dx%d", config->width, config->height);

	return 0;
}

#define HUB12_INIT(inst)                                                                           \
	static uint8_t hub12_framebuffer_##inst[(DT_INST_PROP(inst, width) *                       \
						 DT_INST_PROP(inst, height)) /                     \
						HUB12_PIXELS_PER_BYTE];                            \
	static struct hub12_data hub12_data_##inst = {                                             \
		.framebuffer = hub12_framebuffer_##inst,                                           \
	};                                                                                         \
                                                                                                   \
	static const struct hub12_config hub12_config_##inst = {                                   \
		.pa = GPIO_DT_SPEC_INST_GET(inst, pa_gpios),                                       \
		.pb = GPIO_DT_SPEC_INST_GET(inst, pb_gpios),                                       \
		.pe = GPIO_DT_SPEC_INST_GET(inst, pe_gpios),                                       \
		.plat = GPIO_DT_SPEC_INST_GET(inst, plat_gpios),                                   \
		.spi = SPI_DT_SPEC_INST_GET(inst, SPI_OP_MODE_MASTER | SPI_WORD_SET(8)),           \
		.width = DT_INST_PROP(inst, width),                                                \
		.height = DT_INST_PROP(inst, height),                                              \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, hub12_init, NULL, &hub12_data_##inst, &hub12_config_##inst,    \
			      POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY, &hub12_api);

DT_INST_FOREACH_STATUS_OKAY(HUB12_INIT)
