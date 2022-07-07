/*
 * Copyright (c) 2021, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/display.h>
#include <string.h>

#define PIXEL_BIT(idx, val)  (val ? BIT(idx) : 0)
#define PIXEL_MASK(...)      (FOR_EACH_IDX(PIXEL_BIT, (|), __VA_ARGS__))

static struct display_capabilities caps;
static uint8_t buf[5];
static const struct display_buffer_descriptor buf_desc = {
	.buf_size = sizeof(buf),
	.width    = 5,
	.height   = 5,
	.pitch    = 8,
};

static void update_block_3x3(const struct device *dev)
{
	int ret;
	static const uint8_t small_bufs[][2] = {
		{ PIXEL_MASK(0, 1, 0) |
		  PIXEL_MASK(0, 1, 0) << 4,
		  PIXEL_MASK(0, 1, 0) },
		{ PIXEL_MASK(0, 0, 1) |
		  PIXEL_MASK(0, 1, 0) << 4,
		  PIXEL_MASK(1, 0, 0) },
		{ PIXEL_MASK(0, 0, 0) |
		  PIXEL_MASK(1, 1, 1) << 4,
		  PIXEL_MASK(0, 0, 0) },
		{ PIXEL_MASK(1, 0, 0) |
		  PIXEL_MASK(0, 1, 0) << 4,
		  PIXEL_MASK(0, 0, 1) },
	};
	static const struct display_buffer_descriptor small_buf_desc = {
		.buf_size = sizeof(small_bufs[0]),
		.width    = 3,
		.height   = 3,
		.pitch    = 4,
	};

	for (int i = 0; i < 4 * ARRAY_SIZE(small_bufs); ++i) {
		k_sleep(K_MSEC(100));

		ret = display_write(dev, 1, 1,
				    &small_buf_desc, small_bufs[i % 4]);
		if (ret < 0) {
			printk("display_write failed: %u/%d\n",
				__LINE__, ret);
		}
	}
}

static void update_block_5x5(const struct device *dev)
{
	int ret;

	buf[0] = PIXEL_MASK(0, 0, 0, 0, 1);
	buf[1] = PIXEL_MASK(0, 0, 0, 1, 1);
	buf[2] = PIXEL_MASK(0, 0, 1, 1, 1);
	buf[3] = PIXEL_MASK(0, 1, 1, 1, 1);
	buf[4] = PIXEL_MASK(1, 1, 1, 1, 1);
	ret = display_write(dev, 0, 0, &buf_desc, buf);
	if (ret < 0) {
		printk("display_write failed: %u/%d\n",
			__LINE__, ret);
	}

	for (int i = 0; i < 4 * 5; ++i) {
		uint8_t tmp;

		k_sleep(K_MSEC(100));

		tmp = buf[0];
		buf[0] = buf[1];
		buf[1] = buf[2];
		buf[2] = buf[3];
		buf[3] = buf[4];
		buf[4] = tmp;
		ret = display_write(dev, 0, 0, &buf_desc, buf);
		if (ret < 0) {
			printk("display_write failed: %u/%d\n",
				__LINE__, ret);
		}
	}
}

static void show_all_brightness_levels(const struct device *dev)
{
	int ret;
	uint8_t brightness = 0;

	do {
		--brightness;
		ret = display_set_brightness(dev, brightness);
		if (ret < 0) {
			printk("display_set_brightness failed: %u/%d\n",
				__LINE__, ret);
		}

		k_sleep(K_MSEC(5));
	} while (brightness);
}

static void update_through_framebuffer(const struct device *dev)
{
	uint8_t *framebuf = display_get_framebuffer(dev);
	uint8_t dimmed = 0;
	bool inc = false;
	enum {
		MIN_BRIGHTNESS = 0x0F,
		BLOCK_SIZE     = 5,
		STEPS          = BLOCK_SIZE - 1,
		RUNS           = 4
	};

	if (!framebuf) {
		printk("frame buffer not available\n");
		return;
	}

	/* Clear screen. */
	memset(framebuf, 0, caps.x_resolution * caps.y_resolution);

	for (int i = 0; i < 1 + 3 * RUNS * STEPS; ++i) {
		k_sleep(K_MSEC(100));

		for (uint8_t column = 0; column < BLOCK_SIZE; ++column) {
			for (uint8_t row = 0; row < BLOCK_SIZE; ++row) {
				uint8_t diff;
				uint8_t step = (0xFF - MIN_BRIGHTNESS) / STEPS;

				/*
				 * Depending on the iteration, use different
				 * pixel values for:
				 * - vertical lines
				 */
				if (i < 1 * RUNS * STEPS) {
					diff = dimmed > column
					     ? dimmed - column
					     : column - dimmed;
				/*
				 * - horizontal lines
				 */
				} else if (i < 2 * RUNS * STEPS) {
					diff = dimmed > row
					     ? dimmed - row
					     : row - dimmed;
				/*
				 * - diagonal lines
				 */
				} else {
					uint8_t dist = column + row;

					diff = 2 * dimmed > dist
					     ? 2 * dimmed - dist
					     : dist - 2 * dimmed;
					step /= 2;
				}

				framebuf[column + row * caps.x_resolution] =
					MIN_BRIGHTNESS + diff * step;
			}
		}

		if (dimmed == 0 || dimmed == STEPS) {
			inc = !inc;
		}
		if (inc) {
			++dimmed;
		} else {
			--dimmed;
		}
	}
}

