/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT raydium_rm67199

#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mipi_dsi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(rm67199, CONFIG_DISPLAY_LOG_LEVEL);

/* RM67199 MIPI DSI Display Controller Commands */

/* Basic Commands */
#define RM67199_NOP     (0x00)
#define RM67199_SWRESET (0x01)

/* Read Device Information */
#define RM67199_RDDID   (0x04)
#define RM67199_RDNUMED (0x05)

/* Read Display Status */
#define RM67199_RDDPM     (0x0A)
#define RM67199_RDDMADCTR (0x0B)
#define RM67199_RDDCOLMOD (0x0C)
#define RM67199_RDDIM     (0x0D)
#define RM67199_RDDSM     (0x0E)
#define RM67199_RDDSDR    (0x0F)

/* Sleep Mode Control */
#define RM67199_SLPIN  (0x10)
#define RM67199_SLPOUT (0x11)

/* Display Control */
#define RM67199_INVOFF  (0x20)
#define RM67199_INVON   (0x21)
#define RM67199_ALLPOFF (0x22)
#define RM67199_DISPOFF (0x28)
#define RM67199_DISPON  (0x29)

/* Tearing Effect Control */
#define RM67199_TEOFF (0x34)
#define RM67199_TEON  (0x35)

/* Display Configuration */
#define RM67199_MADCTR (0x36)
#define RM67199_IDMOFF (0x38)
#define RM67199_IDMON  (0x39)
#define RM67199_COLMOD (0x3A)

/* Scan Line Control */
#define RM67199_STES (0x44)
#define RM67199_GSL  (0x45)

/* Brightness Control */
#define RM67199_RDDISBV (0x52)

/* Color Enhancement */
#define RM67199_WRCE1 (0x5A)
#define RM67199_WRCE2 (0x5C)
#define RM67199_RDCE2 (0x5D)

/* Timer and Panel Control */
#define RM67199_WRTMR (0x62)
#define RM67199_RDTMR (0x63)
#define RM67199_WRPA  (0x64)
#define RM67199_RDPA  (0x65)
#define RM67199_WRWB  (0x66)
#define RM67199_RDWB  (0x67)

/* DDB and Checksum */
#define RM67199_RDFC (0xAA)
#define RM67199_RDCC (0xAF)

/* DSI Configuration */
#define RM67199_SETDSIMODE (0xC2)

/* Manufacturer Commands */
#define RM67199_WRMAUCCTR (0xFE)

/*
 * These commands are taken from NXP's MCUXpresso SDK.
 * Additional documentation is added where possible, but the
 * Manufacture command set pages are not described in the datasheet
 */
