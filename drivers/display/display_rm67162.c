/*
 * Copyright 2023, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT raydium_rm67162

#include <zephyr/drivers/display.h>
#include <zephyr/drivers/mipi_dsi.h>
#include <zephyr/drivers/mipi_dsi/mipi_dsi_mcux_2l.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/policy.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(rm67162, CONFIG_DISPLAY_LOG_LEVEL);

/*
 * These commands are taken from NXP's MCUXpresso SDK.
 * Additional documentation is added where possible, but the
 * Manufacture command set pages are not described in the datasheet
 */
static const struct {
	uint8_t cmd;
	uint8_t param;
} rm67162_init_400x392[] = {
	/* CMD Mode switch, select manufacture command set page 0 */
	{.cmd = 0xFE, .param = 0x01},
	{.cmd = 0x06, .param = 0x62},
	{.cmd = 0x0E, .param = 0x80},
	{.cmd = 0x0F, .param = 0x80},
	{.cmd = 0x10, .param = 0x71},
	{.cmd = 0x13, .param = 0x81},
	{.cmd = 0x14, .param = 0x81},
	{.cmd = 0x15, .param = 0x82},
	{.cmd = 0x16, .param = 0x82},
	{.cmd = 0x18, .param = 0x88},
	{.cmd = 0x19, .param = 0x55},
	{.cmd = 0x1A, .param = 0x10},
	{.cmd = 0x1C, .param = 0x99},
	{.cmd = 0x1D, .param = 0x03},
	{.cmd = 0x1E, .param = 0x03},
	{.cmd = 0x1F, .param = 0x03},
	{.cmd = 0x20, .param = 0x03},
	{.cmd = 0x25, .param = 0x03},
	{.cmd = 0x26, .param = 0x8D},
	{.cmd = 0x2A, .param = 0x03},
	{.cmd = 0x2B, .param = 0x8D},
	{.cmd = 0x36, .param = 0x00},
	{.cmd = 0x37, .param = 0x10},
	{.cmd = 0x3A, .param = 0x00},
	{.cmd = 0x3B, .param = 0x00},
	{.cmd = 0x3D, .param = 0x20},
	{.cmd = 0x3F, .param = 0x3A},
	{.cmd = 0x40, .param = 0x30},
	{.cmd = 0x41, .param = 0x30},
	{.cmd = 0x42, .param = 0x33},
	{.cmd = 0x43, .param = 0x22},
	{.cmd = 0x44, .param = 0x11},
	{.cmd = 0x45, .param = 0x66},
	{.cmd = 0x46, .param = 0x55},
	{.cmd = 0x47, .param = 0x44},
	{.cmd = 0x4C, .param = 0x33},
	{.cmd = 0x4D, .param = 0x22},
	{.cmd = 0x4E, .param = 0x11},
	{.cmd = 0x4F, .param = 0x66},
	{.cmd = 0x50, .param = 0x55},
	{.cmd = 0x51, .param = 0x44},
	{.cmd = 0x57, .param = 0xB3},
	{.cmd = 0x6B, .param = 0x19},
	{.cmd = 0x70, .param = 0x55},
	{.cmd = 0x74, .param = 0x0C},

	/* VGMP/VGSP Voltage Control (select manufacture command set page 1 ) */
	{.cmd = 0xFE, .param = 0x02},
	{.cmd = 0x9B, .param = 0x40},
	{.cmd = 0x9C, .param = 0x67},
	{.cmd = 0x9D, .param = 0x20},

	/* VGMP/VGSP Voltage Control (select manufacture command set page 2 ) */
	{.cmd = 0xFE, .param = 0x03},
	{.cmd = 0x9B, .param = 0x40},
	{.cmd = 0x9C, .param = 0x67},
	{.cmd = 0x9D, .param = 0x20},

	/* VSR Command (select manufacture command set page 3 ) */
	{.cmd = 0xFE, .param = 0x04},
	{.cmd = 0x5D, .param = 0x10},

	/* VSR1 Timing Set (select manufacture command set page 3 ) */
	{.cmd = 0xFE, .param = 0x04},
	{.cmd = 0x00, .param = 0x8D},
	{.cmd = 0x01, .param = 0x00},
	{.cmd = 0x02, .param = 0x01},
	{.cmd = 0x03, .param = 0x01},
	{.cmd = 0x04, .param = 0x10},
	{.cmd = 0x05, .param = 0x01},
	{.cmd = 0x06, .param = 0xA7},
	{.cmd = 0x07, .param = 0x20},
	{.cmd = 0x08, .param = 0x00},

	/* VSR2 Timing Set (select manufacture command set page 3 ) */
	{.cmd = 0xFE, .param = 0x04},
	{.cmd = 0x09, .param = 0xC2},
	{.cmd = 0x0A, .param = 0x00},
	{.cmd = 0x0B, .param = 0x02},
	{.cmd = 0x0C, .param = 0x01},
	{.cmd = 0x0D, .param = 0x40},
	{.cmd = 0x0E, .param = 0x06},
	{.cmd = 0x0F, .param = 0x01},
	{.cmd = 0x10, .param = 0xA7},
	{.cmd = 0x11, .param = 0x00},

	/* VSR3 Timing Set (select manufacture command set page 3 ) */
	{.cmd = 0xFE, .param = 0x04},
	{.cmd = 0x12, .param = 0xC2},
	{.cmd = 0x13, .param = 0x00},
	{.cmd = 0x14, .param = 0x02},
	{.cmd = 0x15, .param = 0x01},
	{.cmd = 0x16, .param = 0x40},
	{.cmd = 0x17, .param = 0x07},
	{.cmd = 0x18, .param = 0x01},
	{.cmd = 0x19, .param = 0xA7},
	{.cmd = 0x1A, .param = 0x00},

	/* VSR4 Timing Set (select manufacture command set page 3 ) */
	{.cmd = 0xFE, .param = 0x04},
	{.cmd = 0x1B, .param = 0x82},
	{.cmd = 0x1C, .param = 0x00},
	{.cmd = 0x1D, .param = 0xFF},
	{.cmd = 0x1E, .param = 0x05},
	{.cmd = 0x1F, .param = 0x60},
	{.cmd = 0x20, .param = 0x02},
	{.cmd = 0x21, .param = 0x01},
	{.cmd = 0x22, .param = 0x7C},
	{.cmd = 0x23, .param = 0x00},

	/* VSR5 Timing Set (select manufacture command set page 3 ) */
	{.cmd = 0xFE, .param = 0x04},
	{.cmd = 0x24, .param = 0xC2},
	{.cmd = 0x25, .param = 0x00},
	{.cmd = 0x26, .param = 0x04},
	{.cmd = 0x27, .param = 0x02},
	{.cmd = 0x28, .param = 0x70},
	{.cmd = 0x29, .param = 0x05},
	{.cmd = 0x2A, .param = 0x74},
	{.cmd = 0x2B, .param = 0x8D},
	{.cmd = 0x2D, .param = 0x00},

	/* VSR6 Timing Set (select manufacture command set page 3 ) */
	{.cmd = 0xFE, .param = 0x04},
	{.cmd = 0x2F, .param = 0xC2},
	{.cmd = 0x30, .param = 0x00},
	{.cmd = 0x31, .param = 0x04},
	{.cmd = 0x32, .param = 0x02},
	{.cmd = 0x33, .param = 0x70},
	{.cmd = 0x34, .param = 0x07},
	{.cmd = 0x35, .param = 0x74},
	{.cmd = 0x36, .param = 0x8D},
	{.cmd = 0x37, .param = 0x00},

	/* VSR Marping command  (select manufacture command set page 3 ) */
	{.cmd = 0xFE, .param = 0x04},
	{.cmd = 0x5E, .param = 0x20},
	{.cmd = 0x5F, .param = 0x31},
	{.cmd = 0x60, .param = 0x54},
	{.cmd = 0x61, .param = 0x76},
	{.cmd = 0x62, .param = 0x98},

	/* Select manufacture command set page 4 */
	/* ELVSS -2.4V(RT4723). 0x15: RT4723. 0x01: RT4723B. 0x17: STAM1332. */
	{.cmd = 0xFE, .param = 0x05},
	{.cmd = 0x05, .param = 0x15},
	{.cmd = 0x2A, .param = 0x04},
	{.cmd = 0x91, .param = 0x00},

	/* Select user command set */
	{.cmd = 0xFE, .param = 0x00},
	/* Set tearing effect signal to only output at V-blank*/
	{.cmd = 0x35, .param = 0x00},
};

