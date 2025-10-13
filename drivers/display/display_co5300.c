/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT chipone_co5300

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(co5300, CONFIG_DISPLAY_LOG_LEVEL);

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/mipi_dsi/mipi_dsi_mcux_2l.h>
#include <zephyr/drivers/mipi_dsi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#include <fsl_lcdif.h>
#include <fsl_mipi_dsi.h>

struct display_cmds {
	uint8_t *cmd_code;
	uint8_t size;
};

struct co5300_config {
	const struct device *mipi_dsi;
	const struct gpio_dt_spec reset_gpio;
	const struct gpio_dt_spec backlight_gpio;
	const struct gpio_dt_spec tear_effect_gpio;
	const struct gpio_dt_spec power_gpio;
	uint16_t panel_width;
	uint16_t panel_height;
	uint16_t channel;
	uint16_t num_of_lanes;
};

struct co5300_data {
	uint8_t pixel_format;
	uint8_t bytes_per_pixel;
	struct gpio_callback tear_effect_gpio_cb;
	struct k_sem tear_effect_sem;
};

/* Organized as MIPI_CMD | SIZE OF MIPI PARAM | MIPI PARAM */
uint8_t lcm_init_cmds[] = {	0xFE, 0x1, 0x20,
				0xF4, 0x1, 0x5A,
				0xF5, 0x1, 0x59,
				0xFE, 0x1, 0x40,
				0x96, 0x1, 0x00,
				0xC9, 0x1, 0x00,
				0xFE, 0x1, 0x00,
				0x35, 0x1, 0x00,
				0x53, 0x1, 0x20,
				0x51, 0x1, 0xFF,
				0x63, 0x1, 0xFF,
				0x2A, 0x4, 0x00, 0x06, 0x01, 0xD7,
				0x2B, 0x4, 0x00, 0x00, 0x01, 0xD1};

static void co5300_tear_effect_isr_handler(const struct device* gpio_dev,
			struct gpio_callback *cb, uint32_t pins)
{
	struct co5300_data *data = CONTAINER_OF(cb, struct co5300_data, tear_effect_gpio_cb);

	k_sem_give(&data->tear_effect_sem);
}


static int co5300_blanking_on(const struct device *dev)
{
	const struct co5300_config *config = dev->config;

	if (config->backlight_gpio.port != NULL) {
		return gpio_pin_set_dt(&config->backlight_gpio, 0);
	} else {
		return -ENOTSUP;
	}
}

static int co5300_blanking_off(const struct device *dev)
{
	const struct co5300_config *config = dev->config;

	if (config->backlight_gpio.port != NULL) {
		return gpio_pin_set_dt(&config->backlight_gpio, 1);
	} else {
		return -ENOTSUP;
	}
}

