/*
 * Copyright 2022, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT raydium_rm68200

#include <zephyr/drivers/display.h>
#include <zephyr/drivers/mipi_dsi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(rm68200, CONFIG_DISPLAY_LOG_LEVEL);

/* DCS Commands */
#define DCS_CMD_PAGE		0xFE
#define DCS_CMD_PAGE_UCS	0x0
#define DCS_CMD_PAGE_SET_PAGE0	0x1
#define DCS_CMD_PAGE_SET_PAGE1	0x2
#define DCS_CMD_PAGE_SET_PAGE2	0x3
#define DCS_CMD_PAGE_SET_PAGE3	0x4

/* MCS Commands */
#define MCS_STBCTR		0x12
#define MCS_SGOPCTR		0x16
#define MCS_SDCTR		0x1A
#define MCS_INVCTR		0x1B
#define MCS_EXT_PWR_IC_TYPE	0x24
#define MCS_EXT_PWR_SET_AVDD	0x25
#define MCS_AVEE_FROM_PFM	0x26
#define MCS_AVDD_FROM_PFM	0x27
#define MCS_SETAVEE		0x29
#define MCS_BT2CTR		0x2B
#define MCS_BT3CTR		0x2F
#define MCS_BT4CTR		0x34
#define MCS_VCMCTR		0x46
#define MCS_SETVGMN		0x52
#define MCS_SETVGSN		0x53
#define MCS_SETVGMP		0x54
#define MCS_SETVGSP		0x55
#define MCS_SW_CTRL		0x5F
#define MCS_GAMMA_VP1		0x60
#define MCS_GAMMA_VP4		0x61
#define MCS_GAMMA_VP8		0x62
#define MCS_GAMMA_VP16		0x63
#define MCS_GAMMA_VP24		0x64
#define MCS_GAMMA_VP52		0x65
#define MCS_GAMMA_VP80		0x66
#define MCS_GAMMA_VP108		0x67
#define MCS_GAMMA_VP147		0x68
#define MCS_GAMMA_VP175		0x69
#define MCS_GAMMA_VP203		0x6A
#define MCS_GAMMA_VP231		0x6B
#define MCS_GAMMA_VP239		0x6C
#define MCS_GAMMA_VP247		0x6D
#define MCS_GAMMA_VP251		0x6E
#define MCS_GAMMA_VP255		0x6F
#define MCS_GAMMA_VN1		0x70
#define MCS_GAMMA_VN4		0x71
#define MCS_GAMMA_VN8		0x72
#define MCS_GAMMA_VN16		0x73
#define MCS_GAMMA_VN24		0x74
#define MCS_GAMMA_VN52		0x75
#define MCS_GAMMA_VN80		0x76
#define MCS_GAMMA_VN108		0x77
#define MCS_GAMMA_VN147		0x78
#define MCS_GAMMA_VN175		0x79
#define MCS_GAMMA_VN203		0x7A
#define MCS_GAMMA_VN231		0x7B
#define MCS_GAMMA_VN239		0x7C
#define MCS_GAMMA_VN247		0x7D
#define MCS_GAMMA_VN251		0x7E
#define MCS_GAMMA_VN255		0x7F
#define MCS_GAMMA_UPDATE	0x80

struct rm68200_config {
	const struct device *mipi_dsi;
	const struct gpio_dt_spec reset_gpio;
	const struct gpio_dt_spec bl_gpio;
	uint8_t num_of_lanes;
	uint8_t pixel_format;
	uint16_t panel_width;
	uint16_t panel_height;
	uint8_t channel;
};

static int rm68200_dcs_write(const struct device *dev, uint8_t cmd, uint8_t *buf,
				 uint8_t len)
{
	const struct rm68200_config *config = dev->config;

	return mipi_dsi_dcs_write(config->mipi_dsi, config->channel, cmd, buf, len);
}

static int rm68200_write(const struct device *dev, const uint16_t x,
			 const uint16_t y,
			 const struct display_buffer_descriptor *desc,
			 const void *buf)
{
	return 0;
}