struct rm67162_config {
	const struct device *mipi_dsi;
	uint8_t channel;
	uint8_t num_of_lanes;
	const struct gpio_dt_spec reset_gpio;
	const struct gpio_dt_spec bl_gpio;
	const struct gpio_dt_spec te_gpio;
	uint16_t panel_width;
	uint16_t panel_height;
};


struct rm67162_data {
	uint8_t pixel_format;
	uint8_t bytes_per_pixel;
	struct gpio_callback te_gpio_cb;
	struct k_sem te_sem;
};

static void rm67162_te_isr_handler(const struct device *gpio_dev,
				   struct gpio_callback *cb, uint32_t pins)
{
	struct rm67162_data *data = CONTAINER_OF(cb, struct rm67162_data, te_gpio_cb);

	k_sem_give(&data->te_sem);
}

static int rm67162_init(const struct device *dev)
{
	const struct rm67162_config *config = dev->config;
	struct rm67162_data *data = dev->data;
	struct mipi_dsi_device mdev = {0};
	int ret;
	uint32_t i;
	uint8_t cmd, param;

	/* Attach to MIPI DSI host */
	mdev.data_lanes = config->num_of_lanes;
	mdev.pixfmt = data->pixel_format;

	ret = mipi_dsi_attach(config->mipi_dsi, config->channel, &mdev);
	if (ret < 0) {
		LOG_ERR("Could not attach to MIPI-DSI host");
		return ret;
	}

	if (config->reset_gpio.port != NULL) {
		ret = gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Could not configure reset GPIO (%d)", ret);
			return ret;
		}

		/*
		 * Power to the display has been enabled via the regulator fixed api during
		 * regulator init. Per datasheet, we must wait at least 10ms before
		 * starting reset sequence after power on.
		 */
		k_sleep(K_MSEC(10));
		/* Start reset sequence */
		ret = gpio_pin_set_dt(&config->reset_gpio, 0);
		if (ret < 0) {
			LOG_ERR("Could not pull reset low (%d)", ret);
			return ret;
		}
		/* Per datasheet, reset low pulse width should be at least 10usec */
		k_sleep(K_USEC(30));
		gpio_pin_set_dt(&config->reset_gpio, 1);
		if (ret < 0) {
			LOG_ERR("Could not pull reset high (%d)", ret);
			return ret;
		}
		/*
		 * It is necessary to wait at least 120msec after releasing reset,
		 * before sending additional commands. This delay can be 5msec
		 * if we are certain the display module is in SLEEP IN state,
		 * but this is not guaranteed (for example, with a warm reset)
		 */
		k_sleep(K_MSEC(150));
	}

	/* Now, write initialization settings for display, running at 400x392 */
	for (i = 0; i < ARRAY_SIZE(rm67162_init_400x392); i++) {
		cmd = rm67162_init_400x392[i].cmd;
		param = rm67162_init_400x392[i].param;
		ret = mipi_dsi_dcs_write(config->mipi_dsi, config->channel,
					cmd, &param, 1);
		if (ret < 0) {
			return ret;
		}
	}

	/* Set pixel format */
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

	/* Delay 50 ms before exiting sleep mode */
	k_sleep(K_MSEC(50));
	ret = mipi_dsi_dcs_write(config->mipi_dsi, config->channel,
				MIPI_DCS_EXIT_SLEEP_MODE, NULL, 0);
	if (ret < 0) {
		return ret;
	}
	/*
	 * We must wait 5 ms after exiting sleep mode before sending additional
	 * commands. If we intend to enter sleep mode, we must delay
	 * 120 ms before sending that command. To be safe, delay 150ms
	 */
	k_sleep(K_MSEC(150));

	/* Setup backlight */
	if (config->bl_gpio.port != NULL) {
		ret = gpio_pin_configure_dt(&config->bl_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Could not configure bl GPIO (%d)", ret);
			return ret;
		}
	}

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
		gpio_init_callback(&data->te_gpio_cb, rm67162_te_isr_handler,
				BIT(config->te_gpio.pin));
		gpio_add_callback(config->te_gpio.port, &data->te_gpio_cb);

		/* Setup te pin semaphore */
		k_sem_init(&data->te_sem, 0, 1);
	}

	/* Now, enable display */
	return mipi_dsi_dcs_write(config->mipi_dsi, config->channel,
				MIPI_DCS_SET_DISPLAY_ON, NULL, 0);
}