#if 0
/* Helper function to write framebuffer data to co5300 via MIPI interface. */
static int co5300_write_fb(const struct device *dev, bool first_write,
			const uint8_t *src, const struct display_buffer_descriptor *desc)
{
	const struct co5300_config *config = dev->config;
	struct co5300_data *data = dev->data;
	ssize_t wlen;
	struct mipi_dsi_msg msg = {0};
	uint8_t *local_src = (uint8_t *)src;
	uint32_t len = desc->height * desc->width * data->bytes_per_pixel;
	uint32_t len_sent = 0U;

	/* Note- we need to set custom flags on the DCS message,
	 * so we bypass the mipi_dsi_dcs_write API
	 */
	if (first_write) {
		msg.cmd = MIPI_DCS_WRITE_MEMORY_START;
	} else {
		msg.cmd = MIPI_DCS_WRITE_MEMORY_CONTINUE;
	}
	msg.type = MIPI_DSI_DCS_LONG_WRITE;
	msg.flags = MCUX_DSI_2L_FB_DATA;
	msg.user_data = (void *)desc;

	while (len > 0) {
		msg.tx_len = len;
		msg.tx_buf = local_src;
		wlen = mipi_dsi_transfer(config->mipi_dsi, config->channel, &msg);
		if (wlen < 0) {
			return (int)wlen;
		}
		/* Advance source pointer and decrement remaining */
		if (desc->pitch > desc->width) {
			len_sent += wlen;
			local_src += wlen + len_sent / (desc->width * data->bytes_per_pixel) *
				((desc->pitch - desc->width) * data->bytes_per_pixel);
		} else {
			local_src += wlen;
		}
		len -= wlen;
		/* All future commands should use WRITE_MEMORY_CONTINUE */
		msg.cmd = MIPI_DCS_WRITE_MEMORY_CONTINUE;
	}
	return wlen;
}
#else
/* Helper function to write framebuffer data to co5300 via MIPI interface. */
static int co5300_write_fb(const struct device *dev, bool first_write,
			const uint8_t *src, const struct display_buffer_descriptor *desc)
{
	const struct co5300_config *config = dev->config;
	struct co5300_data *data = dev->data;
	ssize_t wlen;
	struct mipi_dsi_msg msg = {0};
	uint8_t *local_src = (uint8_t *)src;
	uint32_t len = desc->height * desc->width * data->bytes_per_pixel;
	uint32_t len_sent = 0U;

	/* Note- we need to set custom flags on the DCS message,
	 * so we bypass the mipi_dsi_dcs_write API
	 */
	if (first_write) {
		msg.cmd = MIPI_DCS_WRITE_MEMORY_START;
	} else {
		msg.cmd = MIPI_DCS_WRITE_MEMORY_CONTINUE;
	}
	msg.type = MIPI_DSI_DCS_LONG_WRITE;
	msg.flags = MCUX_DSI_2L_FB_DATA;
	msg.user_data = (void *)desc;

	while (len > 0) {
		msg.tx_len = len;
		msg.tx_buf = local_src;
		wlen = mipi_dsi_transfer(config->mipi_dsi, config->channel, &msg);
		if (wlen < 0) {
			return (int)wlen;
		}
		/* Advance source pointer and decrement remaining */
		if (desc->pitch > desc->width) {
			len_sent += wlen;
			local_src += wlen + len_sent / (desc->width * data->bytes_per_pixel) *
				((desc->pitch - desc->width) * data->bytes_per_pixel);
		} else {
			local_src += wlen;
		}
		len -= wlen;
		/* All future commands should use WRITE_MEMORY_CONTINUE */
		msg.cmd = MIPI_DCS_WRITE_MEMORY_CONTINUE;
	}
	return wlen;
}
#endif



