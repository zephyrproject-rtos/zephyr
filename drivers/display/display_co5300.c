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
#include <zephyr/linker/devicetree_regions.h>
#include <fsl_lcdif.h>
#include <fsl_mipi_dsi.h>

/* display command structure passed to mipi to control the display */
struct display_cmds {
	uint8_t *cmd_code;
	uint8_t size;
};

struct co5300_config {
	const struct device *mipi_dsi;
	const struct gpio_dt_spec reset_gpios;
	const struct gpio_dt_spec backlight_gpios;
	const struct gpio_dt_spec tear_effect_gpios;
	const struct gpio_dt_spec power_gpios;
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
	/* Pointer to framebuffer */
	uint8_t *frame_ptr;
	uint32_t frame_pitch;
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

static void co5300_tear_effect_isr_handler(const struct device *gpio_dev,
			struct gpio_callback *cb, uint32_t pins)
{
	struct co5300_data *data = CONTAINER_OF(cb, struct co5300_data, tear_effect_gpio_cb);

	k_sem_give(&data->tear_effect_sem);
}


static int co5300_blanking_on(const struct device *dev)
{
	const struct co5300_config *config = dev->config;

	if (config->backlight_gpios.port != NULL) {
		return gpio_pin_set_dt(&config->backlight_gpios, 0);
	}

	return -ENOTSUP;
}

static int co5300_blanking_off(const struct device *dev)
{
	const struct co5300_config *config = dev->config;

	if (config->backlight_gpios.port != NULL) {
		return gpio_pin_set_dt(&config->backlight_gpios, 1);
	}

	return -ENOTSUP;
}

static void co5300_copy_and_adjust_coordinates(const struct device *dev,
		const uint16_t x, const uint16_t y,
		const struct display_buffer_descriptor *desc,
		const void *buf,
		uint16_t *local_x, uint16_t *local_y,
		struct display_buffer_descriptor *local_desc)
{
	struct co5300_data *data = dev->data;
	const uint8_t *src;
	uint8_t *dst;

	/* Copy the update area to the framebuffer */
	src = buf;
	dst = data->frame_ptr + (y * data->frame_pitch * data->bytes_per_pixel)
		+ (x * data->bytes_per_pixel);
	for (uint16_t row = 0; row < desc->height; row++) {
		memcpy(dst, src, desc->width * data->bytes_per_pixel);
		src += desc->pitch * data->bytes_per_pixel;
		dst += data->frame_pitch * data->bytes_per_pixel;
	}