static const struct {
	uint8_t cmd;
	uint8_t param;
} rm67199_init_setting[] = {
	{.cmd = RM67199_WRMAUCCTR, .param = 0xA0},
	{.cmd = 0x2B, .param = 0x18},
	{.cmd = RM67199_WRMAUCCTR, .param = 0x70},
	{.cmd = 0x7D, .param = 0x05},
	{.cmd = RM67199_RDCE2, .param = 0x0A},
	{.cmd = RM67199_WRCE1, .param = 0x79},
	{.cmd = RM67199_WRCE2, .param = 0x00},
	{.cmd = RM67199_RDDISBV, .param = 0x00},
	{.cmd = RM67199_WRMAUCCTR, .param = 0xD0},
	{.cmd = 0x40, .param = 0x02},
	{.cmd = 0x13, .param = 0x40},
	{.cmd = RM67199_WRMAUCCTR, .param = 0x40},
	{.cmd = RM67199_RDNUMED, .param = 0x08},
	{.cmd = 0x06, .param = 0x08},
	{.cmd = 0x08, .param = 0x08},
	{.cmd = 0x09, .param = 0x08},
	{.cmd = RM67199_RDDPM, .param = 0xCA},
	{.cmd = RM67199_RDDMADCTR, .param = 0x88},
	{.cmd = RM67199_INVOFF, .param = 0x93},
	{.cmd = RM67199_INVON, .param = 0x93},
	{.cmd = 0x24, .param = 0x02},
	{.cmd = 0x26, .param = 0x02},
	{.cmd = RM67199_DISPOFF, .param = 0x05},
	{.cmd = 0x2A, .param = 0x05},
	{.cmd = 0x74, .param = 0x2F},
	{.cmd = 0x75, .param = 0x1E},
	{.cmd = 0xAD, .param = 0x00},
	{.cmd = RM67199_WRMAUCCTR, .param = 0x60},
	{.cmd = 0x00, .param = 0xCC},
	{.cmd = 0x01, .param = 0x00},
	{.cmd = 0x02, .param = 0x04},
	{.cmd = 0x03, .param = 0x00},
	{.cmd = 0x04, .param = 0x00},
	{.cmd = RM67199_RDNUMED, .param = 0x07},
	{.cmd = 0x06, .param = 0x00},
	{.cmd = 0x07, .param = 0x88},
	{.cmd = 0x08, .param = 0x00},
	{.cmd = 0x09, .param = 0xCC},
	{.cmd = RM67199_RDDPM, .param = 0x00},
	{.cmd = RM67199_RDDMADCTR, .param = 0x04},
	{.cmd = 0x0C, .param = 0x00},
	{.cmd = 0x0D, .param = 0x00},
	{.cmd = 0x0E, .param = 0x05},
	{.cmd = 0x0F, .param = 0x00},
	{.cmd = 0x10, .param = 0x88},
	{.cmd = 0x11, .param = 0x00},
	{.cmd = 0x12, .param = 0xCC},
	{.cmd = 0x13, .param = 0x0F},
	{.cmd = 0x14, .param = 0xFF},
	{.cmd = 0x15, .param = 0x04},
	{.cmd = 0x16, .param = 0x00},
	{.cmd = 0x17, .param = 0x06},
	{.cmd = 0x18, .param = 0x00},
	{.cmd = 0x19, .param = 0x96},
	{.cmd = 0x1A, .param = 0x00},
	{.cmd = 0x24, .param = 0xCC},
	{.cmd = 0x25, .param = 0x00},
	{.cmd = 0x26, .param = 0x02},
	{.cmd = 0x27, .param = 0x00},
	{.cmd = RM67199_DISPOFF, .param = 0x00},
	{.cmd = RM67199_DISPON, .param = 0x06},
	{.cmd = 0x2A, .param = 0x06},
	{.cmd = 0x2B, .param = 0x82},
	{.cmd = 0x2D, .param = 0x00},
	{.cmd = 0x2F, .param = 0xCC},
	{.cmd = 0x30, .param = 0x00},
	{.cmd = 0x31, .param = 0x02},
	{.cmd = 0x32, .param = 0x00},
	{.cmd = 0x33, .param = 0x00},
	{.cmd = RM67199_TEOFF, .param = 0x07},
	{.cmd = RM67199_TEON, .param = 0x06},
	{.cmd = RM67199_MADCTR, .param = 0x82},
	{.cmd = 0x37, .param = 0x00},
	{.cmd = RM67199_IDMOFF, .param = 0xCC},
	{.cmd = RM67199_IDMON, .param = 0x00},
	{.cmd = RM67199_COLMOD, .param = 0x02},
	{.cmd = 0x3B, .param = 0x00},
	{.cmd = 0x3D, .param = 0x00},
	{.cmd = 0x3F, .param = 0x07},
	{.cmd = 0x40, .param = 0x00},
	{.cmd = 0x41, .param = 0x88},
	{.cmd = 0x42, .param = 0x00},
	{.cmd = 0x43, .param = 0xCC},
	{.cmd = RM67199_STES, .param = 0x00},
	{.cmd = RM67199_GSL, .param = 0x02},
	{.cmd = 0x46, .param = 0x00},
	{.cmd = 0x47, .param = 0x00},
	{.cmd = 0x48, .param = 0x06},
	{.cmd = 0x49, .param = 0x02},
	{.cmd = 0x4A, .param = 0x8A},
	{.cmd = 0x4B, .param = 0x00},
	{.cmd = 0x5F, .param = 0xCA},
	{.cmd = 0x60, .param = 0x01},
	{.cmd = 0x61, .param = 0xE8},
	{.cmd = RM67199_WRTMR, .param = 0x09},
	{.cmd = RM67199_RDTMR, .param = 0x00},
	{.cmd = RM67199_WRPA, .param = 0x07},
	{.cmd = RM67199_RDPA, .param = 0x00},
	{.cmd = RM67199_WRWB, .param = 0x30},
	{.cmd = RM67199_RDWB, .param = 0x80},
	{.cmd = 0x9B, .param = 0x03},
	{.cmd = 0xA9, .param = 0x07},
	{.cmd = RM67199_RDFC, .param = 0x06},
	{.cmd = 0xAB, .param = 0x02},
	{.cmd = 0xAC, .param = 0x10},
	{.cmd = 0xAD, .param = 0x11},
	{.cmd = 0xAE, .param = 0x05},
	{.cmd = RM67199_RDCC, .param = 0x04},
	{.cmd = 0xB0, .param = 0x10},
	{.cmd = 0xB1, .param = 0x10},
	{.cmd = 0xB2, .param = 0x10},
	{.cmd = 0xB3, .param = 0x10},
	{.cmd = 0xB4, .param = 0x10},
	{.cmd = 0xB5, .param = 0x10},
	{.cmd = 0xB6, .param = 0x10},
	{.cmd = 0xB7, .param = 0x10},
	{.cmd = 0xB8, .param = 0x10},
	{.cmd = 0xB9, .param = 0x10},
	{.cmd = 0xBA, .param = 0x04},
	{.cmd = 0xBB, .param = 0x05},
	{.cmd = 0xBC, .param = 0x00},
	{.cmd = 0xBD, .param = 0x01},
	{.cmd = 0xBE, .param = 0x0A},
	{.cmd = 0xBF, .param = 0x10},
	{.cmd = 0xC0, .param = 0x11},
	{.cmd = RM67199_WRMAUCCTR, .param = 0xA0},
	{.cmd = RM67199_ALLPOFF, .param = 0x00},
};

