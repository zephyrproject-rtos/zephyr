/*
 * Copyright (c) 2026 Qingsong Gou <gouqs@hotmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT jdi_parallel

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/jdi.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(display_jdi_parallel, CONFIG_DISPLAY_LOG_LEVEL);

struct jdi_parallel_config {
	const struct device *jdi_bus;
	struct jdi_config jdi_cfg;
	struct gpio_dt_spec vlcd_gpio;
	struct gpio_dt_spec vddp_gpio;
	struct pwm_dt_spec vcom_pwm;
	struct pwm_dt_spec vcom_pwm_inv;
	uint16_t power_seq_delay_ms;
	uint16_t width;
	uint16_t height;
};

static int jdi_parallel_write_data(const struct device *dev, uint16_t x, uint16_t y, uint16_t width,
				   uint16_t height, const uint8_t *data, size_t len,
				   const struct jdi_config *jdi_cfg)
{
	const struct jdi_parallel_config *config = dev->config;

	return jdi_write_data(config->jdi_bus, x, y, width, height, data, len, jdi_cfg);
}

static int jdi_parallel_blanking_on(const struct device *dev)
{
	ARG_UNUSED(dev);

	LOG_DBG("Turning display off");

	return 0;
}

static int jdi_parallel_blanking_off(const struct device *dev)
{
	const struct jdi_parallel_config *config = dev->config;
	int ret;

	LOG_DBG("Turning display on");

	if (config->vlcd_gpio.port) {
		if (device_is_ready(config->vlcd_gpio.port)) {
			gpio_pin_set_dt(&config->vlcd_gpio, 1);
			k_msleep(config->power_seq_delay_ms);
		}
	}

	if (config->vddp_gpio.port) {
		if (device_is_ready(config->vddp_gpio.port)) {
			gpio_pin_set_dt(&config->vddp_gpio, 1);
			k_msleep(config->power_seq_delay_ms);
		}
	}

	/* Start VCOM PWM (50% duty cycle) for JDI memory-in-pixel display */
	if (config->vcom_pwm.dev) {
		if (!pwm_is_ready_dt(&config->vcom_pwm)) {
			LOG_ERR("VCOM PWM device not ready");
			return -ENODEV;
		}
		ret = pwm_set_dt(&config->vcom_pwm, config->vcom_pwm.period,
				 config->vcom_pwm.period / 2U);
		if (ret < 0) {
			LOG_ERR("Failed to set VCOM PWM: %d", ret);
			return ret;
		}
		LOG_DBG("VCOM PWM A started: period=%u ns", config->vcom_pwm.period);
	}

	if (config->vcom_pwm_inv.dev) {
		if (!pwm_is_ready_dt(&config->vcom_pwm_inv)) {
			LOG_ERR("VCOM PWM invert device not ready");
			return -ENODEV;
		}
		ret = pwm_set_dt(&config->vcom_pwm_inv, config->vcom_pwm_inv.period,
				 config->vcom_pwm_inv.period / 2U);
		if (ret < 0) {
			LOG_ERR("Failed to set VCOM PWM invert: %d", ret);
			return ret;
		}
		LOG_DBG("VCOM PWM B (inverted) started: period=%u ns",
			config->vcom_pwm_inv.period);
	}

	return 0;
}

static void jdi_parallel_get_capabilities(const struct device *dev,
					  struct display_capabilities *caps)
{
	const struct jdi_parallel_config *config = dev->config;

	memset(caps, 0, sizeof(*caps));

	caps->x_resolution = config->width;
	caps->y_resolution = config->height;
	caps->current_orientation = DISPLAY_ORIENTATION_NORMAL;
	caps->screen_info = SCREEN_INFO_MONO_VTILED;
}

static int jdi_parallel_write(const struct device *dev, const uint16_t x, const uint16_t y,
			      const struct display_buffer_descriptor *desc, const void *buf)
{
	const struct jdi_parallel_config *config = dev->config;
	const uint8_t *src = buf;

	if (x >= config->width || y >= config->height) {
		return -EINVAL;
	}

	return jdi_parallel_write_data(dev, x, y, desc->width, desc->height, src, desc->buf_size,
				       &config->jdi_cfg);
}

static int jdi_parallel_read(const struct device *dev, const uint16_t x, const uint16_t y,
			     const struct display_buffer_descriptor *desc, void *buf)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(x);
	ARG_UNUSED(y);
	ARG_UNUSED(desc);
	ARG_UNUSED(buf);

	return -ENOTSUP;
}