	/*
	 * Initialize descriptor for local frame buffer.
	 * The start coordinates and the width/height of the updated area
	 * cannot be odd value for the panel. Adjust them to be even.
	 */
	if (x % 2 != 0) {
		*local_x = x - 1;
		local_desc->width = desc->width + 1U;
	} else {
		*local_x = x;
		local_desc->width = desc->width;
	}
	if (y % 2 != 0) {
		*local_y = y - 1;
		local_desc->height = desc->height + 1U;
	} else {
		*local_y = y;
		local_desc->height = desc->height;
	}
	local_desc->width = ROUND_UP(local_desc->width, 2U);
	local_desc->height = ROUND_UP(local_desc->height, 2U);
	local_desc->pitch = data->frame_pitch;
	local_desc->frame_incomplete = desc->frame_incomplete;
	local_desc->buf_size = local_desc->width * local_desc->height * data->bytes_per_pixel;
}

static int co5300_write(const struct device *dev,
		const uint16_t x, const uint16_t y,
		const struct display_buffer_descriptor *desc,
		const void *buf)
{
	const struct co5300_config *config = dev->config;
	struct co5300_data *data = dev->data;
	int ret;
	uint16_t start_pos;
	uint16_t end_pos;
	uint8_t cmd_params[4];
	struct mipi_dsi_msg msg = {0};
	uint16_t line_each_sent = 0U;
	int bytes_written = 0;
	const uint8_t *src;
	uint32_t tx_size = 0U;
	uint16_t local_x, local_y;
	struct display_buffer_descriptor local_desc = {0};

	/* Check whether the updated area is outside of the panel frame. */
	if ((x > config->panel_width) || (y > config->panel_height) ||
			((x + desc->width) > config->panel_width) ||
			((y + desc->height) > config->panel_height)) {
		LOG_ERR("Update area outside panel dimensions");
		return -EINVAL;
	}

	/* Check whether the updated area is valid */
	if (desc->width == 0 || desc->height == 0) {
		LOG_ERR("The height/width of the update area cannot be 0");
		return -EINVAL;
	}

	/* Copy data to framebuffer and adjust coordinates for even values */
	co5300_copy_and_adjust_coordinates(dev, x, y, desc, buf, &local_x, &local_y, &local_desc);

	/*
	 * Set column address of target area. The circular panel actually starts
	 * to show from row 6, row 0~5 are cut off physically. The actual display
	 * area is row 6~472 and line 0~466. So adjust coordinates accordingly.
	 */
	/* First two bytes are starting X coordinate */
	start_pos = local_x + 6U;
	sys_put_be16(start_pos, &cmd_params[0]);

	/* Second two bytes are ending X coordinate */
	end_pos = local_x + local_desc.width + 5U;
	sys_put_be16(end_pos, &cmd_params[2]);
	ret = mipi_dsi_dcs_write(config->mipi_dsi, config->channel,
				MIPI_DCS_SET_COLUMN_ADDRESS, cmd_params,
				sizeof(cmd_params));
	if (ret < 0) {
		return ret;
	}

	/* Set page address of target area */
	/* First two bytes are starting Y coordinate */
	start_pos = local_y;
	sys_put_be16(start_pos, &cmd_params[0]);

	/* Second two bytes are ending Y coordinate */
	end_pos = local_y + local_desc.height - 1;
	sys_put_be16(end_pos, &cmd_params[2]);
	ret = mipi_dsi_dcs_write(config->mipi_dsi, config->channel,
				MIPI_DCS_SET_PAGE_ADDRESS, cmd_params,
				sizeof(cmd_params));
	if (ret < 0) {
		return ret;
	}

	/*
	 * When writing the to the framebuffer and the tearing effect GPIO is present,
	 * we need to wait for the tear_effect GPIO semaphore to be released.
	 */
	if (config->tear_effect_gpios.port != NULL) {
		k_sem_take(&data->tear_effect_sem, K_FOREVER);
	}

	/* Start memory write. */
	/* The address and the total pixel size of the updated area. */
	src = data->frame_ptr + (local_y * data->frame_pitch * data->bytes_per_pixel)
		+ (local_x * data->bytes_per_pixel);
	tx_size = local_desc.buf_size;

	msg.type = MIPI_DSI_DCS_LONG_WRITE;
	msg.flags = MCUX_DSI_2L_FB_DATA;
	msg.user_data = &local_desc;
	msg.cmd = MIPI_DCS_WRITE_MEMORY_START;

	while (tx_size > 0) {
		msg.tx_len = tx_size;
		msg.tx_buf = src;
		bytes_written = (int)mipi_dsi_transfer(config->mipi_dsi, config->channel, &msg);
		if (bytes_written < 0) {
			return bytes_written;
		}

		tx_size -= bytes_written;

		if (tx_size == 0U) {
			break;
		}

		/* Advance source pointer and decrement remaining */
		if (local_desc.pitch > local_desc.width) {
			line_each_sent = bytes_written / (local_desc.width * data->bytes_per_pixel);
			src += line_each_sent * local_desc.pitch * data->bytes_per_pixel;
			src += bytes_written % (local_desc.width * data->bytes_per_pixel);
		} else {
			src += bytes_written;
		}

		/* All future commands should use WRITE_MEMORY_CONTINUE */
		msg.cmd = MIPI_DCS_WRITE_MEMORY_CONTINUE;
	}
	return 0;
}

static int co5300_set_brightness(const struct device *dev, const uint8_t contrast)
{
	const struct co5300_config *config = dev->config;

	return mipi_dsi_dcs_write(config->mipi_dsi, config->channel,
		MIPI_DCS_SET_DISPLAY_BRIGHTNESS, &contrast, 1);
}

static void co5300_get_capabilities(const struct device *dev,
		struct display_capabilities *capabilities)
{
	const struct co5300_config *config = dev->config;
	struct co5300_data *data = dev->data;

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
		LOG_ERR("Unsupported display format");
		/* Other display formats not implemented */
		break;
	}
	capabilities->current_orientation = DISPLAY_ORIENTATION_NORMAL;
}

