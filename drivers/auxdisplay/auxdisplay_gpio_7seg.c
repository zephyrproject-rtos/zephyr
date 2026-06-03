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

#ifdef CONFIG_AUXDISPLAY_GPIO_7SEG_USE_16BIT_FONT
typedef uint16_t font_char_t;
#define DP (BIT(15))
#else
typedef uint8_t font_char_t;
#define DP (BIT(7))
#endif

#ifdef CONFIG_AUXDISPLAY_GPIO_7SEG_INTERNAL_FONT_DIGITS7
static const font_char_t DIGITS[] = {
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
#endif
#ifdef CONFIG_AUXDISPLAY_GPIO_7SEG_INTERNAL_FONT_DIGITS15
static const font_char_t DIGITS[] = {
	[0] = BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(4) | BIT(5),
	[1] = BIT(1) | BIT(2),
	[2] = BIT(0) | BIT(1) | BIT(3) | BIT(4) | BIT(9) | BIT(13),
	[3] = BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(9) | BIT(13),
	[4] = BIT(1) | BIT(2) | BIT(5) | BIT(9) | BIT(13),
	[5] = BIT(0) | BIT(2) | BIT(3) | BIT(5) | BIT(9) | BIT(13),
	[6] = BIT(0) | BIT(2) | BIT(3) | BIT(4) | BIT(5) | BIT(9) | BIT(13),
	[7] = BIT(0) | BIT(1) | BIT(2),
	[8] = BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(4) | BIT(5) | BIT(9) | BIT(13),
	[9] = BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(5) | BIT(9) | BIT(13),
};
#endif
#ifdef CONFIG_AUXDISPLAY_GPIO_7SEG_INTERNAL_FONT_ALPHA7
static const font_char_t ALPHA[] = {
	/* SP  !     "     #     $     %     &     '  */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* (   )     *     +     ,     -     .     /  */
	0x39, 0x0F, 0x00, 0x00, 0x00, 0x40, 0x80, 0x52,
	/* 0   1     2     3     4     5     6     7  */
	0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07,
	/* 8   9     :     ;     <     =     >     ?  */
	0x7F, 0x67, 0x00, 0x00, 0x00, 0x48, 0x00, 0x00,
	/* 64  A     B     C     D     E     F     G  */
	0x00, 0x77, 0x7F, 0x39, 0x5E, 0x79, 0x71, 0x3D,
	/* H   I     J     K     L     M     N     O  */
	0x76, 0x06, 0x0E, 0x00, 0x38, 0x00, 0x37, 0x3F,
	/* P   Q     R     S     T     U     V     W  */
	0x73, 0x67, 0x50, 0x6D, 0x78, 0x3E, 0x00, 0x00,
	/* X   Y     Z     [     \     ]     ^     _  */
	0x00, 0x6E, 0x00, 0x39, 0x64, 0x0F, 0x00, 0x08,
	/* 96  a     b     c     d     e     f     g  */
	0x63, 0x5F, 0x7C, 0x58, 0x5E, 0x7B, 0x71, 0x6F,
	/* h   i     j     k     l     m     n     o  */
	0x74, 0x04, 0x0E, 0x00, 0x18, 0x00, 0x54, 0x5C,
	/* p   q     r     s     t     u     v     w  */
	0x73, 0x67, 0x50, 0x6D, 0x78, 0x1C, 0x00, 0x00,
	/* x   y     z     {     |     }     ~    DEL */
	0x00, 0x6E, 0x00, 0x39, 0x30, 0x0F, 0x22, 0x01,
};
#endif
#ifdef CONFIG_AUXDISPLAY_GPIO_7SEG_INTERNAL_FONT_ALPHA15
static const font_char_t ALPHA[] = {
	/* SP    !       "       #(all)  $       %       &       '   */
	0x0000, 0x0080, 0x0082, 0x3FFF, 0x2AAD, 0x1124, 0x2B1A, 0x0002,
	/* (     )       *       +       ,       -       .       /   */
	0x0039, 0x000F, 0x3FC0, 0x2A80, 0x0400, 0x2200, 0x4000, 0x1100,
	/* 0     1       2       3       4       5       6       7   */
	0x003F, 0x0006, 0x221B, 0x220F, 0x2226, 0x222D, 0x223D, 0x0007,
	/* 8     9       :       ;       <       =       >       ?   */
	0x223F, 0x2227, 0x0880, 0x1080, 0x0500, 0x2208, 0x1040, 0x0A03,
	/* @     A       B       C       D       E       F       G   */
	0x02BB, 0x2237, 0x2539, 0x0039, 0x1070, 0x2039, 0x2231, 0x023D,
	/* H     I       J       K       L       M       N       O   */
	0x2236, 0x0889, 0x001E, 0x2530, 0x0038, 0x0176, 0x0476, 0x003F,
	/* P     Q       R       S       T       U       V       W   */
	0x2233, 0x043F, 0x2633, 0x222D, 0x0881, 0x003E, 0x0446, 0x1436,
	/* X     Y       Z       [       \       ]       ^(dlta) _   */
	0x1540, 0x0940, 0x1109, 0x0039, 0x0440, 0x000F, 0x1408, 0x0008,
	/* `     a       b       c       d       e       f       g   */
	0x2223, 0x120F, 0x223C, 0x2218, 0x221E, 0x223B, 0x2A41, 0x222F,
	/* h     i       j       k       l       m       n       o   */
	0x2430, 0x0800, 0x100E, 0x0D80, 0x0880, 0x2A14, 0x2214, 0x221C,
	/* p     q       r       s       t       u       v       w   */
	0x2233, 0x0247, 0x2010, 0x024D, 0x2038, 0x001C, 0x0404, 0x081C,
	/* x     y       z       {       |       }       ~       UP- */
	0x1540, 0x1140, 0x1109, 0x2500, 0x0880, 0x1240, 0x0160, 0x0001,
};
#endif

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
	font_char_t *buffer;
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

	memset(cfg->buffer, 0, cfg->digit_count * sizeof(font_char_t));
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

		if (ch[i] == '.') {
			/* Leading dot, leave the digit blank */
			cfg->buffer[cursor] = DP;
		} else {
#if defined(CONFIG_AUXDISPLAY_GPIO_7SEG_INTERNAL_FONT_ALPHA7) || \
	defined(CONFIG_AUXDISPLAY_GPIO_7SEG_INTERNAL_FONT_ALPHA15)
			if (ch[i] >= ' ' && ch[i] <= '~') {
				cfg->buffer[cursor] = ALPHA[ch[i] - ' '];
#else
			if (ch[i] >= '0' && ch[i] <= '9') {
				cfg->buffer[cursor] = DIGITS[ch[i] - '0'];
#endif
			} else {
				cfg->buffer[cursor] = BLANK;
			}
		}

		/* Move the cursor */
		if (data->cursor_x < cfg->capabilities.columns - 1) {
			data->cursor_x++;
		} else if (data->cursor_y < cfg->capabilities.rows - 1) {
			data->cursor_x = 0;
			data->cursor_y++;
		}
	}

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
	struct auxdisplay_gpio_7seg_data *data = dev->data;

	/* Turn off all digits */
	for (uint32_t i = 0; i < cfg->digit_count; i++) {
		gpio_pin_set_dt(&cfg->digit_gpios[i], 0);
	}

	/* Reset the refresh position */
	data->refresh_pos = 0;
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
	static font_char_t auxdisplay_gpio_7seg_buffer_##n[DT_INST_PROP_LEN(n, digit_gpios)];      \
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