struct rm67199_config {
	const struct device *mipi_dsi;
	uint8_t channel;
	uint8_t num_of_lanes;
	const struct gpio_dt_spec reset_gpio;
	const struct gpio_dt_spec bl_gpio;
};

struct rm67199_data {
	uint8_t pixel_format;
	uint8_t bytes_per_pixel;
	struct k_sem te_sem;
};

static int rm67199_init(const struct device *dev)
{
	const struct rm67199_config *config = dev->config;
	struct rm67199_data *data = dev->data;
	struct mipi_dsi_device mdev = {0};
	int ret;
	uint32_t i;
	uint8_t buf[2];

	LOG_INF("starting RM67199 init");

	/* Attach to MIPI DSI host */
	mdev.data_lanes = config->num_of_lanes;
	mdev.pixfmt = data->pixel_format;
	mdev.mode_flags = MIPI_DSI_MODE_VIDEO;

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
		 * starting reset sequence after power-on.
		 */
		k_sleep(K_MSEC(10));
		/* Start reset sequence */
		ret = gpio_pin_set_dt(&config->reset_gpio, 0);
		if (ret < 0) {
			LOG_ERR("Could not pull reset low (%d)", ret);
			return ret;
		}
		/* Per datasheet, reset low pulse width should be at least 10usec */
		k_sleep(K_USEC(10));
		ret = gpio_pin_set_dt(&config->reset_gpio, 1);
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

	/* Write initialization settings for display */
	for (i = 0; i < ARRAY_SIZE(rm67199_init_setting); i++) {
		buf[0] = rm67199_init_setting[i].cmd;
		buf[1] = rm67199_init_setting[i].param;
		ret = mipi_dsi_generic_write(config->mipi_dsi, config->channel, buf, 2);
		if (ret < 0) {
			return ret;
		}
	}

	/* Change to send user command. */
	buf[0] = RM67199_WRMAUCCTR;
	buf[1] = 0x00;
	ret = mipi_dsi_generic_write(config->mipi_dsi, config->channel, buf, 2);
	if (ret < 0) {
		return ret;
	}

	/* Set DSI mode */
	buf[0] = RM67199_SETDSIMODE;
	buf[1] = 0x03;
	ret = mipi_dsi_generic_write(config->mipi_dsi, config->channel, buf, 2);
	if (ret < 0) {
		return ret;
	}

	/* Set pixel format */
	if (data->pixel_format == MIPI_DSI_PIXFMT_RGB888) {
		buf[1] = MIPI_DCS_PIXEL_FORMAT_24BIT;
		data->bytes_per_pixel = 3;
	} else if (data->pixel_format == MIPI_DSI_PIXFMT_RGB565) {
		buf[1] = MIPI_DCS_PIXEL_FORMAT_16BIT;
		data->bytes_per_pixel = 2;
	} else {
		/* Unsupported pixel format */
		LOG_ERR("Pixel format not supported");
		return -ENOTSUP;
	}
	buf[0] = MIPI_DCS_SET_PIXEL_FORMAT;
	ret = mipi_dsi_generic_write(config->mipi_dsi, config->channel, buf, 2);
	if (ret < 0) {
		return ret;
	}

	/* Brightness. */
	buf[0] = MIPI_DCS_SET_DISPLAY_BRIGHTNESS;
	buf[1] = 0xFF;
	ret = mipi_dsi_generic_write(config->mipi_dsi, config->channel, buf, 2);
	if (ret < 0) {
		return ret;
	}

	/* Delay 50 ms before exiting sleep mode */
	k_sleep(K_MSEC(50));
	buf[0] = MIPI_DCS_EXIT_SLEEP_MODE;
	ret = mipi_dsi_generic_write(config->mipi_dsi, config->channel, &buf[0], 1);
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

	/* Now, enable display */
	buf[0] = MIPI_DCS_SET_DISPLAY_ON;
	ret = mipi_dsi_generic_write(config->mipi_dsi, config->channel, &buf[0], 1);
	if (ret < 0) {
		LOG_ERR("%s failed", __func__);
	} else {
		LOG_INF("%s succeeded", __func__);
	}

	return ret;
}