static int co5300_set_pixel_format(const struct device *dev,
		const enum display_pixel_format pixel_format)
{
	const struct co5300_config *config = dev->config;
	struct co5300_data *data = dev->data;
	uint8_t cmd_params[2];
	int ret;
	uint8_t mipi_pixel_format, bytes_per_pixel;

	switch (pixel_format) {
	case PIXEL_FORMAT_RGB_565:
		mipi_pixel_format = MIPI_DSI_PIXFMT_RGB565;
		bytes_per_pixel = 2U;
		cmd_params[0] = MIPI_DCS_ADDRESS_MODE_BGR;
		cmd_params[1] = MIPI_DCS_PIXEL_FORMAT_16BIT;
		break;
	case PIXEL_FORMAT_RGB_888:
		mipi_pixel_format = MIPI_DSI_PIXFMT_RGB888;
		bytes_per_pixel = 3U;
		cmd_params[0] = 0U;
		cmd_params[1] = MIPI_DCS_PIXEL_FORMAT_24BIT;
		break;
	default:
		/* Other display formats not implemented */
		return -ENOTSUP;
	}

	/*
	 * Controller-specific requirement, when using RGB565 format
	 * the order shall be set to BGR.
	 */
	ret = mipi_dsi_dcs_write(config->mipi_dsi, config->channel,
		MIPI_DCS_SET_ADDRESS_MODE, &cmd_params[0], 1U);
	if (ret < 0) {
		return ret;
	}

	ret = mipi_dsi_dcs_write(config->mipi_dsi, config->channel,
			MIPI_DCS_SET_PIXEL_FORMAT, &cmd_params[1], 1U);
	if (ret < 0) {
		return ret;
	}

	/* Update the format in the device data after DCS command succeeds. */
	data->bytes_per_pixel = bytes_per_pixel;
	data->pixel_format = mipi_pixel_format;

	return 0;
}

static int co5300_set_orientation(const struct device *dev,
		const enum display_orientation orientation)
{
	if (orientation == DISPLAY_ORIENTATION_NORMAL) {
		return 0;
	}
	LOG_ERR("Changing display orientation not implemented");
	return -ENOTSUP;
}

static int co5300_reset(const struct device *dev)
{
	const struct co5300_config *config = dev->config;
	int ret = 0;

	if (config->reset_gpios.port != NULL) {
		ret = gpio_pin_configure_dt(&config->reset_gpios, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Could not configure reset GPIO (%d)", ret);
			return ret;
		}

		k_sleep(K_MSEC(10));
		ret = gpio_pin_set_dt(&config->reset_gpios, 0);
		if (ret < 0) {
			LOG_ERR("Could not pull reset low (%d)", ret);
			return ret;
		}

		k_sleep(K_MSEC(30));
		ret = gpio_pin_set_dt(&config->reset_gpios, 1);
		if (ret < 0) {
			LOG_ERR("Could not pull reset high (%d)", ret);
			return ret;
		}
		k_sleep(K_MSEC(150));
	}

	return 0;
}

