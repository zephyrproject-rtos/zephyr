/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT display_mipi_dbi

#include <zephyr/drivers/display.h>
#if CONFIG_MIPI_DSI
#include <zephyr/drivers/mipi_dsi.h>
#include <zephyr/drivers/mipi_dsi/mipi_dsi_mcux_2l.h>
#include <fsl_mipi_dsi.h>
#endif
#include <zephyr/drivers/mipi_dbi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/policy.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(display_mipi_dbi, CONFIG_DISPLAY_LOG_LEVEL);

struct display_mipi_dbi_config {
	const struct device *mipi_dbi;
	const struct gpio_dt_spec bl_gpio;
	const struct gpio_dt_spec te_gpio;
	const uint16_t panel_width;
	const uint16_t panel_height;
};

struct display_mipi_dbi_data {
	enum display_pixel_format pixel_format;
	struct gpio_callback te_gpio_cb;
	struct k_sem te_sem;
};

static void display_mipi_dbi_te_isr_handler(const struct device *gpio_dev,
					struct gpio_callback *cb, uint32_t pins)
{
	struct display_mipi_dbi_data *data =
				CONTAINER_OF(cb, struct display_mipi_dbi_data, te_gpio_cb);

	k_sem_give(&data->te_sem);
}

static int display_mipi_dbi_init(const struct device *dev)
{
	int ret = 0;
	const struct display_mipi_dbi_config *config = dev->config;
	struct display_mipi_dbi_data *data = dev->data;

	if (config->te_gpio.port != NULL) {
		/* Setup TE pin */
		ret = gpio_pin_configure_dt(&config->te_gpio, GPIO_INPUT);
		if (ret < 0) {
			LOG_ERR("Could not configure TE GPIO (%d)", ret);
			return ret;
		}

		ret = gpio_pin_interrupt_configure_dt(&config->te_gpio,
							GPIO_INT_EDGE_TO_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Could not configure TE interrupt (%d)", ret);
			return ret;
		}

		/* Init and install GPIO callback */
		gpio_init_callback(&data->te_gpio_cb, display_mipi_dbi_te_isr_handler,
				BIT(config->te_gpio.pin));
		gpio_add_callback(config->te_gpio.port, &data->te_gpio_cb);

		/* Setup te pin semaphore */
		k_sem_init(&data->te_sem, 0, 1);
	}

	return ret;
}

static int display_mipi_dbi_write(const struct device *dev, const uint16_t x,
			 const uint16_t y,
			 const struct display_buffer_descriptor *desc,
			 const void *buf)
{
	const struct display_mipi_dbi_config *config = dev->config;
	struct display_mipi_dbi_data *data = dev->data;
	uint16_t start, end;
	uint8_t param[4];

	LOG_DBG("W=%d, H=%d @%d,%d", desc->width, desc->height, x, y);

	/* Set column address of target area */
	/* First two bytes are starting X coordinate */
	start = x;
	end = x + desc->width - 1;
	sys_put_be16(start, &param[0]);
	/* Second two bytes are ending X coordinate */
	sys_put_be16(end, &param[2]);

	mipi_dbi_command_write(config->mipi_dbi, NULL, MIPI_DCS_SET_COLUMN_ADDRESS,
							param, sizeof(param));

	/* Set page address of target area */
	/* First two bytes are starting Y coordinate */
	start = y;
	end = y + desc->height - 1;
	sys_put_be16(start, &param[0]);
	/* Second two bytes are ending X coordinate */
	sys_put_be16(end, &param[2]);

	mipi_dbi_command_write(config->mipi_dbi, NULL, MIPI_DCS_SET_PAGE_ADDRESS,
							param, sizeof(param));

	/*
	 * Now, write the framebuffer. If the tearing effect GPIO is present,
	 * wait until the display controller issues an interrupt (which will
	 * give to the TE semaphore) before sending the frame
	 */
	if (config->te_gpio.port != NULL) {
		/* Block sleep state until next TE interrupt so we can send
		 * frame during that interval
		 */
		pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE,
					 PM_ALL_SUBSTATES);
		k_sem_take(&data->te_sem, K_FOREVER);
		pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE,
					 PM_ALL_SUBSTATES);
	}

	mipi_dbi_write_display(config->mipi_dbi, NULL, (uint8_t *)buf,
	 (struct display_buffer_descriptor *)desc, data->pixel_format);

	return 0;
}

