/*
 * Copyright (c) 2024-2025 Chen Xingyu <hi@xingrz.me>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT gpio_7_segment

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/auxdisplay.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(auxdisplay_gpio_7seg, CONFIG_AUXDISPLAY_LOG_LEVEL);

static const uint8_t DIGITS[] = {
	[0] = BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(4) | BIT(5),
	[1] = BIT(1) | BIT(2),
	[2] = BIT(0) | BIT(1) | BIT(3) | BIT(4) | BIT(6),
	[3] = BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(6),
	[4] = BIT(1) | BIT(2) | BIT(5) | BIT(6),
	[5] = BIT(0) | BIT(2) | BIT(3) | BIT(5) | BIT(6),
	[6] = BIT(0) | BIT(2) | BIT(3) | BIT(4) | BIT(5) | BIT(6),
	[7] = BIT(0) | BIT(1) | BIT(2),
	[8] = BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(4) | BIT(5) | BIT(6),
	[9] = BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(5) | BIT(6),
};

#define DP    (BIT(7))
#define BLANK (0x00)

struct auxdisplay_gpio_7seg_data {
	struct k_timer timer;
	uint32_t refresh_pos;
	int16_t cursor_x;
	int16_t cursor_y;
};

struct auxdisplay_gpio_7seg_config {
	struct auxdisplay_capabilities capabilities;
	const struct gpio_dt_spec *segment_gpios;
	const struct gpio_dt_spec *digit_gpios;
	uint32_t segment_count;
	uint32_t digit_count;
	uint32_t refresh_period_ms;
	uint8_t *buffer;
};

static int auxdisplay_gpio_7seg_display_on(const struct device *dev)
{
	const struct auxdisplay_gpio_7seg_config *cfg = dev->config;
	struct auxdisplay_gpio_7seg_data *data = dev->data;

	data->refresh_pos = 0;
	k_timer_start(&data->timer, K_NO_WAIT, K_MSEC(cfg->refresh_period_ms));

	return 0;
}

static int auxdisplay_gpio_7seg_display_off(const struct device *dev)
{
	struct auxdisplay_gpio_7seg_data *data = dev->data;

	k_timer_stop(&data->timer);

	return 0;
}

static int auxdisplay_gpio_7seg_cursor_position_set(const struct device *dev,
						    enum auxdisplay_position type, int16_t x,
						    int16_t y)
{
	const struct auxdisplay_gpio_7seg_config *cfg = dev->config;
	struct auxdisplay_gpio_7seg_data *data = dev->data;

	if (type == AUXDISPLAY_POSITION_RELATIVE) {
		x += data->cursor_x;
		y += data->cursor_y;
	} else if (type == AUXDISPLAY_POSITION_RELATIVE_DIRECTION) {
		return -EINVAL;
	}

	if (x < 0 || y < 0) {
		return -EINVAL;
	} else if (x >= cfg->capabilities.columns || y >= cfg->capabilities.rows) {
		return -EINVAL;
	}

	data->cursor_x = x;
	data->cursor_y = y;

	return 0;
}

static int auxdisplay_gpio_7seg_cursor_position_get(const struct device *dev, int16_t *x,
						    int16_t *y)
{
	struct auxdisplay_gpio_7seg_data *data = dev->data;

	*x = data->cursor_x;
	*y = data->cursor_y;

	return 0;
}

static int auxdisplay_gpio_7seg_capabilities_get(const struct device *dev,
						 struct auxdisplay_capabilities *capabilities)
{
	const struct auxdisplay_gpio_7seg_config *cfg = dev->config;

	memcpy(capabilities, &cfg->capabilities, sizeof(struct auxdisplay_capabilities));

	return 0;
}

static int auxdisplay_gpio_7seg_clear(const struct device *dev)
{
	const struct auxdisplay_gpio_7seg_config *cfg = dev->config;
	struct auxdisplay_gpio_7seg_data *data = dev->data;

	memset(cfg->buffer, 0, cfg->digit_count);
	data->refresh_pos = 0;
	data->cursor_x = 0;
	data->cursor_y = 0;

	return 0;
}

static int auxdisplay_gpio_7seg_write(const struct device *dev, const uint8_t *ch, uint16_t len)
{
	const struct auxdisplay_gpio_7seg_config *cfg = dev->config;
	struct auxdisplay_gpio_7seg_data *data = dev->data;
	uint32_t i, cursor;

	for (i = 0; i < len; i++) {
		cursor = data->cursor_y * cfg->capabilities.columns + data->cursor_x;

		/*
		 * Special case where the decimal point should be added to the
		 * previous digit
		 */
		if (ch[i] == '.' && cursor > 0) {
			cfg->buffer[cursor - 1] |= DP;
			continue;
		}

		if (cursor >= cfg->digit_count) {
			break;
		}

		if (ch[i] >= '0' && ch[i] <= '9') {
			cfg->buffer[cursor] = DIGITS[ch[i] - '0'];
		} else if (ch[i] == '.') {
			/* Leading dot, leave the digit blank */
			cfg->buffer[cursor] = DP;
		} else {
			cfg->buffer[cursor] = BLANK;
		}

		/* Move the cursor */
		if (data->cursor_x < cfg->capabilities.columns - 1) {
			data->cursor_x++;
		} else if (data->cursor_y < cfg->capabilities.rows - 1) {
			data->cursor_x = 0;
			data->cursor_y++;
		}
	}

	/* Reset the refresh position */
	data->refresh_pos = 0;

	return 0;
}