#if 0
static int co5300_write(const struct device *dev,
		const uint16_t x, const uint16_t y,
		const struct display_buffer_descriptor *desc,
		const void *buf)
{
	const struct co5300_config *config = dev->config;
	struct co5300_data *data = dev->data;
	int ret;
	uint16_t start, end;
	const uint8_t *src;
	bool first_cmd;
	uint8_t param[4];

	LOG_DBG("W=%d, H=%d @%d,%d", desc->width, desc->height, x, y);

	/*
	 * CO5300 runs in MIPI DBI mode. This means we can use command mode
	 * to write to the video memory buffer on the CO5300 control IC,
	 * and the IC will update the display automatically.
	 */

	/* Set column address of target area */
	/* First two bytes are starting X coordinate */
	start = x;
	end = x + desc->width - 1;
	sys_put_be16(start, &param[0]);
	/* Second two bytes are ending X coordinate */
	sys_put_be16(end, &param[2]);
	ret = mipi_dsi_dcs_write(config->mipi_dsi, config->channel,
				MIPI_DCS_SET_COLUMN_ADDRESS, param,
				sizeof(param));
	if (ret < 0) {
		return ret;
	}

	/* Set page address of target area */
	/* First two bytes are starting Y coordinate */
	start = y;
	end = y + desc->height - 1;
	sys_put_be16(start, &param[0]);
	/* Second two bytes are ending X coordinate */
	sys_put_be16(end, &param[2]);
	ret = mipi_dsi_dcs_write(config->mipi_dsi, config->channel,
				MIPI_DCS_SET_PAGE_ADDRESS, param,
				sizeof(param));
	if (ret < 0) {
		return ret;
	}

	/*
	 * Now, write the framebuffer. If the tearing effect GPIO is present,
	 * wait until the display controller issues an interrupt (which will
	 * give to the TE semaphore) before sending the frame
	 */
	if (config->tear_effect_gpio.port != NULL) {
		/* Block sleep state until next TE interrupt so we can send
		 * frame during that interval
		 */
		k_sem_take(&data->tear_effect_sem, K_FOREVER);
	}
	src = buf;
	first_cmd = true;

	co5300_write_fb(dev, first_cmd, src, desc);

	return 0;

}
#else
static int co5300_write(const struct device *dev,
		const uint16_t x, const uint16_t y,
		const struct display_buffer_descriptor *desc,
		const void *buf)
{
	const struct co5300_config *config = dev->config;
	struct co5300_data *data = dev->data;
	int ret;
	uint16_t start, end;
	const uint8_t *src;
	bool first_cmd;
	uint8_t param[4];

	LOG_DBG("W=%d, H=%d @%d,%d", desc->width, desc->height, x, y);

	/*
	 * CO5300 runs in MIPI DBI mode. This means we can use command mode
	 * to write to the video memory buffer on the CO5300 control IC,
	 * and the IC will update the display automatically.
	 */

	/* Set column address of target area */
	/* First two bytes are starting X coordinate */
	start = x;
	end = x + desc->width - 1;
	sys_put_be16(start, &param[0]);
	/* Second two bytes are ending X coordinate */
	sys_put_be16(end, &param[2]);
	ret = mipi_dsi_dcs_write(config->mipi_dsi, config->channel,
				MIPI_DCS_SET_COLUMN_ADDRESS, param,
				sizeof(param));
	if (ret < 0) {
		return ret;
	}

	/* Set page address of target area */
	/* First two bytes are starting Y coordinate */
	start = y;
	end = y + desc->height - 1;
	sys_put_be16(start, &param[0]);
	/* Second two bytes are ending X coordinate */
	sys_put_be16(end, &param[2]);
	ret = mipi_dsi_dcs_write(config->mipi_dsi, config->channel,
				MIPI_DCS_SET_PAGE_ADDRESS, param,
				sizeof(param));
	if (ret < 0) {
		return ret;
	}

	/*
	 * Now, write the framebuffer. If the tearing effect GPIO is present,
	 * wait until the display controller issues an interrupt (which will
	 * give to the TE semaphore) before sending the frame
	 */
	if (config->tear_effect_gpio.port != NULL) {
		/* Block sleep state until next TE interrupt so we can send
		 * frame during that interval
		 */
		k_sem_take(&data->tear_effect_sem, K_FOREVER);
	}
	src = buf;
	first_cmd = true;

	co5300_write_fb(dev, first_cmd, src, desc);

	return 0;

}
#endif

static int co5300_read(const struct device *dev,
		const uint16_t x, const uint16_t y,
		const struct display_buffer_descriptor *desc, void *buf)
{
	return 0;
}

static int co5300_clear(const struct device *dev)
{
	return 0;
}

static void *co5300_get_framebuffer(const struct device *dev)
{
	return 0;
}

static int co5300_set_brightness(const struct device *dev, uint8_t)
{
	return 0;
}

static int co5300_set_contrast(const struct device *dev, uint8_t contrast)
{
	return 0;
}

#if 0
static void co5300_get_capabilities(const struct device *dev,
		struct display_capabilities *capabilities)
{
	const struct co5300_config *config = dev->config;
	const struct co5300_data *data = dev->data;

	memset(capabilities, 0, sizeof(struct display_capabilities));
	capabilities->x_resolution = config->panel_width;
	capabilities->y_resolution = config->panel_height;
	capabilities->supported_pixel_formats = PIXEL_FORMAT_RGB_565 |
						PIXEL_FORMAT_RGB_888;