static int jdi_parallel_set_pixel_format(const struct device *dev, enum display_pixel_format format)
{
	if (format != PIXEL_FORMAT_MONO01) {
		LOG_ERR("Unsupported pixel format: %d", format);
		return -ENOTSUP;
	}

	return 0;
}

static DEVICE_API(display, jdi_parallel_api) = {
	.blanking_on = jdi_parallel_blanking_on,
	.blanking_off = jdi_parallel_blanking_off,
	.write = jdi_parallel_write,
	.read = jdi_parallel_read,
	.get_capabilities = jdi_parallel_get_capabilities,
	.set_pixel_format = jdi_parallel_set_pixel_format,
};

static int jdi_parallel_init(const struct device *dev)
{
	const struct jdi_parallel_config *config = dev->config;
	int ret;

	LOG_DBG("Initializing JDI parallel display");

	/* Check if JDI bus is ready */
	if (!device_is_ready(config->jdi_bus)) {
		LOG_ERR("JDI bus device not ready");
		return -ENODEV;
	}

	/* Configure power GPIOs (inactive state initially) */
	if (config->vlcd_gpio.port) {
		if (!gpio_is_ready_dt(&config->vlcd_gpio)) {
			LOG_ERR("VLCD GPIO device not ready");
			return -ENODEV;
		}
		gpio_pin_configure_dt(&config->vlcd_gpio, GPIO_OUTPUT_INACTIVE);
	}
	if (config->vddp_gpio.port) {
		if (!gpio_is_ready_dt(&config->vddp_gpio)) {
			LOG_ERR("VDDP GPIO device not ready");
			return -ENODEV;
		}
		gpio_pin_configure_dt(&config->vddp_gpio, GPIO_OUTPUT_INACTIVE);
	}

	/* Configure JDI bus */
	ret = jdi_config(config->jdi_bus, &config->jdi_cfg);
	if (ret < 0) {
		LOG_ERR("Failed to configure JDI bus: %d", ret);
		return ret;
	}

	LOG_DBG("JDI parallel display initialized successfully");

	return 0;
}

#define JDI_PARALLEL_DEFINE(n)                                                                     \
	static const struct jdi_parallel_config jdi_parallel_config_##n = {                        \
		.jdi_bus = DEVICE_DT_GET(DT_INST_BUS(n)),                                          \
		.width = DT_INST_PROP(n, width),                                                   \
		.height = DT_INST_PROP(n, height),                                                 \
		.jdi_cfg = {                                                                       \
			.freq = DT_INST_PROP(n, clock_frequency),                                  \
			.bank_col_head = DT_INST_PROP_OR(n, bank_col_head, 0),                     \
			.valid_columns = DT_INST_PROP_OR(n, valid_columns, 0),                     \
			.bank_col_tail = DT_INST_PROP_OR(n, bank_col_tail, 0),                     \
			.bank_row_head = DT_INST_PROP_OR(n, bank_row_head, 0),                     \
			.valid_rows = DT_INST_PROP_OR(n, valid_rows, 0),                           \
			.bank_row_tail = DT_INST_PROP_OR(n, bank_row_tail, 0),                     \
			.enb_start_col = DT_INST_PROP_OR(n, enb_start_col, 0),                     \
			.enb_end_col = DT_INST_PROP_OR(n, enb_end_col, 0),                         \
			.enb_pol_invert = DT_INST_PROP_OR(n, enb_pol_invert, 0),                   \
			.hck_pol_invert = DT_INST_PROP_OR(n, hck_pol_invert, 0),                   \
			.hst_pol_invert = DT_INST_PROP_OR(n, hst_pol_invert, 0),                   \
			.vck_pol_invert = DT_INST_PROP_OR(n, vck_pol_invert, 0),                   \
			.vst_pol_invert = DT_INST_PROP_OR(n, vst_pol_invert, 0),                   \
		},                                                                                 \
		.vlcd_gpio = GPIO_DT_SPEC_INST_GET_OR(n, vlcd_gpios, {0}),                         \
		.vddp_gpio = GPIO_DT_SPEC_INST_GET_OR(n, vddp_gpios, {0}),                         \
		.vcom_pwm = PWM_DT_SPEC_INST_GET_BY_IDX(n, 0),                                     \
		.vcom_pwm_inv = PWM_DT_SPEC_INST_GET_BY_IDX_OR(n, 1, {0}),                         \
		.power_seq_delay_ms = DT_INST_PROP_OR(n, power_seq_delay_ms, 11),                  \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, jdi_parallel_init, NULL, NULL, &jdi_parallel_config_##n,          \
			      POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY,                           \
			      &jdi_parallel_api);
DT_INST_FOREACH_STATUS_OKAY(JDI_PARALLEL_DEFINE)
