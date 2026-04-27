/*
 * Copyright (c) 2026 Andrew Yong <me@ndoo.sg>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT himax_hx8353e

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/mipi_dbi.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(backlight_gpios)
#include <zephyr/drivers/gpio.h>
#endif
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(pwms)
#include <zephyr/drivers/pwm.h>
#endif

LOG_MODULE_REGISTER(hx8353e, CONFIG_DISPLAY_LOG_LEVEL);

/* Standard MIPI DCS commands (HX8353-E datasheet §8.1, §8.2) */
#define HX8353E_SLPOUT  0x11
#define HX8353E_INVOFF  0x20
#define HX8353E_INVON   0x21
#define HX8353E_DISPOFF 0x28
#define HX8353E_DISPON  0x29
#define HX8353E_CASET   0x2A
#define HX8353E_PASET   0x2B
#define HX8353E_RAMWR   0x2C
#define HX8353E_MADCTL  0x36
#define HX8353E_COLMOD  0x3A

/* MADCTL bits (HX8353-E datasheet §8.2.29) */
#define HX8353E_MADCTL_MY  BIT(7)
#define HX8353E_MADCTL_MX  BIT(6)
#define HX8353E_MADCTL_MV  BIT(5)

/* COLMOD: 16-bit/pixel RGB565 (HX8353-E datasheet §8.2.33) */
#define HX8353E_COLMOD_RGB565 0x05

/*
 * MADCTL values for Zephyr display orientations.
 * ROTATED_90 (MX|MV = 0x60) verified as landscape on the MD-UV390 PLUS panel.
 * Source: HX8353-E datasheet §8.2.29 MADCTL bit definitions.
 */
#define HX8353E_MADCTL_NORMAL      0x00
#define HX8353E_MADCTL_ROTATED_90  (HX8353E_MADCTL_MX | HX8353E_MADCTL_MV)
#define HX8353E_MADCTL_ROTATED_180 (HX8353E_MADCTL_MY | HX8353E_MADCTL_MX)
#define HX8353E_MADCTL_ROTATED_270 (HX8353E_MADCTL_MY | HX8353E_MADCTL_MV)

struct hx8353e_config {
	const struct device *mipi_dbi;
	struct mipi_dbi_config dbi_config;
	uint16_t width;
	uint16_t height;
	uint16_t rotation;
	bool bit_inversion;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(backlight_gpios)
	struct gpio_dt_spec backlight_gpio;
#endif
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(pwms)
	struct pwm_dt_spec backlight_pwm;
#endif
};

struct hx8353e_data {
	enum display_orientation orientation;
	bool blanking;
	bool last_window_valid;
	uint16_t last_x;
	uint16_t last_y;
	uint16_t last_w;
	uint16_t last_h;
	uint8_t brightness;
};

static int hx8353e_cmd(const struct device *dev, uint8_t cmd,
		       const uint8_t *data, size_t len)
{
	const struct hx8353e_config *cfg = dev->config;

	return mipi_dbi_command_write(cfg->mipi_dbi, &cfg->dbi_config,
				      cmd, data, len);
}

static void hx8353e_backlight_off(const struct device *dev)
{
	const struct hx8353e_config *cfg = dev->config;

	ARG_UNUSED(cfg);

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(pwms)
	if (cfg->backlight_pwm.dev != NULL) {
		pwm_set_pulse_dt(&cfg->backlight_pwm, 0);
	}
#endif
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(backlight_gpios)
	if (cfg->backlight_gpio.port != NULL) {
		gpio_pin_set_dt(&cfg->backlight_gpio, 0);
	}
#endif
}

static void hx8353e_backlight_restore(const struct device *dev)
{
	const struct hx8353e_config *cfg = dev->config;
	const struct hx8353e_data *data = dev->data;

	ARG_UNUSED(cfg);
	ARG_UNUSED(data);

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(backlight_gpios)
	if (cfg->backlight_gpio.port != NULL) {
		gpio_pin_set_dt(&cfg->backlight_gpio, 1);
	}
#endif
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(pwms)
	if (cfg->backlight_pwm.dev != NULL) {
		uint32_t pulse = (uint32_t)((uint64_t)cfg->backlight_pwm.period
					    * data->brightness / 255U);

		pwm_set_pulse_dt(&cfg->backlight_pwm, pulse);
	}
#endif
}