static int co5300_setup_tear_effect(const struct device *dev)
{
	const struct co5300_config *config = dev->config;
	struct co5300_data *data = dev->data;
	int ret = 0;

	if (config->tear_effect_gpios.port != NULL) {
		ret = gpio_pin_configure_dt(&config->tear_effect_gpios, GPIO_INPUT);
		if (ret < 0) {
			LOG_ERR("Could not configure TE GPIO (%d)", ret);
			return ret;
		}

		ret = gpio_pin_interrupt_configure_dt(&config->tear_effect_gpios,
				GPIO_INT_EDGE_TO_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Could not configure TE interrupt (%d)", ret);
			return ret;
		}

		gpio_init_callback(&data->tear_effect_gpio_cb, co5300_tear_effect_isr_handler,
				BIT(config->tear_effect_gpios.pin));
		ret = gpio_add_callback(config->tear_effect_gpios.port, &data->tear_effect_gpio_cb);
		if (ret < 0) {
			LOG_ERR("Could not add TE gpio callback");
			return ret;
		}

		/* Setup semaphore for using the tear effect pin */
		k_sem_init(&data->tear_effect_sem, 0, 1);
	}

	return 0;
}

static int co5300_init(const struct device *dev)
{
	const struct co5300_config *config = dev->config;
	struct co5300_data *data = dev->data;
	struct mipi_dsi_device mdev = {0};
	struct display_cmds lcm_init_settings = {0};
	uint8_t *ptr_to_cmd_register = 0;
	uint8_t *ptr_to_last_cmd = 0;
	uint8_t cmd_params = 0;
	uint8_t cmd_param_size = 0;
	uint8_t cmd_register = 0;
	int ret = 0;

	/* Attach to MIPI DSI host */
	mdev.data_lanes = config->num_of_lanes;
	mdev.pixfmt = data->pixel_format;
	ret = mipi_dsi_attach(config->mipi_dsi, config->channel, &mdev);
	if (ret < 0) {
		LOG_ERR("Could not attach to MIPI-DSI host");
		return ret;
	}

	/* Perform GPIO Reset */
	ret = co5300_reset(dev);
	if (ret < 0) {
		return ret;
	}

	/* Set the LCM init settings. */
	lcm_init_settings.cmd_code = lcm_init_cmds;
	lcm_init_settings.size = ARRAY_SIZE(lcm_init_cmds);
	ptr_to_cmd_register = lcm_init_settings.cmd_code;
	ptr_to_last_cmd = lcm_init_settings.cmd_code + lcm_init_settings.size;
	while (ptr_to_cmd_register < ptr_to_last_cmd) {
		/*
		 * Walk through the display_cmds array,
		 * incrementing the ptr by the param size.
		 */
		cmd_register = *ptr_to_cmd_register++;
		cmd_param_size = *ptr_to_cmd_register++;
		cmd_params = *ptr_to_cmd_register;
		ptr_to_cmd_register += cmd_param_size;

		ret = mipi_dsi_dcs_write(config->mipi_dsi, config->channel,
				cmd_register, &cmd_params, cmd_param_size);
		if (ret < 0) {
			return ret;
		}
	}

	/* Set pixel format */
	if (data->pixel_format == MIPI_DSI_PIXFMT_RGB888) {
		co5300_set_pixel_format(dev, PIXEL_FORMAT_RGB_888);
	} else if (data->pixel_format == MIPI_DSI_PIXFMT_RGB565) {
		co5300_set_pixel_format(dev, PIXEL_FORMAT_RGB_565);
	} else {
		/* Unsupported pixel format */
		LOG_ERR("Pixel format not supported");
		return -ENOTSUP;
	}

	/* Delay 50 ms before exiting sleep mode */
	k_sleep(K_MSEC(50));
	ret = mipi_dsi_dcs_write(config->mipi_dsi, config->channel,
				MIPI_DCS_EXIT_SLEEP_MODE, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	/* After the monitor is directed to go to sleep, commands should be delayed 150ms */
	k_sleep(K_MSEC(150));

	/* Setup backlight */
	if (config->backlight_gpios.port != NULL) {
		ret = gpio_pin_configure_dt(&config->backlight_gpios, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Could not configure bl GPIO (%d)", ret);
			return ret;
		}
	}

	/* Setup tear effect pin and callback */
	ret = co5300_setup_tear_effect(dev);
	if (ret < 0) {
		return ret;
	}

	/* Clear frame buffer. */
	memset(data->frame_ptr, 0,
		config->panel_height * data->frame_pitch * data->bytes_per_pixel);

	/* Enable display */
	return mipi_dsi_dcs_write(config->mipi_dsi, config->channel,
				MIPI_DCS_SET_DISPLAY_ON, NULL, 0);
}

static DEVICE_API(display, co5300_api) = {
	.blanking_on = co5300_blanking_on,
	.blanking_off = co5300_blanking_off,
	.write = co5300_write,
	.set_brightness = co5300_set_brightness,
	.get_capabilities = co5300_get_capabilities,
	.set_pixel_format = co5300_set_pixel_format,
	.set_orientation = co5300_set_orientation,
};

/* Place the frame buffer in secondary RAM if specified, otherwise use default RAM */
#define CO5300_FRAMEBUFFER_PLACEMENT(node_id)							\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(node_id, ext_ram),					\
	(Z_GENERIC_SECTION(LINKER_DT_NODE_REGION_NAME(DT_INST_PHANDLE(node_id, ext_ram)))), ())