/* Helper to write framebuffer data to rm67162 via MIPI interface. */
static int rm67162_write_fb(const struct device *dev, bool first_write,
			const uint8_t *src, uint32_t len)
{
	const struct rm67162_config *config = dev->config;
	uint32_t wlen = 0;
	struct mipi_dsi_msg msg = {0};

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
	while (len > 0) {
		msg.tx_len = len;
		msg.tx_buf = src;
		wlen = mipi_dsi_transfer(config->mipi_dsi, config->channel, &msg);
		if (wlen < 0) {
			return wlen;
		}
		/* Advance source pointer and decrement remaining */
		src += wlen;
		len -= wlen;
		/* All future commands should use WRITE_MEMORY_CONTINUE */
		msg.cmd = MIPI_DCS_WRITE_MEMORY_CONTINUE;
	}
	return wlen;
}

static int rm67162_write(const struct device *dev, const uint16_t x,
			 const uint16_t y,
			 const struct display_buffer_descriptor *desc,
			 const void *buf)
{
	const struct rm67162_config *config = dev->config;
	struct rm67162_data *data = dev->data;
	int ret;
	uint16_t start, end, h_idx;
	const uint8_t *src;
	bool first_cmd;
	uint8_t param[4];

	LOG_DBG("W=%d, H=%d @%d,%d", desc->width, desc->height, x, y);

	/*
	 * RM67162 runs in MIPI DBI mode. This means we can use command mode
	 * to write to the video memory buffer on the RM67162 control IC,
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
	src = buf;
	first_cmd = true;

	if (desc->pitch == desc->width) {
		/* Buffer is contiguous, we can perform entire transfer */
		rm67162_write_fb(dev, first_cmd, src,
			desc->height * desc->width * data->bytes_per_pixel);
	} else {
		/* Buffer is not contiguous, we must write each line separately */
		for (h_idx = 0; h_idx < desc->height; h_idx++) {
			rm67162_write_fb(dev, first_cmd, src,
				desc->width * data->bytes_per_pixel);
			first_cmd = false;
			/* The pitch is not equal to width, account for it here */
			src += data->bytes_per_pixel * (desc->pitch - desc->width);
		}
	}

	return 0;
}