static int hx8353e_init_display(const struct device *dev)
{
	const struct hx8353e_config *cfg = dev->config;
	struct hx8353e_data *data = dev->data;
	int ret;

	ret = mipi_dbi_reset(cfg->mipi_dbi, 20);
	if (ret < 0 && ret != -ENOSYS) {
		return ret;
	}
	k_msleep(20);

	/* tSLOUT minimum 120 ms (HX8353-E datasheet §8.2.12 SLPOUT) */
	ret = hx8353e_cmd(dev, HX8353E_SLPOUT, NULL, 0);
	if (ret < 0) {
		return ret;
	}
	k_msleep(120);

	uint8_t madctl;

	switch (data->orientation) {
	case DISPLAY_ORIENTATION_ROTATED_90:
		madctl = HX8353E_MADCTL_ROTATED_90;
		break;
	case DISPLAY_ORIENTATION_ROTATED_180:
		madctl = HX8353E_MADCTL_ROTATED_180;
		break;
	case DISPLAY_ORIENTATION_ROTATED_270:
		madctl = HX8353E_MADCTL_ROTATED_270;
		break;
	default:
		madctl = HX8353E_MADCTL_NORMAL;
		break;
	}

	ret = hx8353e_cmd(dev, HX8353E_MADCTL, &madctl, 1);
	if (ret < 0) {
		return ret;
	}

	static const uint8_t colmod[] = {HX8353E_COLMOD_RGB565};

	ret = hx8353e_cmd(dev, HX8353E_COLMOD, colmod, sizeof(colmod));
	if (ret < 0) {
		return ret;
	}

	ret = hx8353e_cmd(dev,
			  cfg->bit_inversion ? HX8353E_INVON : HX8353E_INVOFF,
			  NULL, 0);
	if (ret < 0) {
		return ret;
	}

	/*
	 * Clear GRAM to black before display-on. Power-on GRAM content is
	 * undefined; this prevents a flash of random pixels at boot.
	 * Row window advances one line at a time so the zero buffer fits in a
	 * static BSS array.
	 */
	uint8_t caset[] = {0x00, 0x00, 0x00, (uint8_t)(cfg->width - 1)};

	ret = hx8353e_cmd(dev, HX8353E_CASET, caset, sizeof(caset));
	if (ret < 0) {
		return ret;
	}

	static const uint8_t zero_row[162 * 2];
	uint8_t paset_row[4] = {0x00, 0x00, 0x00, 0x00};

	for (uint16_t row = 0; row < cfg->height; row++) {
		paset_row[1] = (uint8_t)row;
		paset_row[3] = (uint8_t)row;
		ret = hx8353e_cmd(dev, HX8353E_PASET, paset_row, sizeof(paset_row));
		if (ret < 0) {
			return ret;
		}
		ret = mipi_dbi_command_write(cfg->mipi_dbi, &cfg->dbi_config,
					     HX8353E_RAMWR, zero_row,
					     cfg->width * 2);
		if (ret < 0) {
			return ret;
		}
	}

	ret = hx8353e_cmd(dev, HX8353E_DISPON, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	/*
	 * Wait one frame after DISPON before enabling the backlight. The gate
	 * driver needs one full frame scan (~17 ms at 60 Hz) to settle the LC
	 * layer to the black pixels written above; enabling the backlight sooner
	 * causes a brief white flash.
	 */
	k_msleep(20);

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(backlight_gpios)
	if (cfg->backlight_gpio.port != NULL) {
		ret = gpio_pin_configure_dt(&cfg->backlight_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			return ret;
		}
	}
#endif
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(pwms)
	if (cfg->backlight_pwm.dev != NULL) {
		ret = pwm_set_pulse_dt(&cfg->backlight_pwm,
				       cfg->backlight_pwm.period);
		if (ret < 0) {
			return ret;
		}
	}
#endif

	LOG_INF("init complete");
	return 0;
}

static int hx8353e_blanking_on(const struct device *dev)
{
	struct hx8353e_data *data = dev->data;

	hx8353e_backlight_off(dev);

	data->blanking = true;
	data->last_window_valid = false;
	return hx8353e_cmd(dev, HX8353E_DISPOFF, NULL, 0);
}

static int hx8353e_blanking_off(const struct device *dev)
{
	struct hx8353e_data *data = dev->data;
	int ret;

	data->blanking = false;
	data->last_window_valid = false;

	ret = hx8353e_cmd(dev, HX8353E_DISPON, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	hx8353e_backlight_restore(dev);
	return 0;
}

static int hx8353e_set_brightness(const struct device *dev, const uint8_t brightness)
{
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(pwms)
	const struct hx8353e_config *cfg = dev->config;
	struct hx8353e_data *data = dev->data;

	data->brightness = brightness;

	if (cfg->backlight_pwm.dev == NULL) {
		return -ENOTSUP;
	}

	uint32_t pulse = (uint32_t)((uint64_t)cfg->backlight_pwm.period * brightness / 255U);

	return pwm_set_pulse_dt(&cfg->backlight_pwm, pulse);
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(brightness);
	return -ENOTSUP;
#endif
}

static int hx8353e_write(const struct device *dev, const uint16_t x, const uint16_t y,
			 const struct display_buffer_descriptor *desc, const void *buf)
{
	const struct hx8353e_config *cfg = dev->config;
	struct hx8353e_data *data = dev->data;
	struct display_buffer_descriptor mipi_desc;
	uint16_t x_end = x + desc->width  - 1;
	uint16_t y_end = y + desc->height - 1;
	int ret;

	LOG_DBG("write x=%u y=%u w=%u h=%u buf_size=%u",
		x, y, desc->width, desc->height, desc->buf_size);

	bool need_window = !data->last_window_valid ||
			   data->last_x != x || data->last_y != y ||
			   data->last_w != desc->width || data->last_h != desc->height;

	if (need_window) {
		uint8_t caset[] = {
			(uint8_t)(x     >> 8), (uint8_t)x,
			(uint8_t)(x_end >> 8), (uint8_t)x_end
		};
		uint8_t paset[] = {
			(uint8_t)(y     >> 8), (uint8_t)y,
			(uint8_t)(y_end >> 8), (uint8_t)y_end
		};

		ret = hx8353e_cmd(dev, HX8353E_CASET, caset, sizeof(caset));
		if (ret < 0) {
			return ret;
		}
		ret = hx8353e_cmd(dev, HX8353E_PASET, paset, sizeof(paset));
		if (ret < 0) {
			return ret;
		}

		data->last_window_valid = true;
		data->last_x = x;
		data->last_y = y;
		data->last_w = desc->width;
		data->last_h = desc->height;
	}

	ret = hx8353e_cmd(dev, HX8353E_RAMWR, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	mipi_desc.width    = desc->width;
	mipi_desc.height   = desc->height;
	mipi_desc.pitch    = desc->width;
	mipi_desc.buf_size = desc->buf_size;

	return mipi_dbi_write_display(cfg->mipi_dbi, &cfg->dbi_config,
				      buf, &mipi_desc, PIXEL_FORMAT_RGB_565X);
}

static void hx8353e_get_capabilities(const struct device *dev,
				     struct display_capabilities *caps)
{
	const struct hx8353e_config *cfg = dev->config;
	const struct hx8353e_data *data = dev->data;

	memset(caps, 0, sizeof(*caps));
	caps->x_resolution         = cfg->width;
	caps->y_resolution         = cfg->height;
	/*
	 * The HX8353E 8-bit parallel bus (Table 5.5 in datasheet) expects the
	 * high byte of each RGB565 pixel first, then the low byte — big-endian
	 * on the wire. RGB_565X signals this byte order to callers so they
	 * store pixels with the high byte at the lower address.
	 */
	caps->supported_pixel_formats = PIXEL_FORMAT_RGB_565X;
	caps->current_pixel_format    = PIXEL_FORMAT_RGB_565X;
	caps->current_orientation     = data->orientation;
}

static int hx8353e_set_pixel_format(const struct device *dev,
				    const enum display_pixel_format pf)
{
	ARG_UNUSED(dev);

	if (pf == PIXEL_FORMAT_RGB_565X) {
		return 0;
	}
	return -ENOTSUP;
}

static int hx8353e_set_orientation(const struct device *dev,
				   const enum display_orientation orientation)
{
	struct hx8353e_data *data = dev->data;
	uint8_t madctl;
	int ret;

	switch (orientation) {
	case DISPLAY_ORIENTATION_ROTATED_90:
		madctl = HX8353E_MADCTL_ROTATED_90;
		break;
	case DISPLAY_ORIENTATION_ROTATED_180:
		madctl = HX8353E_MADCTL_ROTATED_180;
		break;
	case DISPLAY_ORIENTATION_ROTATED_270:
		madctl = HX8353E_MADCTL_ROTATED_270;
		break;
	default:
		madctl = HX8353E_MADCTL_NORMAL;
		break;
	}

	ret = hx8353e_cmd(dev, HX8353E_MADCTL, &madctl, 1);
	if (ret < 0) {
		return ret;
	}
	data->orientation = orientation;
	data->last_window_valid = false;
	return 0;
}

DEVICE_API(display, hx8353e_driver_api) = {
	.blanking_on      = hx8353e_blanking_on,
	.blanking_off     = hx8353e_blanking_off,
	.write            = hx8353e_write,
	.get_capabilities = hx8353e_get_capabilities,
	.set_pixel_format = hx8353e_set_pixel_format,
	.set_orientation  = hx8353e_set_orientation,
	.set_brightness   = hx8353e_set_brightness,
};

static int hx8353e_init(const struct device *dev)
{
	const struct hx8353e_config *cfg = dev->config;
	struct hx8353e_data *data = dev->data;

	if (!device_is_ready(cfg->mipi_dbi)) {
		LOG_ERR("MIPI DBI device not ready");
		return -ENODEV;
	}

	switch (cfg->rotation) {
	case 90:
		data->orientation = DISPLAY_ORIENTATION_ROTATED_90;
		break;
	case 180:
		data->orientation = DISPLAY_ORIENTATION_ROTATED_180;
		break;
	case 270:
		data->orientation = DISPLAY_ORIENTATION_ROTATED_270;
		break;
	default:
		data->orientation = DISPLAY_ORIENTATION_NORMAL;
		break;
	}
	data->last_window_valid = false;

	return hx8353e_init_display(dev);
}

#define HX8353E_INIT(n)								\
	static const struct hx8353e_config hx8353e_config_##n = {		\
		.mipi_dbi      = DEVICE_DT_GET(DT_INST_PARENT(n)),		\
		.dbi_config    = MIPI_DBI_CONFIG_DT_INST(n,			\
				 SPI_OP_MODE_MASTER | SPI_WORD_SET(8), 0),	\
		.width         = DT_INST_PROP(n, width),			\
		.height        = DT_INST_PROP(n, height),			\
		.rotation      = DT_INST_PROP(n, rotation),			\
		.bit_inversion = DT_INST_PROP(n, bit_inversion),		\
		IF_ENABLED(DT_ANY_INST_HAS_PROP_STATUS_OKAY(backlight_gpios),	\
			(.backlight_gpio =					\
			 GPIO_DT_SPEC_INST_GET_OR(n, backlight_gpios, {0}),))	\
		IF_ENABLED(DT_ANY_INST_HAS_PROP_STATUS_OKAY(pwms),		\
			(.backlight_pwm =					\
			 PWM_DT_SPEC_INST_GET_OR(n, pwms, {0}),))		\
	};									\
										\
	static struct hx8353e_data hx8353e_data_##n = {				\
		.brightness = 255,						\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n, hx8353e_init, NULL,				\
			      &hx8353e_data_##n, &hx8353e_config_##n,		\
			      POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY,	\
			      &hx8353e_driver_api);

DT_INST_FOREACH_STATUS_OKAY(HX8353E_INIT)