	switch (data->pixel_format) {
	case MIPI_DSI_PIXFMT_RGB565:
		capabilities->current_pixel_format = PIXEL_FORMAT_RGB_565;
		break;
	case MIPI_DSI_PIXFMT_RGB888:
		capabilities->current_pixel_format = PIXEL_FORMAT_RGB_888;
		break;
	default:
		LOG_WRN("Unsupported display format");
		/* Other display formats not implemented */
		break;
	}
	capabilities->current_orientation = DISPLAY_ORIENTATION_ROTATED_90;
}
#else
static void co5300_get_capabilities(const struct device *dev,
		struct display_capabilities *capabilities)
{
	const struct co5300_config *config = dev->config;
	const struct co5300_data *data = dev->data;

	memset(capabilities, 0, sizeof(struct display_capabilities));
	capabilities->x_resolution = config->panel_width;
	capabilities->y_resolution = config->panel_height;
	capabilities->supported_pixel_formats = PIXEL_FORMAT_RGB_565 |
						PIXEL_FORMAT_RGB_888;

	switch (data->pixel_format) {
	case MIPI_DSI_PIXFMT_RGB565:
		capabilities->current_pixel_format = PIXEL_FORMAT_RGB_565;
		break;
	case MIPI_DSI_PIXFMT_RGB888:
		capabilities->current_pixel_format = PIXEL_FORMAT_RGB_888;
		break;
	default:
		LOG_WRN("Unsupported display format");
		/* Other display formats not implemented */
		break;
	}
	capabilities->current_orientation = DISPLAY_ORIENTATION_ROTATED_90;
}
#endif

#if 0
static int co5300_set_pixel_format(const struct device *dev,
		const enum display_pixel_format pixel_format)
{
	const struct co5300_config *config = dev->config;
	struct co5300_data *data = dev->data;
	uint8_t param;

	switch (data->pixel_format) {
	case PIXEL_FORMAT_RGB_565:
		data->pixel_format = MIPI_DSI_PIXFMT_RGB565;
		param = MIPI_DCS_PIXEL_FORMAT_16BIT;
		data->bytes_per_pixel = 2;
		break;
	case PIXEL_FORMAT_RGB_888:
		data->pixel_format = MIPI_DSI_PIXFMT_RGB888;
		param = MIPI_DCS_PIXEL_FORMAT_24BIT;
		data->bytes_per_pixel = 3;
		break;
	default:
		/* Other display formats not implemented */
		return -ENOTSUP;
	}

	return mipi_dsi_dcs_write(config->mipi_dsi, config->channel,
				MIPI_DCS_SET_PIXEL_FORMAT, &param, 1);

	return 0;

}
#else
static int co5300_set_pixel_format(const struct device *dev,
		const enum display_pixel_format pixel_format)
{
	const struct co5300_config *config = dev->config;
	struct co5300_data *data = dev->data;
	uint8_t param;

	switch (data->pixel_format) {
	case PIXEL_FORMAT_RGB_565:
		data->pixel_format = MIPI_DSI_PIXFMT_RGB565;
		param = MIPI_DCS_PIXEL_FORMAT_16BIT;
		data->bytes_per_pixel = 2;
		break;
	case PIXEL_FORMAT_RGB_888:
		data->pixel_format = MIPI_DSI_PIXFMT_RGB888;
		param = MIPI_DCS_PIXEL_FORMAT_24BIT;
		data->bytes_per_pixel = 3;
		break;
	default:
		/* Other display formats not implemented */
		return -ENOTSUP;
	}

	return mipi_dsi_dcs_write(config->mipi_dsi, config->channel,
				MIPI_DCS_SET_PIXEL_FORMAT, &param, 1);

	return 0;

}
#endif