static void rm67162_get_capabilities(const struct device *dev,
				     struct display_capabilities *capabilities)
{
	const struct rm67162_config *config = dev->config;
	const struct rm67162_data *data = dev->data;

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

static int rm67162_blanking_off(const struct device *dev)
{
	const struct rm67162_config *config = dev->config;

	if (config->bl_gpio.port != NULL) {
		return gpio_pin_set_dt(&config->bl_gpio, 1);
	} else {
		return -ENOTSUP;
	}
}

static int rm67162_blanking_on(const struct device *dev)
{
	const struct rm67162_config *config = dev->config;

	if (config->bl_gpio.port != NULL) {
		return gpio_pin_set_dt(&config->bl_gpio, 0);
	} else {
		return -ENOTSUP;
	}
}

static int rm67162_set_pixel_format(const struct device *dev,
				    const enum display_pixel_format pixel_format)
{
	const struct rm67162_config *config = dev->config;
	struct rm67162_data *data = dev->data;
	uint8_t param;

	switch (pixel_format) {
	case PIXEL_FORMAT_RGB_565:
		data->pixel_format = MIPI_DSI_PIXFMT_RGB565;
		return 0;
	case PIXEL_FORMAT_RGB_888:
		data->pixel_format = MIPI_DSI_PIXFMT_RGB888;
		return 0;
	default:
		/* Other display formats not implemented */
		return -ENOTSUP;
	}
	if (data->pixel_format == MIPI_DSI_PIXFMT_RGB888) {
		param = MIPI_DCS_PIXEL_FORMAT_24BIT;
		data->bytes_per_pixel = 3;
	} else if (data->pixel_format == MIPI_DSI_PIXFMT_RGB565) {
		param = MIPI_DCS_PIXEL_FORMAT_16BIT;
		data->bytes_per_pixel = 2;
	}
	return mipi_dsi_dcs_write(config->mipi_dsi, config->channel,
				MIPI_DCS_SET_PIXEL_FORMAT, &param, 1);
}

static int rm67162_set_orientation(const struct device *dev,
				   const enum display_orientation orientation)
{
	if (orientation == DISPLAY_ORIENTATION_NORMAL) {
		return 0;
	}
	LOG_ERR("Changing display orientation not implemented");
	return -ENOTSUP;
}

#ifdef CONFIG_PM_DEVICE

static int rm67162_pm_action(const struct device *dev,
			     enum pm_device_action action)
{
	const struct rm67162_config *config = dev->config;
	struct rm67162_data *data = dev->data;
	struct mipi_dsi_device mdev = {0};