static int rm68200_blanking_off(const struct device *dev)
{
	const struct rm68200_config *config = dev->config;

	if (config->bl_gpio.port != NULL) {
		return gpio_pin_set_dt(&config->bl_gpio, 1);
	} else {
		return -ENOTSUP;
	}
}

static int rm68200_blanking_on(const struct device *dev)
{
	const struct rm68200_config *config = dev->config;

	if (config->bl_gpio.port != NULL) {
		return gpio_pin_set_dt(&config->bl_gpio, 0);
	} else {
		return -ENOTSUP;
	}
}

static int rm68200_set_pixel_format(const struct device *dev,
				    const enum display_pixel_format pixel_format)
{
	const struct rm68200_config *config = dev->config;

	if (pixel_format == config->pixel_format) {
		return 0;
	}
	LOG_ERR("Pixel format change not implemented");
	return -ENOTSUP;
}

static int rm68200_set_orientation(const struct device *dev,
				   const enum display_orientation orientation)
{
	if (orientation == DISPLAY_ORIENTATION_NORMAL) {
		return 0;
	}
	LOG_ERR("Changing display orientation not implemented");
	return -ENOTSUP;
}

static void rm68200_get_capabilities(const struct device *dev,
				     struct display_capabilities *capabilities)
{
	const struct rm68200_config *config = dev->config;

	memset(capabilities, 0, sizeof(struct display_capabilities));
	capabilities->x_resolution = config->panel_width;
	capabilities->y_resolution = config->panel_height;
	capabilities->supported_pixel_formats = config->pixel_format;
	capabilities->current_pixel_format = config->pixel_format;
	capabilities->current_orientation = DISPLAY_ORIENTATION_NORMAL;
}

static const struct display_driver_api rm68200_api = {
	.blanking_on = rm68200_blanking_on,
	.blanking_off = rm68200_blanking_off,
	.write = rm68200_write,
	.get_capabilities = rm68200_get_capabilities,
	.set_pixel_format = rm68200_set_pixel_format,
	.set_orientation = rm68200_set_orientation,
};