#if 0
static int co5300_set_orientation(const struct device *dev,
		const enum display_orientation orientation)
{
	if (orientation == DISPLAY_ORIENTATION_NORMAL) {
		return 0;
	}
	LOG_ERR("Changing display orientation not implemented");
	return -ENOTSUP;
}
#else
static int co5300_set_orientation(const struct device *dev,
		const enum display_orientation orientation)
{
	if (orientation == DISPLAY_ORIENTATION_NORMAL) {
		return 0;
	}
	LOG_ERR("Changing display orientation not implemented");
	return -ENOTSUP;
}
#endif

static int co5300_init(const struct device *dev)
{
	const struct co5300_config *config = dev->config;
	struct co5300_data *data = dev->data;
	struct mipi_dsi_device mdev = {0};

	/* Attach to MIPI DSI host */
	mdev.data_lanes = config->num_of_lanes;
	mdev.pixfmt = data->pixel_format;

	int ret = mipi_dsi_attach(config->mipi_dsi, config->channel, &mdev);
	if (ret < 0) {
		LOG_ERR("Could not attach to MIPI-DSI host");
		return ret;
	}

	/* Perform GPIO Reset */
	if (config->power_gpio.port != NULL) {
		ret = gpio_pin_configure_dt(&config->power_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Could not configure power GPIO (%d)", ret);
			return ret;
		}

		ret = gpio_pin_set_dt(&config->power_gpio, 1);
		if (ret < 0) {
			LOG_ERR("Could not pull power high (%d)", ret);
			return ret;
		}

		k_sleep(K_MSEC(100));

		if (config->reset_gpio.port != NULL) {
			ret = gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_INACTIVE);
			if (ret < 0) {
				LOG_ERR("Could not configure reset GPIO (%d)", ret);
				return ret;
			}

			ret = gpio_pin_set_dt(&config->reset_gpio, 0);
			if (ret < 0) {
				LOG_ERR("Could not pull reset low (%d)", ret);
				return ret;
			}

			k_sleep(K_MSEC(1));
			gpio_pin_set_dt(&config->reset_gpio, 1);
			if (ret < 0) {
				LOG_ERR("Could not pull reset high (%d)", ret);
				return ret;
			}
			k_sleep(K_MSEC(150));
		}
	}

	/* Set the LCM init settings. */
	struct display_cmds lcm_init_settings = {};
	lcm_init_settings.cmd_code = lcm_init_cmds;
	lcm_init_settings.size = ARRAY_SIZE(lcm_init_cmds);
	uint8_t* curr_cmd = lcm_init_settings.cmd_code;

	while (curr_cmd != (lcm_init_settings.cmd_code + lcm_init_settings.size)) {
		if (curr_cmd > (lcm_init_settings.cmd_code + lcm_init_settings.size)) {
			LOG_ERR("Logical error when sending mipi command code.");
			return -EILSEQ;
		}

		uint32_t cmd_code = (uint32_t)*curr_cmd++;
		uint32_t param_size = *curr_cmd++;
		uint32_t param = *curr_cmd;
		curr_cmd += param_size;

		ret = mipi_dsi_dcs_write(config->mipi_dsi, config->channel,
				cmd_code, &param, param_size);
		if (ret < 0) {
			return ret;
		}
	}

	/* Set pixel format */
	int param = 0;
	if (data->pixel_format == MIPI_DSI_PIXFMT_RGB888) {
		param = MIPI_DCS_PIXEL_FORMAT_24BIT;
		data->bytes_per_pixel = 3;
	} else if (data->pixel_format == MIPI_DSI_PIXFMT_RGB565) {
		param = MIPI_DCS_PIXEL_FORMAT_16BIT;
		data->bytes_per_pixel = 2;
	} else {
		/* Unsupported pixel format */
		LOG_ERR("Pixel format not supported");
		return -ENOTSUP;
	}
	ret = mipi_dsi_dcs_write(config->mipi_dsi, config->channel,
				MIPI_DCS_SET_PIXEL_FORMAT, &param, 1);

	if (ret < 0) {
		return ret;
	}

	/* Command the display to enter sleep mode */
	k_sleep(K_MSEC(50));
	ret = mipi_dsi_dcs_write(config->mipi_dsi, config->channel,
				MIPI_DCS_EXIT_SLEEP_MODE, NULL, 0);
	if (ret < 0) {
		return ret;
	}


	/* Commands after the monitor is directed to go to sleep should be delayed 150ms */
	k_sleep(K_MSEC(150));

	/* Setup backlight */
	if (config->backlight_gpio.port != NULL) {
		ret = gpio_pin_configure_dt(&config->backlight_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Could not configure bl GPIO (%d)", ret);
			return ret;
		}
	}

	/* Setup tear effect pin */
	if (config->tear_effect_gpio.port != NULL) {
		ret = gpio_pin_configure_dt(&config->tear_effect_gpio, GPIO_INPUT);
		if (ret < 0) {
			LOG_ERR("Could not configure TE GPIO (%d)", ret);
			return ret;
		}

		ret = gpio_pin_interrupt_configure_dt(&config->tear_effect_gpio,
						      GPIO_INT_EDGE_TO_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Could not configure TE interrupt (%d)", ret);
			return ret;
		}

		/* Init and install GPIO callback */
		gpio_init_callback(&data->tear_effect_gpio_cb, co5300_tear_effect_isr_handler,
				BIT(config->tear_effect_gpio.pin));
		ret = gpio_add_callback(config->tear_effect_gpio.port, &data->tear_effect_gpio_cb);
		if (ret < 0) {
			LOG_ERR("Could not add TE gpio callback");
			return ret;
		}

		/* Setup semaphore for using the tear effect pin */
		k_sem_init(&data->tear_effect_sem, 0, 1);
	}

	/* Enable display */
	return mipi_dsi_dcs_write(config->mipi_dsi, config->channel,
				MIPI_DCS_SET_DISPLAY_ON, NULL, 0);
}