#define CO5300_FRAMEBUFFER_DECL(node_id)							\
	CO5300_FRAMEBUFFER_PLACEMENT(node_id) static uint8_t					\
		__aligned(DT_INST_PROP(node_id, addr_align))					\
		co5300_frame_buffer_##node_id[DT_INST_PROP(node_id, height) * 3U *		\
		ROUND_UP(DT_INST_PROP(node_id, width), DT_INST_PROP(node_id, pitch_align))]

#define CO5300_FRAMEBUFFER(node_id) co5300_frame_buffer_##node_id

#define CO5300_DEVICE_INIT(node_id)								\
	static const struct co5300_config co5300_config_##node_id = {				\
		.mipi_dsi = DEVICE_DT_GET(DT_INST_BUS(node_id)),				\
		.num_of_lanes = DT_INST_PROP_BY_IDX(node_id, data_lanes, 0),			\
		.channel = DT_INST_REG_ADDR(node_id),						\
		.reset_gpios = GPIO_DT_SPEC_INST_GET_OR(node_id, reset_gpios, {0}),		\
		.power_gpios = GPIO_DT_SPEC_INST_GET_OR(node_id, power_gpios, {0}),		\
		.backlight_gpios = GPIO_DT_SPEC_INST_GET_OR(node_id, backlight_gpios, {0}),	\
		.tear_effect_gpios = GPIO_DT_SPEC_INST_GET_OR(node_id, tear_effect_gpios, {0}),	\
		.panel_width = DT_INST_PROP(node_id, width),					\
		.panel_height = DT_INST_PROP(node_id, height),					\
	};											\
	CO5300_FRAMEBUFFER_DECL(node_id);							\
	static struct co5300_data co5300_data_##node_id = {					\
		.pixel_format = DT_INST_PROP(node_id, pixel_format),				\
		.frame_ptr = CO5300_FRAMEBUFFER(node_id),					\
		.frame_pitch = ROUND_UP(DT_INST_PROP(node_id, width),				\
				DT_INST_PROP(node_id, pitch_align)),				\
	};											\
	DEVICE_DT_INST_DEFINE(node_id,								\
			    &co5300_init,							\
			    0,									\
			    &co5300_data_##node_id,						\
			    &co5300_config_##node_id,						\
			    POST_KERNEL,							\
			    CONFIG_APPLICATION_INIT_PRIORITY,					\
			    &co5300_api);

DT_INST_FOREACH_STATUS_OKAY(CO5300_DEVICE_INIT)