static int rm68200_init(const struct device *dev)
{
	const struct rm68200_config *config = dev->config;
	uint8_t param;
	int ret;
	struct mipi_dsi_device mdev;

	mdev.data_lanes = config->num_of_lanes;
	mdev.pixfmt = config->pixel_format;
	/* RM68200 runs in video mode */
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

		/* Power to the display has been enabled via the regulator fixed api during
		 * regulator init.
		 * reset:0 -> reset:1
		 */
		gpio_pin_set_dt(&config->reset_gpio, 0);
		/* Per datasheet, reset low pulse width should be at least 15usec */
		k_sleep(K_USEC(50));
		gpio_pin_set_dt(&config->reset_gpio, 1);
		/* Per datasheet, it is necessary to wait 5msec after releasing reset */
		k_sleep(K_MSEC(5));
	}

	param = DCS_CMD_PAGE_SET_PAGE0;
	rm68200_dcs_write(dev, DCS_CMD_PAGE, &param, 1);

	param = 0xC0;
	rm68200_dcs_write(dev, MCS_EXT_PWR_IC_TYPE, &param, 1);

	param = 0x53;
	rm68200_dcs_write(dev, MCS_EXT_PWR_SET_AVDD, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, MCS_AVEE_FROM_PFM, &param, 1);

	param = 0xE5;
	rm68200_dcs_write(dev, MCS_BT2CTR, &param, 1);

	param = 0x0A;
	rm68200_dcs_write(dev, MCS_AVDD_FROM_PFM, &param, 1);

	param = 0x0A;
	rm68200_dcs_write(dev, MCS_SETAVEE, &param, 1);

	param = 0x52;
	rm68200_dcs_write(dev, MCS_SGOPCTR, &param, 1);

	param = 0x53;
	rm68200_dcs_write(dev, MCS_BT3CTR, &param, 1);

	param = 0x5A;
	rm68200_dcs_write(dev, MCS_BT4CTR, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, MCS_INVCTR, &param, 1);

	param = 0x0A;
	rm68200_dcs_write(dev, MCS_STBCTR, &param, 1);

	param = 0x06;
	rm68200_dcs_write(dev, MCS_SDCTR, &param, 1);

	param = 0x56;
	rm68200_dcs_write(dev, MCS_VCMCTR, &param, 1);

	param = 0xA0;
	rm68200_dcs_write(dev, MCS_SETVGMN, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, MCS_SETVGSN, &param, 1);

	param = 0xA0;
	rm68200_dcs_write(dev, MCS_SETVGMP, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, MCS_SETVGSP, &param, 1);

	param = 0x10U | (config->num_of_lanes - 1U);
	rm68200_dcs_write(dev, MCS_SW_CTRL, &param, 1);

	param = DCS_CMD_PAGE_SET_PAGE2;
	rm68200_dcs_write(dev, DCS_CMD_PAGE, &param, 1);

	/* There is no description for the below registers in the datasheet */
	param = 0x05;
	rm68200_dcs_write(dev, 0x00, &param, 1);

	param = 0x0B;
	rm68200_dcs_write(dev, 0x02, &param, 1);

	param = 0x0F;
	rm68200_dcs_write(dev, 0x03, &param, 1);

	param = 0x7D;
	rm68200_dcs_write(dev, 0x04, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x05, &param, 1);

	param = 0x50;
	rm68200_dcs_write(dev, 0x06, &param, 1);

	param = 0x05;
	rm68200_dcs_write(dev, 0x07, &param, 1);

	param = 0x16;
	rm68200_dcs_write(dev, 0x08, &param, 1);

	param = 0x0D;
	rm68200_dcs_write(dev, 0x09, &param, 1);

	param = 0x11;
	rm68200_dcs_write(dev, 0x0A, &param, 1);

	param = 0x7D;
	rm68200_dcs_write(dev, 0x0B, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x0C, &param, 1);

	param = 0x50;
	rm68200_dcs_write(dev, 0x0D, &param, 1);

	param = 0x07;
	rm68200_dcs_write(dev, 0x0E, &param, 1);

	param = 0x08;
	rm68200_dcs_write(dev, 0x0F, &param, 1);

	param = 0x01;
	rm68200_dcs_write(dev, 0x10, &param, 1);

	param = 0x02;
	rm68200_dcs_write(dev, 0x11, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x12, &param, 1);

	param = 0x7D;
	rm68200_dcs_write(dev, 0x13, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x14, &param, 1);

	param = 0x85;
	rm68200_dcs_write(dev, 0x15, &param, 1);

	param = 0x08;
	rm68200_dcs_write(dev, 0x16, &param, 1);

	param = 0x03;
	rm68200_dcs_write(dev, 0x17, &param, 1);

	param = 0x04;
	rm68200_dcs_write(dev, 0x18, &param, 1);

	param = 0x05;
	rm68200_dcs_write(dev, 0x19, &param, 1);

	param = 0x06;
	rm68200_dcs_write(dev, 0x1A, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x1B, &param, 1);

	param = 0x7D;
	rm68200_dcs_write(dev, 0x1C, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x1D, &param, 1);

	param = 0x85;
	rm68200_dcs_write(dev, 0x1E, &param, 1);

	param = 0x08;
	rm68200_dcs_write(dev, 0x1F, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x20, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x21, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x22, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x23, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x24, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x25, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x26, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x27, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x28, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x29, &param, 1);

	param = 0x07;
	rm68200_dcs_write(dev, 0x2A, &param, 1);

	param = 0x08;
	rm68200_dcs_write(dev, 0x2B, &param, 1);

	param = 0x01;
	rm68200_dcs_write(dev, 0x2D, &param, 1);

	param = 0x02;
	rm68200_dcs_write(dev, 0x2F, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x30, &param, 1);

	param = 0x40;
	rm68200_dcs_write(dev, 0x31, &param, 1);

	param = 0x05;
	rm68200_dcs_write(dev, 0x32, &param, 1);

	param = 0x08;
	rm68200_dcs_write(dev, 0x33, &param, 1);

	param = 0x54;
	rm68200_dcs_write(dev, 0x34, &param, 1);

	param = 0x7D;
	rm68200_dcs_write(dev, 0x35, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x36, &param, 1);

	param = 0x03;
	rm68200_dcs_write(dev, 0x37, &param, 1);

	param = 0x04;
	rm68200_dcs_write(dev, 0x38, &param, 1);

	param = 0x05;
	rm68200_dcs_write(dev, 0x39, &param, 1);

	param = 0x06;
	rm68200_dcs_write(dev, 0x3A, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x3B, &param, 1);

	param = 0x40;
	rm68200_dcs_write(dev, 0x3D, &param, 1);

	param = 0x05;
	rm68200_dcs_write(dev, 0x3F, &param, 1);

	param = 0x08;
	rm68200_dcs_write(dev, 0x40, &param, 1);

	param = 0x54;
	rm68200_dcs_write(dev, 0x41, &param, 1);

	param = 0x7D;
	rm68200_dcs_write(dev, 0x42, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x43, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x44, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x45, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x46, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x47, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x48, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x49, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x4A, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x4B, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x4C, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x4D, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x4E, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x4F, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x50, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x51, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x52, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x53, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x54, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x55, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x56, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x58, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x59, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x5A, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x5B, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x5C, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x5D, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x5E, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x5F, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x60, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x61, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x62, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x63, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x64, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x65, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x66, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x67, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x68, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x69, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x6A, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x6B, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x6C, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x6D, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x6E, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x6F, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x70, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x71, &param, 1);

	param = 0x20;
	rm68200_dcs_write(dev, 0x72, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x73, &param, 1);

	param = 0x08;
	rm68200_dcs_write(dev, 0x74, &param, 1);

	param = 0x08;
	rm68200_dcs_write(dev, 0x75, &param, 1);

	param = 0x08;
	rm68200_dcs_write(dev, 0x76, &param, 1);

	param = 0x08;
	rm68200_dcs_write(dev, 0x77, &param, 1);

	param = 0x08;
	rm68200_dcs_write(dev, 0x78, &param, 1);

	param = 0x08;
	rm68200_dcs_write(dev, 0x79, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x7A, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x7B, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x7C, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x7D, &param, 1);

	param = 0xBF;
	rm68200_dcs_write(dev, 0x7E, &param, 1);

	param = 0x02;
	rm68200_dcs_write(dev, 0x7F, &param, 1);

	param = 0x06;
	rm68200_dcs_write(dev, 0x80, &param, 1);

	param = 0x14;
	rm68200_dcs_write(dev, 0x81, &param, 1);

	param = 0x10;
	rm68200_dcs_write(dev, 0x82, &param, 1);

	param = 0x16;
	rm68200_dcs_write(dev, 0x83, &param, 1);

	param = 0x12;
	rm68200_dcs_write(dev, 0x84, &param, 1);

	param = 0x08;
	rm68200_dcs_write(dev, 0x85, &param, 1);

	param = 0x3F;
	rm68200_dcs_write(dev, 0x86, &param, 1);

	param = 0x3F;
	rm68200_dcs_write(dev, 0x87, &param, 1);

	param = 0x3F;
	rm68200_dcs_write(dev, 0x88, &param, 1);

	param = 0x3F;
	rm68200_dcs_write(dev, 0x89, &param, 1);

	param = 0x3F;
	rm68200_dcs_write(dev, 0x8A, &param, 1);

	param = 0x0C;
	rm68200_dcs_write(dev, 0x8B, &param, 1);

	param = 0x0A;
	rm68200_dcs_write(dev, 0x8C, &param, 1);

	param = 0x0E;
	rm68200_dcs_write(dev, 0x8D, &param, 1);

	param = 0x3F;
	rm68200_dcs_write(dev, 0x8E, &param, 1);

	param = 0x3F;
	rm68200_dcs_write(dev, 0x8F, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0x90, &param, 1);

	param = 0x04;
	rm68200_dcs_write(dev, 0x91, &param, 1);

	param = 0x3F;
	rm68200_dcs_write(dev, 0x92, &param, 1);

	param = 0x3F;
	rm68200_dcs_write(dev, 0x93, &param, 1);

	param = 0x3F;
	rm68200_dcs_write(dev, 0x94, &param, 1);

	param = 0x3F;
	rm68200_dcs_write(dev, 0x95, &param, 1);

	param = 0x05;
	rm68200_dcs_write(dev, 0x96, &param, 1);

	param = 0x01;
	rm68200_dcs_write(dev, 0x97, &param, 1);

	param = 0x3F;
	rm68200_dcs_write(dev, 0x98, &param, 1);

	param = 0x3F;
	rm68200_dcs_write(dev, 0x99, &param, 1);

	param = 0x0F;
	rm68200_dcs_write(dev, 0x9A, &param, 1);

	param = 0x0B;
	rm68200_dcs_write(dev, 0x9B, &param, 1);

	param = 0x0D;
	rm68200_dcs_write(dev, 0x9C, &param, 1);

	param = 0x3F;
	rm68200_dcs_write(dev, 0x9D, &param, 1);

	param = 0x3F;
	rm68200_dcs_write(dev, 0x9E, &param, 1);

	param = 0x3F;
	rm68200_dcs_write(dev, 0x9F, &param, 1);

	param = 0x3F;
	rm68200_dcs_write(dev, 0xA0, &param, 1);

	param = 0x3F;
	rm68200_dcs_write(dev, 0xA2, &param, 1);

	param = 0x09;
	rm68200_dcs_write(dev, 0xA3, &param, 1);

	param = 0x13;
	rm68200_dcs_write(dev, 0xA4, &param, 1);

	param = 0x17;
	rm68200_dcs_write(dev, 0xA5, &param, 1);

	param = 0x11;
	rm68200_dcs_write(dev, 0xA6, &param, 1);

	param = 0x15;
	rm68200_dcs_write(dev, 0xA7, &param, 1);

	param = 0x07;
	rm68200_dcs_write(dev, 0xA9, &param, 1);

	param = 0x03;
	rm68200_dcs_write(dev, 0xAA, &param, 1);

	param = 0x3F;
	rm68200_dcs_write(dev, 0xAB, &param, 1);

	param = 0x3F;
	rm68200_dcs_write(dev, 0xAC, &param, 1);

	param = 0x05;
	rm68200_dcs_write(dev, 0xAD, &param, 1);

	param = 0x01;
	rm68200_dcs_write(dev, 0xAE, &param, 1);

	param = 0x17;
	rm68200_dcs_write(dev, 0xAF, &param, 1);

	param = 0x13;
	rm68200_dcs_write(dev, 0xB0, &param, 1);

	param = 0x15;
	rm68200_dcs_write(dev, 0xB1, &param, 1);

	param = 0x11;
	rm68200_dcs_write(dev, 0xB2, &param, 1);

	param = 0x0F;
	rm68200_dcs_write(dev, 0xB3, &param, 1);

	param = 0x3F;
	rm68200_dcs_write(dev, 0xB4, &param, 1);

	param = 0x3F;
	rm68200_dcs_write(dev, 0xB5, &param, 1);

	param = 0x3F;
	rm68200_dcs_write(dev, 0xB6, &param, 1);

	param = 0x3F;
	rm68200_dcs_write(dev, 0xB7, &param, 1);

	param = 0x3F;
	rm68200_dcs_write(dev, 0xB8, &param, 1);

	param = 0x0B;
	rm68200_dcs_write(dev, 0xB9, &param, 1);

	param = 0x0D;
	rm68200_dcs_write(dev, 0xBA, &param, 1);

	param = 0x09;
	rm68200_dcs_write(dev, 0xBB, &param, 1);

	param = 0x3F;
	rm68200_dcs_write(dev, 0xBC, &param, 1);

	param = 0x3F;
	rm68200_dcs_write(dev, 0xBD, &param, 1);

	param = 0x07;
	rm68200_dcs_write(dev, 0xBE, &param, 1);

	param = 0x03;
	rm68200_dcs_write(dev, 0xBF, &param, 1);

	param = 0x3F;
	rm68200_dcs_write(dev, 0xC0, &param, 1);

	param = 0x3F;
	rm68200_dcs_write(dev, 0xC1, &param, 1);

	param = 0x3F;
	rm68200_dcs_write(dev, 0xC2, &param, 1);

	param = 0x3F;
	rm68200_dcs_write(dev, 0xC3, &param, 1);

	param = 0x02;
	rm68200_dcs_write(dev, 0xC4, &param, 1);

	param = 0x06;
	rm68200_dcs_write(dev, 0xC5, &param, 1);

	param = 0x3F;
	rm68200_dcs_write(dev, 0xC6, &param, 1);

	param = 0x3F;
	rm68200_dcs_write(dev, 0xC7, &param, 1);

	param = 0x08;
	rm68200_dcs_write(dev, 0xC8, &param, 1);

	param = 0x0C;
	rm68200_dcs_write(dev, 0xC9, &param, 1);

	param = 0x0A;
	rm68200_dcs_write(dev, 0xCA, &param, 1);

	param = 0x3F;
	rm68200_dcs_write(dev, 0xCB, &param, 1);

	param = 0x3F;
	rm68200_dcs_write(dev, 0xCC, &param, 1);

	param = 0x3F;
	rm68200_dcs_write(dev, 0xCD, &param, 1);

	param = 0x3F;
	rm68200_dcs_write(dev, 0xCE, &param, 1);

	param = 0x3F;
	rm68200_dcs_write(dev, 0xCF, &param, 1);

	param = 0x0E;
	rm68200_dcs_write(dev, 0xD0, &param, 1);

	param = 0x10;
	rm68200_dcs_write(dev, 0xD1, &param, 1);

	param = 0x14;
	rm68200_dcs_write(dev, 0xD2, &param, 1);

	param = 0x12;
	rm68200_dcs_write(dev, 0xD3, &param, 1);

	param = 0x16;
	rm68200_dcs_write(dev, 0xD4, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, 0xD5, &param, 1);

	param = 0x04;
	rm68200_dcs_write(dev, 0xD6, &param, 1);

	param = 0x3F;
	rm68200_dcs_write(dev, 0xD7, &param, 1);

	param = 0x02;
	rm68200_dcs_write(dev, 0xDC, &param, 1);

	param = 0x12;
	rm68200_dcs_write(dev, 0xDE, &param, 1);

	param = 0x0E;
	rm68200_dcs_write(dev, DCS_CMD_PAGE, &param, 1);

	param = 0x75;
	rm68200_dcs_write(dev, 0x01, &param, 1);

	/* Gamma Settings */
	param = DCS_CMD_PAGE_SET_PAGE3;
	rm68200_dcs_write(dev, DCS_CMD_PAGE, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, MCS_GAMMA_VP1, &param, 1);

	param = 0x0C;
	rm68200_dcs_write(dev, MCS_GAMMA_VP4, &param, 1);

	param = 0x12;
	rm68200_dcs_write(dev, MCS_GAMMA_VP8, &param, 1);

	param = 0x0E;
	rm68200_dcs_write(dev, MCS_GAMMA_VP16, &param, 1);

	param = 0x06;
	rm68200_dcs_write(dev, MCS_GAMMA_VP24, &param, 1);

	param = 0x12;
	rm68200_dcs_write(dev, MCS_GAMMA_VP52, &param, 1);

	param = 0x0E;
	rm68200_dcs_write(dev, MCS_GAMMA_VP80, &param, 1);

	param = 0x0B;
	rm68200_dcs_write(dev, MCS_GAMMA_VP108, &param, 1);

	param = 0x15;
	rm68200_dcs_write(dev, MCS_GAMMA_VP147, &param, 1);

	param = 0x0B;
	rm68200_dcs_write(dev, MCS_GAMMA_VP175, &param, 1);

	param = 0x10;
	rm68200_dcs_write(dev, MCS_GAMMA_VP203, &param, 1);

	param = 0x07;
	rm68200_dcs_write(dev, MCS_GAMMA_VP231, &param, 1);

	param = 0x0F;
	rm68200_dcs_write(dev, MCS_GAMMA_VP239, &param, 1);

	param = 0x12;
	rm68200_dcs_write(dev, MCS_GAMMA_VP247, &param, 1);

	param = 0x0C;
	rm68200_dcs_write(dev, MCS_GAMMA_VP251, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, MCS_GAMMA_VP255, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, MCS_GAMMA_VN1, &param, 1);

	param = 0x0C;
	rm68200_dcs_write(dev, MCS_GAMMA_VN4, &param, 1);

	param = 0x12;
	rm68200_dcs_write(dev, MCS_GAMMA_VN8, &param, 1);

	param = 0x0E;
	rm68200_dcs_write(dev, MCS_GAMMA_VN16, &param, 1);

	param = 0x06;
	rm68200_dcs_write(dev, MCS_GAMMA_VN24, &param, 1);

	param = 0x12;
	rm68200_dcs_write(dev, MCS_GAMMA_VN52, &param, 1);

	param = 0x0E;
	rm68200_dcs_write(dev, MCS_GAMMA_VN80, &param, 1);

	param = 0x0B;
	rm68200_dcs_write(dev, MCS_GAMMA_VN108, &param, 1);

	param = 0x15;
	rm68200_dcs_write(dev, MCS_GAMMA_VN147, &param, 1);

	param = 0x0B;
	rm68200_dcs_write(dev, MCS_GAMMA_VN175, &param, 1);

	param = 0x10;
	rm68200_dcs_write(dev, MCS_GAMMA_VN203, &param, 1);

	param = 0x07;
	rm68200_dcs_write(dev, MCS_GAMMA_VN231, &param, 1);

	param = 0x0F;
	rm68200_dcs_write(dev, MCS_GAMMA_VN239, &param, 1);

	param = 0x12;
	rm68200_dcs_write(dev, MCS_GAMMA_VN247, &param, 1);

	param = 0x0C;
	rm68200_dcs_write(dev, MCS_GAMMA_VN251, &param, 1);

	param = 0x00;
	rm68200_dcs_write(dev, MCS_GAMMA_VN255, &param, 1);

	/* Page 0 */
	param = DCS_CMD_PAGE_UCS;
	rm68200_dcs_write(dev, DCS_CMD_PAGE, &param, 1);

	rm68200_dcs_write(dev, MIPI_DCS_EXIT_SLEEP_MODE, NULL, 0);

	k_sleep(K_MSEC(200));

	rm68200_dcs_write(dev, MIPI_DCS_SET_DISPLAY_ON, NULL, 0);

	k_sleep(K_MSEC(100));

	rm68200_dcs_write(dev, MIPI_DCS_WRITE_MEMORY_START, NULL, 0);

	param = 0x00;
	rm68200_dcs_write(dev, MIPI_DCS_SET_TEAR_ON, &param, 1);

	k_sleep(K_MSEC(200));

	if (config->bl_gpio.port != NULL) {
		ret = gpio_pin_configure_dt(&config->bl_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Could not configure bl GPIO (%d)", ret);
			return ret;
		}
	}

	return 0;
}

#define RM68200_PANEL(id)							\
	static const struct rm68200_config rm68200_config_##id = {		\
		.mipi_dsi = DEVICE_DT_GET(DT_INST_BUS(id)),			\
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(id, reset_gpios, {0}),	\
		.bl_gpio = GPIO_DT_SPEC_INST_GET_OR(id, bl_gpios, {0}),		\
		.num_of_lanes = DT_INST_PROP_BY_IDX(id, data_lanes, 0),			\
		.pixel_format = DT_INST_PROP(id, pixel_format),			\
		.panel_width = DT_INST_PROP(id, width),				\
		.panel_height = DT_INST_PROP(id, height),			\
		.channel = DT_INST_REG_ADDR(id),				\
	};									\
	DEVICE_DT_INST_DEFINE(id,						\
			    &rm68200_init,					\
			    NULL,						\
			    NULL,						\
			    &rm68200_config_##id,				\
			    POST_KERNEL,					\
			    CONFIG_APPLICATION_INIT_PRIORITY,			\
			    &rm68200_api);

DT_INST_FOREACH_STATUS_OKAY(RM68200_PANEL)