static DEVICE_API(display, co5300_api) = {
	.blanking_on = co5300_blanking_on,
	.blanking_off = co5300_blanking_off,
	.write = co5300_write,
	.read = co5300_read,
	.clear = co5300_clear,
	.get_framebuffer = co5300_get_framebuffer,
	.set_brightness = co5300_set_brightness,
	.set_contrast = co5300_set_contrast,
	.get_capabilities = co5300_get_capabilities,
	.set_pixel_format = co5300_set_pixel_format,
	.set_orientation = co5300_set_orientation,
};

#define CO5300_DEVICE_INIT(node_id)									\
	static const struct co5300_config co5300_config_##node_id = {					\
		.mipi_dsi = DEVICE_DT_GET(DT_INST_BUS(node_id)),					\
		.num_of_lanes = DT_INST_PROP_BY_IDX(node_id, data_lanes, 0),				\
		.channel = DT_INST_REG_ADDR(node_id),							\
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(node_id, reset_gpios, {0}),			\
		.backlight_gpio = GPIO_DT_SPEC_INST_GET_OR(node_id, backlight_gpios, {0}),		\
		.tear_effect_gpio = GPIO_DT_SPEC_INST_GET_OR(node_id, tear_effect_gpios, {0}),		\
		.panel_width = DT_INST_PROP(node_id, width),						\
		.panel_height = DT_INST_PROP(node_id, height),						\
	};												\
	static struct co5300_data co5300_data_##node_id = {						\
		.pixel_format = DT_INST_PROP(node_id, pixel_format),					\
	};												\
	DEVICE_DT_INST_DEFINE(node_id,									\
			    &co5300_init,								\
			    0,										\
			    &co5300_data_##node_id,							\
			    &co5300_config_##node_id,							\
			    POST_KERNEL,								\
			    CONFIG_APPLICATION_INIT_PRIORITY,						\
			    &co5300_api);


DT_INST_FOREACH_STATUS_OKAY(CO5300_DEVICE_INIT)
