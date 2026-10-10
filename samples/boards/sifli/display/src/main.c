/*
 * Copyright (c) 2026 Qingsong Gou <gouqs@hotmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Sample application to display random colors on a JDI RGB222 display.
 *
 * This application fills the entire JDI Memory LCD with a random solid
 * RGB222 color every 1 second.
 *
 * JDI parallel interface uses RGB222 format: 2 bits Red, 2 bits Green,
 * 2 bits Blue per pixel (6 bits total). Each pixel is stored in one byte
 * with the 6-bit color value in the lower bits: [unused|RR|GG|BB].
 */

#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

static const struct pwm_dt_spec pwm_backlight = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led0));

#define RGB222(r, g, b) (((r) << 4) | ((g) << 2) | (b))
#define RGB332(r, g, b) (((r) << 5) | ((g) << 2) | (b))

#define DISPLAY_WIDTH  260
#define DISPLAY_HEIGHT 260

/* Framebuffer: 1 byte per pixel (RGB222 in lower 6 bits) */
static uint8_t framebuffer[DISPLAY_WIDTH * DISPLAY_HEIGHT];

int main(void)
{
	const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	struct display_capabilities caps;
	uint8_t i = 0;
	int ret;

	if (!device_is_ready(dev)) {
		LOG_ERR("Display device not ready");
		return -ENODEV;
	}

	display_get_capabilities(dev, &caps);

	uint16_t width = caps.x_resolution;
	uint16_t height = caps.y_resolution;
	uint16_t buf_size = width * height;

	LOG_INF("Display: %dx%d RGB222, buffer size: %d bytes", width, height, buf_size);

	struct display_buffer_descriptor desc = {
		.width = width,
		.height = height,
		.pitch = width,
		.buf_size = buf_size,
	};

	if (!pwm_is_ready_dt(&pwm_backlight)) {
		LOG_ERR("PWM device %s is not ready", pwm_backlight.dev->name);
	} else {
		ret = pwm_set_dt(&pwm_backlight, pwm_backlight.period, pwm_backlight.period / 2U);
		if (ret) {
			LOG_ERR("Failed to set backlight PWM: %d", ret);
		} else {
			LOG_INF("Backlight set to 50%% (period=%u nsec, pulse=%u nsec)",
				pwm_backlight.period, pwm_backlight.period / 2U);
		}
	}

	display_blanking_off(dev);

	while (1) {
		uint8_t r = RGB332(6, 0, 0) & 0xFF;
		uint8_t g = RGB332(0, 6, 0) & 0xFF;
		uint8_t b = RGB332(0, 0, 3) & 0xFF;

		uint8_t colors[3] = {r, g, b};

		memset(framebuffer, colors[i], buf_size);
		sys_cache_data_flush_range(framebuffer, buf_size);
		i = (i + 1) % 3;

		ret = display_write(dev, 0, 0, &desc, framebuffer);
		if (ret < 0) {
			LOG_ERR("display_write failed: %d", ret);
		}

		LOG_INF("Color: R=%u G=%u B=%u (0x%02x)", r, g, b, colors[i]);
		k_sleep(K_SECONDS(1));
	}

	return 0;
}