static int rm67199_blanking_off(const struct device *dev)
{
	const struct rm67199_config *config = dev->config;

	if (config->bl_gpio.port != NULL) {
		return gpio_pin_set_dt(&config->bl_gpio, 1);
	} else {
		return -ENOTSUP;
	}
}

static int rm67199_blanking_on(const struct device *dev)
{
	const struct rm67199_config *config = dev->config;

	if (config->bl_gpio.port != NULL) {
		return gpio_pin_set_dt(&config->bl_gpio, 0);
	} else {
		return -ENOTSUP;
	}
}

static int rm67199_set_pixel_format(const struct device *dev,
				    const enum display_pixel_format pixel_format)
{
	const struct rm67199_config *config = dev->config;
	struct rm67199_data *data = dev->data;
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

	return mipi_dsi_dcs_write(config->mipi_dsi, config->channel, MIPI_DCS_SET_PIXEL_FORMAT,
				  &param, 1);
}

static int rm67199_set_orientation(const struct device *dev,
				   const enum display_orientation orientation)
{
	ARG_UNUSED(dev);

	if (orientation == DISPLAY_ORIENTATION_NORMAL) {
		return 0;
	}
	LOG_ERR("Changing display orientation not implemented");
	return -ENOTSUP;
}

static const struct display_driver_api rm67199_api = {
	.blanking_on = rm67199_blanking_on,
	.blanking_off = rm67199_blanking_off,
	.set_pixel_format = rm67199_set_pixel_format,
	.set_orientation = rm67199_set_orientation,
};

#define RM67199_PANEL(id)                                                                          \
	static const struct rm67199_config rm67199_config_##id = {                                 \
		.mipi_dsi = DEVICE_DT_GET(DT_INST_BUS(id)),                                        \
		.channel = DT_INST_REG_ADDR(id),                                                   \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(id, reset_gpios, {0}),                      \
		.bl_gpio = GPIO_DT_SPEC_INST_GET_OR(id, bl_gpios, {0}),                            \
		.num_of_lanes = DT_INST_PROP_BY_IDX(id, data_lanes, 0),                            \
	};                                                                                         \
	static struct rm67199_data rm67199_data_##id = {                                           \
		.pixel_format = DT_INST_PROP(id, pixel_format),                                    \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(id, &rm67199_init, NULL, &rm67199_data_##id, &rm67199_config_##id,   \
			      POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY, &rm67199_api);

DT_INST_FOREACH_STATUS_OKAY(RM67199_PANEL)
