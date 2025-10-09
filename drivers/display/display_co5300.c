/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

//@IMPORTANT @INCOMPLETE(Emilio): None of the function are checking for errors at the moment!

#define DT_DRV_COMPAT chipone_co5300

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(co5300, CONFIG_DISPLAY_LOG_LEVEL);

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/mipi_dsi/mipi_dsi_mcux_2l.h>
#include <zephyr/drivers/mipi_dsi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#include <fsl_lcdif.h>
#include <fsl_mipi_dsi.h>

/******TEMP CODE********/
#include <stdint.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int32_t b32;
/******END OF TEMP CODE********/

//#include "co5300_regs.h"

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
	enum display_pixel_format pixel_format;
	uint8_t bytes_per_pixel;
	struct gpio_callback te_gpio_cb;
	struct k_sem te_sem;
};

u8 lcm_init_cmds[] = {0xFE, 0x20, 0xF4, 0x5A, 0xF5, 0x59, 0xFE, 0x40, 0x96,
		0x00, 0xC9, 0x00, 0xFE, 0x00, 0x35, 0x00, 0x53, 0x20, 0x51,
		0xFF, 0x63, 0xFF, 0x2A, 0x00, 0x06, 0x01, 0xD7,
		0x2B, 0x00, 0x00, 0x01, 0xD1};
#define LCM_INIT_CMD_BYTE_COUNT 32


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

static int co5300_write(const struct device *dev,
		const uint16_t x, const uint16_t y,
		const struct display_buffer_descriptor *desc,
		const void *buf)
{

	const struct co5300_config *config = dev->config;
	struct co5300_data *data = dev->data;
//	const struct device *mipi_base = config->mipi_dsi;
//	enum display_pixel_format pixel_format = data->pixel_format;

	struct mipi_dsi_device mdev = {0};
	int ret = 0;

	/* Attach to MIPI DSI host */
	mdev.data_lanes = config->num_of_lanes;
	mdev.pixfmt = data->pixel_format;

	/*
	ret = mipi_dsi_attach(config->mipi_dsi, config->channel, &mdev);
	*/
	if (ret < 0) {
		//LOG_ERR("Could not attach to MIPI-DSI host");
		return ret;
	}

	return 0;

#if 0
	uint8_t cmd                   = (uint8_t)command;
	dbi_lcdif_prv_data_t *prvData = (dbi_lcdif_prv_data_t *)dbiIface->prvData;
	uint16_t height               = prvData->height;
	uint32_t stride;

	if (isInterleave)
	{
	    stride = stride_byte;
	}
	else
	{
	    stride = (uint32_t)prvData->width * (uint32_t)prvData->bytePerPixel;
	}

	/* For RGB888 the stride shall be calculated as 4 bytes per pixel. */
	if (prvData->bytePerPixel == 3U)
	{
	    LCDIF_SetFrameBufferStride(lcdif, 0, stride / 3U * 4U);
	}
	else
	{
	    LCDIF_SetFrameBufferStride(lcdif, 0, stride);
	}
#if DBI_USE_MIPI_PANEL
	MIPI_DSI_HOST_Type *dsi = prvData->dsi;
	uint8_t payloadBytePerPixel;

	if (dsi != NULL)
	{
	    prvData->stride = (uint16_t)stride;

	    /* 1 pixel sent in 1 cycle. */
	    if (prvData->dsiFormat == kDSI_DbiRGB565)
	    {
	        payloadBytePerPixel = 2U;
	    }
	    /* 2 pixel sent in 1 cycle. */
	    else if (prvData->dsiFormat == kDSI_DbiRGB332)
	    {
	        payloadBytePerPixel = 1U;
	    }
	    /* 2 pixels sent in 3 cycles. kDSI_DbiRGB888, kDSI_DbiRGB444 and kDSI_DbiRGB666 */
	    else
	    {
	        payloadBytePerPixel = 3U;
	    }

	    /* If the whole update payload exceeds the DSI max payload size, send the payload in multiple times. */
	    if ((prvData->width * prvData->height * payloadBytePerPixel) > MIPI_DSI_MAX_PAYLOAD_SIZE)
	    {
	        /* Calculate how may lines to send each time. Make sure each time the buffer address meets the align
	         * requirement. */
	        height = MIPI_DSI_MAX_PAYLOAD_SIZE / prvData->width / (uint16_t)payloadBytePerPixel;
	        while (((height * prvData->stride) & (LCDIF_FB_ALIGN - 1U)) != 0U)
	        {
	            height--;
	        }
	        prvData->heightEachWrite = height;

	        /* Point the data to the next piece. */
	        prvData->data = (uint8_t *)((uint32_t)data + (height * prvData->stride));
	    }

	    /* Set payload size. */
	    DSI_SetDbiPixelPayloadSize(dsi, (height * prvData->width * (uint16_t)payloadBytePerPixel) >> 1U);
	}

	/* Update height info. */
	prvData->heightSent = height;
#endif
	prvData->height -= height;

	/* Configure buffer position, address and area. */
	LCDIF_SetFrameBufferPosition(lcdif, 0U, 0U, 0U, prvData->width, height);
	LCDIF_DbiSelectArea(lcdif, 0, 0, 0, prvData->width - 1U, height - 1U, false);
	LCDIF_SetFrameBufferAddr(lcdif, 0, (uint32_t)data);

	/* Send command. */
	LCDIF_DbiSendCommand(lcdif, 0, cmd);

	/* Start memory transfer. */
	LCDIF_DbiWriteMem(lcdif, 0);
#endif

}

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