static void auxdisplay_gpio_7seg_timer_expiry_fn(struct k_timer *timer)
{
	const struct device *dev = k_timer_user_data_get(timer);
	const struct auxdisplay_gpio_7seg_config *cfg = dev->config;
	struct auxdisplay_gpio_7seg_data *data = dev->data;

	/* Turn off the current digit and move to the next one */
	gpio_pin_set_dt(&cfg->digit_gpios[data->refresh_pos], 0);
	data->refresh_pos = (data->refresh_pos + 1) % cfg->digit_count;

	/* Set the segments for the new digit */
	for (uint32_t i = 0; i < cfg->segment_count; i++) {
		gpio_pin_set_dt(&cfg->segment_gpios[i], cfg->buffer[data->refresh_pos] & BIT(i));
	}

	/* Turn on the new digit */
	gpio_pin_set_dt(&cfg->digit_gpios[data->refresh_pos], 1);
}

static void auxdisplay_gpio_7seg_timer_stop_fn(struct k_timer *timer)
{
	const struct device *dev = k_timer_user_data_get(timer);
	const struct auxdisplay_gpio_7seg_config *cfg = dev->config;

	/* Turn off all digits */
	for (uint32_t i = 0; i < cfg->digit_count; i++) {
		gpio_pin_set_dt(&cfg->digit_gpios[i], 0);
	}
}

static int auxdisplay_gpio_7seg_init(const struct device *dev)
{
	const struct auxdisplay_gpio_7seg_config *cfg = dev->config;
	struct auxdisplay_gpio_7seg_data *data = dev->data;

	for (uint32_t i = 0; i < cfg->segment_count; i++) {
		gpio_pin_configure_dt(&cfg->segment_gpios[i], GPIO_OUTPUT_INACTIVE);
	}

	for (uint32_t i = 0; i < cfg->digit_count; i++) {
		gpio_pin_configure_dt(&cfg->digit_gpios[i], GPIO_OUTPUT_INACTIVE);
	}

	k_timer_init(&data->timer, auxdisplay_gpio_7seg_timer_expiry_fn,
		     auxdisplay_gpio_7seg_timer_stop_fn);
	k_timer_user_data_set(&data->timer, (void *)dev);

	auxdisplay_gpio_7seg_display_on(dev);

	return 0;
}

static DEVICE_API(auxdisplay, auxdisplay_gpio_7seg_api) = {
	.display_on = auxdisplay_gpio_7seg_display_on,
	.display_off = auxdisplay_gpio_7seg_display_off,
	.cursor_position_set = auxdisplay_gpio_7seg_cursor_position_set,
	.cursor_position_get = auxdisplay_gpio_7seg_cursor_position_get,
	.capabilities_get = auxdisplay_gpio_7seg_capabilities_get,
	.clear = auxdisplay_gpio_7seg_clear,
	.write = auxdisplay_gpio_7seg_write,
};

#define AUXDISPLAY_GPIO_7SEG_INST(n)                                                               \
	static struct auxdisplay_gpio_7seg_data auxdisplay_gpio_7seg_data_##n;                     \
                                                                                                   \
	static const struct gpio_dt_spec auxdisplay_gpio_7seg_segment_gpios_##n[] = {              \
		DT_INST_FOREACH_PROP_ELEM_SEP(n, segment_gpios, GPIO_DT_SPEC_GET_BY_IDX, (,))};    \
                                                                                                   \
	static const struct gpio_dt_spec auxdisplay_gpio_7seg_digit_gpios_##n[] = {                \
		DT_INST_FOREACH_PROP_ELEM_SEP(n, digit_gpios, GPIO_DT_SPEC_GET_BY_IDX, (,))};      \
                                                                                                   \
	static uint8_t auxdisplay_gpio_7seg_buffer_##n[DT_INST_PROP_LEN(n, digit_gpios)];          \
                                                                                                   \
	static const struct auxdisplay_gpio_7seg_config auxdisplay_gpio_7seg_config_##n = {        \
		.capabilities =                                                                    \
			{                                                                          \
				.columns = DT_INST_PROP(n, columns),                               \
				.rows = DT_INST_PROP(n, rows),                                     \
			},                                                                         \
		.segment_gpios = auxdisplay_gpio_7seg_segment_gpios_##n,                           \
		.segment_count = DT_INST_PROP_LEN(n, segment_gpios),                               \
		.digit_gpios = auxdisplay_gpio_7seg_digit_gpios_##n,                               \
		.digit_count = DT_INST_PROP_LEN(n, digit_gpios),                                   \
		.refresh_period_ms = DT_INST_PROP(n, refresh_period_ms),                           \
		.buffer = auxdisplay_gpio_7seg_buffer_##n,                                         \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, auxdisplay_gpio_7seg_init, NULL, &auxdisplay_gpio_7seg_data_##n,  \
			      &auxdisplay_gpio_7seg_config_##n, POST_KERNEL,                       \
			      CONFIG_AUXDISPLAY_INIT_PRIORITY, &auxdisplay_gpio_7seg_api);

DT_INST_FOREACH_STATUS_OKAY(AUXDISPLAY_GPIO_7SEG_INST)