static int display_mipi_dbi_blanking_off(const struct device *dev)
{
	const struct display_mipi_dbi_config *config = dev->config;

	if (config->bl_gpio.port != NULL) {
		return gpio_pin_set_dt(&config->bl_gpio, 1);
	} else {
		return -ENOTSUP;
	}
}

static int display_mipi_dbi_blanking_on(const struct device *dev)
{
	const struct display_mipi_dbi_config *config = dev->config;

	if (config->bl_gpio.port != NULL) {
		return gpio_pin_set_dt(&config->bl_gpio, 0);
	} else {
		return -ENOTSUP;
	}
}

static int display_mipi_dbi_set_pixel_format(const struct device *dev,
					const enum display_pixel_format pixel_format)
{
	const struct display_mipi_dbi_config *config = dev->config;
	struct display_mipi_dbi_data *data = dev->data;
	uint8_t param;

	data->pixel_format = pixel_format;

	switch (pixel_format) {
	case PIXEL_FORMAT_RGB_565:
		param = MIPI_DCS_PIXEL_FORMAT_16BIT;
		break;
	case PIXEL_FORMAT_RGB_888:
		param = MIPI_DCS_PIXEL_FORMAT_24BIT;
		break;
	default:
		/* Other display formats not implemented */
		return -ENOTSUP;
	}

	return mipi_dbi_command_write(config->mipi_dbi, NULL, MIPI_DCS_SET_PIXEL_FORMAT,
									&param, 1);
}

static void display_mipi_dbi_get_capabilities(const struct device *dev,
					 struct display_capabilities *capabilities)
{
	const struct display_mipi_dbi_config *config = dev->config;
	const struct display_mipi_dbi_data *data = dev->data;

	memset(capabilities, 0, sizeof(struct display_capabilities));
	capabilities->x_resolution = config->panel_width;
	capabilities->y_resolution = config->panel_height;
	capabilities->supported_pixel_formats = PIXEL_FORMAT_RGB_888 | PIXEL_FORMAT_RGB_565;
	capabilities->current_pixel_format = data->pixel_format;

	if ((data->pixel_format & capabilities->supported_pixel_formats) == 0U) {
		LOG_WRN("Unsupported display format");
	}
}

static const struct display_driver_api display_mipi_dbi_api = {
	.blanking_on = display_mipi_dbi_blanking_on,
	.blanking_off = display_mipi_dbi_blanking_off,
	.write = display_mipi_dbi_write,
	.get_capabilities = display_mipi_dbi_get_capabilities,
	.set_pixel_format = display_mipi_dbi_set_pixel_format,
};

#define DISPLAY_MIPI_DBI(id)							\
	const struct display_mipi_dbi_config display_mipi_dbi_config_##id = {	\
		.mipi_dbi = DEVICE_DT_GET(DT_INST_PARENT(id)),			\
		.bl_gpio = GPIO_DT_SPEC_INST_GET_OR(id, bl_gpios, {0}),		\
		.te_gpio = GPIO_DT_SPEC_INST_GET_OR(id, te_gpios, {0}),		\
		.panel_width = DT_INST_PROP(id, width),				\
		.panel_height = DT_INST_PROP(id, height),			\
	};									\
	struct display_mipi_dbi_data display_mipi_dbi_data_##id = {		\
		.pixel_format = DT_INST_PROP(id, current_pixel_format),		\
	};									\
	DEVICE_DT_INST_DEFINE(id,						\
				&display_mipi_dbi_init,				\
				PM_DEVICE_DT_INST_GET(id),			\
				&display_mipi_dbi_data_##id,			\
				&display_mipi_dbi_config_##id,			\
				POST_KERNEL,					\
				CONFIG_APPLICATION_INIT_PRIORITY,		\
				&display_mipi_dbi_api);

DT_INST_FOREACH_STATUS_OKAY(DISPLAY_MIPI_DBI)