static void co5300_get_capabilities(const struct device *dev,
		struct display_capabilities *capabilities)
{
#if 0
/******OLD CODE********/
	capabilities->x_resolution = 256;
	capabilities->y_resolution = 466;
	//@TODO(Emilio): capabilities->pixel_formats = PIXEL_FORMAT_*** | *** | ***;
//	capabilities->pixel_formats = PIXEL_FORMAT_RGB_565;
	//@TODO(Emilio): capabilities->screen_info = SCREEN_INFO_MONO*** | *** | ***;
	//@TODO(Emilio): Find the screen info here.
//	capabilities->current_pixel_format = PXIEL_FORMAT_RGB_565;
//	capabilities->current_orientation = DISPLAY_ORIENTATION_NORMAL;
/******END OF OLD CODE********/
#endif

	const struct co5300_config *config = dev->config;
//	const struct co5300_data *data = dev->data;

	memset(capabilities, 0, sizeof(struct display_capabilities));
	capabilities->x_resolution = config->panel_width;
	capabilities->y_resolution = config->panel_height;
	capabilities->supported_pixel_formats = PIXEL_FORMAT_RGB_565 |
						PIXEL_FORMAT_RGB_888;
//	switch (data->pixel_format) {
	//@INCOMPLETE @HARDCODE(Emilio): Fix this later.
	switch (MIPI_DSI_PIXFMT_RGB565) {
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

static int co5300_set_pixel_format(const struct device *dev,
		const enum display_pixel_format pixel_format)
{
//	const struct co5300_config *config = dev->config;
	struct co5300_data *data = dev->data;
	uint8_t param;

	switch (pixel_format) {
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
	/*
	return mipi_dsi_dcs_write(config->mipi_dsi, config->channel,
				MIPI_DCS_SET_PIXEL_FORMAT, &param, 1);
				*/
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

static int co5300_init(const struct device *dev)
{
	const struct co5300_config *config = dev->config;
	struct co5300_data *data = dev->data;
//	u32 iter;
//	u32 Result = 1;
//	enum display_pixel_format zephyr_pixel_format;
//	const struct mipi_device *mipi_device = config->mipi_dsi;

#if 0
	//@TODO(Emilio): Coming from display_co5300 unsure if
	//	api requires this dual attachment paradigm.
	ret = mipi_dsi_attach(config->mipi_dsi, config->channel, &mdev);
	if (ret < 0) {
		LOG_ERR("Could not attach to MIPI-DSI host");
		return ret;
	}
#endif
	if (config->power_gpio.port != NULL) {
		int ret = gpio_pin_configure_dt(&config->power_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
		//	LOG_ERR("Could not configure power GPIO (%d)", ret);
			return ret;
		}

		//@NOTE(Emilio): Power On Sequence
		ret = gpio_pin_set_dt(&config->power_gpio, 1);
		if (ret < 0) {
		//	LOG_ERR("Could not pull power high (%d)", ret);
			return ret;
		}

		k_sleep(K_MSEC(100));

		if (config->reset_gpio.port != NULL) {
			ret = gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_INACTIVE);
			if (ret < 0) {
		//		LOG_ERR("Could not configure reset GPIO (%d)", ret);
				return ret;
			}

			ret = gpio_pin_set_dt(&config->reset_gpio, 0);
			if (ret < 0) {
		//		LOG_ERR("Could not pull reset low (%d)", ret);
				return ret;
			}

			k_sleep(K_MSEC(1));
			gpio_pin_set_dt(&config->reset_gpio, 1);
			if (ret < 0) {
		//		LOG_ERR("Could not pull reset high (%d)", ret);
				return ret;
			}
			k_sleep(K_MSEC(150));
		}
	}

	int ret = 0;
	struct display_cmds lcm_init_settings= {};
	lcm_init_settings.cmd_code = lcm_init_cmds;
	lcm_init_settings.size = LCM_INIT_CMD_BYTE_COUNT;
	/* Set the LCM init settings. */
	//for (int i = 0; i < ARRAY_SIZE(lcm_init_settings); i++) {
	u8* curr_cmd = lcm_init_settings.cmd_code;
	for (int i = 0; i < lcm_init_settings.size;) {
		//@TODO(Emilio): rename and make correct cmds
		u8 cmd_code = *curr_cmd++;
		i++;
		u8 param = *curr_cmd++;
		i++;
		param++; cmd_code++;
		/*
		int ret = mipi_dsi_dcs_write(config->mipi_dsi, config->channel,
					cmd, &param, 1);
					*/
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
//		LOG_ERR("Pixel format not supported");
		return -ENOTSUP;
	}
	/*
	ret = mipi_dsi_dcs_write(config->mipi_dsi, config->channel,
				MIPI_DCS_SET_PIXEL_FORMAT, &param, 1);
				*/
	if (ret < 0) {
		return ret;
	}

	/* Delay 50 ms before exiting sleep mode */
	k_sleep(K_MSEC(50));

	/*
	ret = mipi_dsi_dcs_write(config->mipi_dsi, config->channel,
				MIPI_DCS_EXIT_SLEEP_MODE, NULL, 0);
				*/
	if (ret < 0) {
		return ret;
	}
	/*
	 * We must wait 5 ms after exiting sleep mode before sending additional
	 * commands. If we intend to enter sleep mode, we must delay
	 * 120 ms before sending that command. To be safe, delay 150ms
	 */
	k_sleep(K_MSEC(150));

	return 0;
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

/******OLD CODE********/
#if 0
#define CO5300_DEVICE_INIT(node_id)									\
	static struct co5300_data co5300_data##node_id;							\
	static const struct co5300_config co5300_config##node_id = {					\
		.mipi_dsi = DEVICE_DT_GET(DT_INST_BUS(node_id)),					\
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(node_id, reset_gpios, {0}),			\
		.bl_gpio = GPIO_DT_SPEC_INST_GET_OR(node_id, bl_gpios, {0}),				\
		.te_gpio = GPIO_DT_SPEC_INST_GET_OR(node_id, te_gpios, {0}),				\
	};												\
													\
	DEVICE_DT_INST_DEFINE(node_id, co5300_init, NULL,						\
			&co5300_data##node_id, &co5300_config##node_id,					\
			POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY, &co5300_driver_api);
#endif
/******END OF OLD CODE********/


#define CO5300_DEVICE_INIT(node_id)								\
	static const struct co5300_config co5300_config_##node_id = {				\
		.mipi_dsi = DEVICE_DT_GET(DT_INST_BUS(node_id)),					\
		.num_of_lanes = DT_INST_PROP_BY_IDX(node_id, data_lanes, 0),				\
		.channel = DT_INST_REG_ADDR(node_id),						\
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(node_id, reset_gpios, {0}),			\
		.backlight_gpio = GPIO_DT_SPEC_INST_GET_OR(node_id, backlight_gpios, {0}),				\
		.tear_effect_gpio = GPIO_DT_SPEC_INST_GET_OR(node_id, tear_effect_gpios, {0}),				\
		.panel_width = DT_INST_PROP(node_id, width),						\
		.panel_height = DT_INST_PROP(node_id, height),					\
	};											\
	static struct co5300_data co5300_data_##node_id = {						\
	};											\
	DEVICE_DT_INST_DEFINE(node_id,								\
			    &co5300_init,							\
			    0,								\
			    &co5300_data_##node_id,							\
			    &co5300_config_##node_id,						\
			    POST_KERNEL,							\
			    CONFIG_APPLICATION_INIT_PRIORITY,					\
			    &co5300_api);


DT_INST_FOREACH_STATUS_OKAY(CO5300_DEVICE_INIT)