	mdev.data_lanes = config->num_of_lanes;
	mdev.pixfmt = data->pixel_format;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		/* Detach from the MIPI DSI controller */
		return mipi_dsi_detach(config->mipi_dsi, config->channel, &mdev);
	case PM_DEVICE_ACTION_RESUME:
		return mipi_dsi_attach(config->mipi_dsi, config->channel, &mdev);
	default:
		return -ENOTSUP;
	}
}

#endif /* CONFIG_PM_DEVICE */

static DEVICE_API(display, rm67162_api) = {
	.blanking_on = rm67162_blanking_on,
	.blanking_off = rm67162_blanking_off,
	.get_capabilities = rm67162_get_capabilities,
	.write = rm67162_write,
	.set_pixel_format = rm67162_set_pixel_format,
	.set_orientation = rm67162_set_orientation,
};

#define RM67162_PANEL(id)							\
	static const struct rm67162_config rm67162_config_##id = {		\
		.mipi_dsi = DEVICE_DT_GET(DT_INST_BUS(id)),			\
		.num_of_lanes = DT_INST_PROP_BY_IDX(id, data_lanes, 0),		\
		.channel = DT_INST_REG_ADDR(id),				\
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(id, reset_gpios, {0}),	\
		.bl_gpio = GPIO_DT_SPEC_INST_GET_OR(id, bl_gpios, {0}),		\
		.te_gpio = GPIO_DT_SPEC_INST_GET_OR(id, te_gpios, {0}),		\
		.panel_width = DT_INST_PROP(id, width),				\
		.panel_height = DT_INST_PROP(id, height),			\
	};									\
	static struct rm67162_data rm67162_data_##id = {			\
		.pixel_format = DT_INST_PROP(id, pixel_format),			\
	};									\
	PM_DEVICE_DT_INST_DEFINE(id, rm67162_pm_action);			\
	DEVICE_DT_INST_DEFINE(id,						\
			    &rm67162_init,					\
			    PM_DEVICE_DT_INST_GET(id),				\
			    &rm67162_data_##id,					\
			    &rm67162_config_##id,				\
			    POST_KERNEL,					\
			    CONFIG_APPLICATION_INIT_PRIORITY,			\
			    &rm67162_api);

DT_INST_FOREACH_STATUS_OKAY(RM67162_PANEL)