void main(void)
{
	printk("nRF LED matrix sample on %s\n", CONFIG_BOARD);

	int ret;
	const struct device *dev = DEVICE_DT_GET_ONE(nordic_nrf_led_matrix);

	if (!dev) {
		printk("Display device not ready\n");
		return;
	}

	display_get_capabilities(dev, &caps);
	if (!(caps.supported_pixel_formats & PIXEL_FORMAT_MONO01)) {
		printk("Expected pixel format not supported\n");
		return;
	}

	ret = display_set_pixel_format(dev, PIXEL_FORMAT_MONO01);
	if (ret < 0) {
		printk("display_set_pixel_format failed: %u/%d\n",
			__LINE__, ret);
	}

	printk("Started\n");

	for (;;) {
		ret = display_set_brightness(dev, 0x7F);
		if (ret < 0) {
			printk("display_set_brightness failed: %u/%d\n",
				__LINE__, ret);
		}

		buf[0] = PIXEL_MASK(1, 0, 1, 0, 1);
		buf[1] = PIXEL_MASK(1, 1, 0, 0, 1);
		buf[2] = PIXEL_MASK(1, 0, 1, 0, 1);
		buf[3] = PIXEL_MASK(1, 0, 0, 1, 1);
		buf[4] = PIXEL_MASK(1, 0, 1, 0, 1);
		ret = display_write(dev, 0, 0, &buf_desc, buf);
		if (ret < 0) {
			printk("display_write failed: %u/%d\n",
				__LINE__, ret);
		}

		ret = display_blanking_off(dev);
		if (ret < 0) {
			printk("display_blanking_off failed: %u/%d\n",
				__LINE__, ret);
		}

		k_sleep(K_MSEC(500));

		update_block_3x3(dev);

		k_sleep(K_MSEC(500));

		update_block_5x5(dev);

		k_sleep(K_MSEC(200));

		ret = display_blanking_on(dev);
		if (ret < 0) {
			printk("display_blanking_on failed: %u/%d\n",
				__LINE__, ret);
		}

		k_sleep(K_MSEC(500));

		ret = display_blanking_off(dev);
		if (ret < 0) {
			printk("display_blanking_off failed: %u/%d\n",
				__LINE__, ret);
		}

		k_sleep(K_MSEC(500));

		show_all_brightness_levels(dev);

		update_through_framebuffer(dev);

		k_sleep(K_MSEC(500));
	}
}
