/*
 * Copyright (c) 2023 bytes at work AG
 * Copyright (c) 2020 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT orisetech_otm8009a

#include <zephyr/kernel.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/mipi_dsi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(otm8009a, CONFIG_DISPLAY_LOG_LEVEL);

#include "display_otm8009a.h"

struct otm8009a_config {
	const struct device *mipi_dsi;
	const struct gpio_dt_spec reset;
	const struct gpio_dt_spec backlight;
	uint8_t data_lanes;
	uint16_t width;
	uint16_t height;
	uint8_t channel;
	uint16_t rotation;
};

struct otm8009a_data {
	uint16_t xres;
	uint16_t yres;
	uint8_t dsi_pixel_format;
	enum display_pixel_format pixel_format;
	enum display_orientation orientation;
};

static inline int otm8009a_dcs_write(const struct device *dev, uint8_t cmd, const void *buf,
				     size_t len)
{
	const struct otm8009a_config *cfg = dev->config;
	int ret;

	ret = mipi_dsi_dcs_write(cfg->mipi_dsi, cfg->channel, cmd, buf, len);
	if (ret < 0) {
		LOG_ERR("DCS 0x%x write failed! (%d)", cmd, ret);
		return ret;
	}

	return 0;
}

static int otm8009a_mcs_write(const struct device *dev, uint16_t cmd, const void *buf, size_t len)
{
	const struct otm8009a_config *cfg = dev->config;
	uint8_t scmd;
	int ret;

	scmd = cmd & 0xFF;
	ret = mipi_dsi_dcs_write(cfg->mipi_dsi, cfg->channel, OTM8009A_MCS_ADRSFT, &scmd, 1);
	if (ret < 0) {
		return ret;
	}

	ret = mipi_dsi_dcs_write(cfg->mipi_dsi, cfg->channel, cmd >> 8, buf, len);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int otm8009a_check_id(const struct device *dev)
{
	const struct otm8009a_config *cfg = dev->config;
	uint32_t id = 0;
	int ret;

	ret = mipi_dsi_dcs_read(cfg->mipi_dsi, cfg->channel, OTM8009A_CMD_ID1, &id, sizeof(id));
	if (ret != sizeof(id)) {
		LOG_ERR("Read panel ID failed! (%d)", ret);
		return -EIO;
	}

	if (id != OTM8009A_ID1) {
		LOG_ERR("ID 0x%x (should 0x%x)", id, OTM8009A_ID1);
		return -EINVAL;
	}

	return 0;
}

static int otm8009a_configure(const struct device *dev)
{
	struct otm8009a_data *data = dev->data;
	uint8_t buf[4];
	int ret;

	static const uint8_t pwr_ctrl2[] = {0x96, 0x34, 0x01, 0x33, 0x33, 0x34, 0x33};
	static const uint8_t sd_ctrl[] = {0x0D, 0x1B, 0x02, 0x01, 0x3C, 0x08};
	static const uint8_t goavst[] = {
		0x85, 0x01, 0x00, 0x84, 0x01, 0x00, 0x81, 0x01, 0x28, 0x82, 0x01, 0x28
	};
	static const uint8_t goaclka1[] = {0x18, 0x04, 0x03, 0x39, 0x00, 0x00, 0x00};
	static const uint8_t goaclka2[] = {0x18, 0x03, 0x03, 0x3A, 0x00, 0x00, 0x00};
	static const uint8_t goaclka3[] = {0x18, 0x02, 0x03, 0x3B, 0x00, 0x00, 0x00};
	static const uint8_t goaclka4[] = {0x18, 0x01, 0x03, 0x3C, 0x00, 0x00, 0x00};
	static const uint8_t goaeclk[] = {0x01, 0x01, 0x20, 0x20, 0x00, 0x00};
	static const uint8_t panctrlset1[] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};
	static const uint8_t panctrlset2[] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00
	};
	static const uint8_t panctrlset3[] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00
	};
	static const uint8_t panctrlset4[] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};
	static const uint8_t panctrlset5[] = {
		0x00, 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00
	};
	static const uint8_t panctrlset6[] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x00, 0x00,
		0x00
	};
	static const uint8_t panctrlset7[] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};
	static const uint8_t panctrlset8[] = {
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
	};
	static const uint8_t panu2d1[] = {
		0x00, 0x26, 0x09, 0x0B, 0x01, 0x25, 0x00, 0x00, 0x00, 0x00
	};
	static const uint8_t panu2d2[] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x26, 0x0A, 0x0C,
		0x02
	};
	static const uint8_t panu2d3[] = {
		0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00
	};
	static const uint8_t pand2u1[] = {
		0x00, 0x25, 0x0C, 0x0A, 0x02, 0x26, 0x00, 0x00, 0x00, 0x00
	};
	static const uint8_t pand2u2[] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x25, 0x0B, 0x09,
		0x01
	};
	static const uint8_t pand2u3[] = {
		0x26, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00
	};
	static const uint8_t pgamma[] = {
		0x00, 0x09, 0x0F, 0x0E, 0x07, 0x10, 0x0B, 0x0A, 0x04, 0x07, 0x0B, 0x08, 0x0F, 0x10,
		0x0A, 0x01
	};
	static const uint8_t ngamma[] =  {
		0x00, 0x09, 0x0F, 0x0E, 0x07, 0x10, 0x0B, 0x0A, 0x04, 0x07, 0x0B, 0x08, 0x0F, 0x10,
		0x0A, 0x01
	};

	/* enter command 2 mode to access manufacturer registers (ref. 5.3) */
	buf[0] = 0x80;
	buf[1] = 0x09;
	buf[2] = 0x01;
	ret = otm8009a_mcs_write(dev, OTM8009A_MCS_CMD2_ENA1, buf, 3);
	if (ret < 0) {
		return ret;
	}

	/* enter Orise command 2 mode */
	buf[0] = 0x80;
	buf[1] = 0x09;
	ret = otm8009a_mcs_write(dev, OTM8009A_MCS_CMD2_ENA2, buf, 2);
	if (ret < 0) {
		return ret;
	}

	/* source driver precharge control */
	buf[0] = 0x30;
	buf[1] = 0x8A;
	ret = otm8009a_mcs_write(dev, OTM8009A_MCS_SD_PCH_CTRL, buf, 2);
	if (ret < 0) {
		return ret;
	}

	/* not documented */
	buf[0] = 0x40;
	ret = otm8009a_mcs_write(dev, OTM8009A_MCS_NO_DOC1, buf, 1);
	if (ret < 0) {
		return ret;
	}

	/* power control settings 4 for DC voltage settings */
	/* enable GVDD test mode */
	buf[0] = 0x04;
	buf[1] = 0xA9;
	ret = otm8009a_mcs_write(dev, OTM8009A_MCS_PWR_CTRL4, buf, 2);
	if (ret < 0) {
		return ret;
	}

	/* power control settings 2 for normal mode */
	/* set pump 4 vgh voltage from 15.0v down to 13.0v */
	/* set pump 5 vgh voltage from -12.0v downto -9.0v */
	/* set pump 4&5 x6 (ONLY VALID when PUMP4_EN_ASDM_HV = "0") */
	/* change pump4 clock ratio from 1 line to 1/2 line */
	ret = otm8009a_mcs_write(dev, OTM8009A_MCS_PWR_CTRL2, pwr_ctrl2, sizeof(pwr_ctrl2));
	if (ret < 0) {
		return ret;
	}

	/* panel driving mode */
	/* set column inversion */
	buf[0] = 0x50;
	ret = otm8009a_mcs_write(dev, OTM8009A_MCS_P_DRV_M, buf, 1);
	if (ret < 0) {
		return ret;
	}

	/* VCOM voltage setting */
	/* VCOM Voltage settings from -1.0000v downto -1.2625v */
	buf[0] = 0x4E;
	ret = otm8009a_mcs_write(dev, OTM8009A_MCS_VCOMDC, buf, 1);
	if (ret < 0) {
		return ret;
	}

	/* oscillator adjustment for idle/normal mode */
	/* set 65Hz */
	buf[0] = 0x66;
	ret = otm8009a_mcs_write(dev, OTM8009A_MCS_OSC_ADJ, buf, 1);
	if (ret < 0) {
		return ret;
	}

	/* RGB video mode setting */
	buf[0] = 0x08;
	ret = otm8009a_mcs_write(dev, OTM8009A_MCS_RGB_VID_SET, buf, 1);
	if (ret < 0) {
		return ret;
	}

	/* GVDD/NGVDD */
	buf[0] = 0x79;
	buf[1] = 0x79;
	ret = otm8009a_mcs_write(dev, OTM8009A_MCS_GVDDSET, buf, 2);
	if (ret < 0) {
		return ret;
	}

	/* source driver timing setting */
	ret = otm8009a_mcs_write(dev, OTM8009A_MCS_SD_CTRL, sd_ctrl, sizeof(sd_ctrl));
	if (ret < 0) {
		return ret;
	}

	/* panel type setting */
	buf[0] = 0x00;
	buf[1] = 0x01;
	ret = otm8009a_mcs_write(dev, OTM8009A_MCS_PANSET, buf, 2);
	if (ret < 0) {
		return ret;
	}

	/* GOA VST setting */
	ret = otm8009a_mcs_write(dev, OTM8009A_MCS_GOAVST, goavst, sizeof(goavst));
	if (ret < 0) {
		return ret;
	}

	/* GOA CLKA1 setting */
	ret = otm8009a_mcs_write(dev, OTM8009A_MCS_GOACLKA1, goaclka1, sizeof(goaclka1));
	if (ret < 0) {
		return ret;
	}

	/* GOA CLKA2 setting */
	ret = otm8009a_mcs_write(dev, OTM8009A_MCS_GOACLKA2, goaclka2, sizeof(goaclka2));
	if (ret < 0) {
		return ret;
	}

	/* GOA CLKA3 setting */
	ret = otm8009a_mcs_write(dev, OTM8009A_MCS_GOACLKA3, goaclka3, sizeof(goaclka3));
	if (ret < 0) {
		return ret;
	}

	/* GOA CLKA4 setting */
	ret = otm8009a_mcs_write(dev, OTM8009A_MCS_GOACLKA4, goaclka4, sizeof(goaclka4));
	if (ret < 0) {
		return ret;
	}

	/* GOA ECLK */
	ret = otm8009a_mcs_write(dev, OTM8009A_MCS_GOAECLK, goaeclk, sizeof(goaeclk));
	if (ret < 0) {
		return ret;
	}

	/** GOA Other Options 1 */
	buf[0] = 0x01;
	ret = otm8009a_mcs_write(dev, OTM8009A_MCS_GOAPT1, buf, 1);
	if (ret < 0) {
		return ret;
	}

	/* GOA Signal Toggle Option Setting */
	buf[0] = 0x02;
	buf[1] = 0x00;
	buf[2] = 0x00;
	ret = otm8009a_mcs_write(dev, OTM8009A_MCS_GOATGOPT, buf, 3);
	if (ret < 0) {
		return ret;
	}

	/* not documented */
	buf[0] = 0x00;
	ret = otm8009a_mcs_write(dev, OTM8009A_MCS_NO_DOC2, buf, 3);
	if (ret < 0) {
		return ret;
	}

	/* Panel Control Setting 1 */
	ret = otm8009a_mcs_write(dev, OTM8009A_MCS_PANCTRLSET1, panctrlset1, sizeof(panctrlset1));
	if (ret < 0) {
		return ret;
	}

	ret = otm8009a_mcs_write(dev, OTM8009A_MCS_PANCTRLSET2, panctrlset2, sizeof(panctrlset2));
	if (ret < 0) {
		return ret;
	}

	ret = otm8009a_mcs_write(dev, OTM8009A_MCS_PANCTRLSET3, panctrlset3, sizeof(panctrlset3));
	if (ret < 0) {
		return ret;
	}

	ret = otm8009a_mcs_write(dev, OTM8009A_MCS_PANCTRLSET4, panctrlset4, sizeof(panctrlset4));
	if (ret < 0) {
		return ret;
	}

	ret = otm8009a_mcs_write(dev, OTM8009A_MCS_PANCTRLSET5, panctrlset5, sizeof(panctrlset5));
	if (ret < 0) {
		return ret;
	}

	ret = otm8009a_mcs_write(dev, OTM8009A_MCS_PANCTRLSET6, panctrlset6, sizeof(panctrlset6));
	if (ret < 0) {
		return ret;
	}

	ret = otm8009a_mcs_write(dev, OTM8009A_MCS_PANCTRLSET7, panctrlset7, sizeof(panctrlset7));
	if (ret < 0) {
		return ret;
	}

	ret = otm8009a_mcs_write(dev, OTM8009A_MCS_PANCTRLSET8, panctrlset8, sizeof(panctrlset8));
	if (ret < 0) {
		return ret;
	}

	ret = otm8009a_mcs_write(dev, OTM8009A_MCS_PANU2D1, panu2d1, sizeof(panu2d1));
	if (ret < 0) {
		return ret;
	}

	ret = otm8009a_mcs_write(dev, OTM8009A_MCS_PANU2D2, panu2d2, sizeof(panu2d2));
	if (ret < 0) {
		return ret;
	}

	ret = otm8009a_mcs_write(dev, OTM8009A_MCS_PANU2D3, panu2d3, sizeof(panu2d3));
	if (ret < 0) {
		return ret;
	}

	ret = otm8009a_mcs_write(dev, OTM8009A_MCS_PAND2U1, pand2u1, sizeof(pand2u1));
	if (ret < 0) {
		return ret;
	}

	ret = otm8009a_mcs_write(dev, OTM8009A_MCS_PAND2U2, pand2u2, sizeof(pand2u2));
	if (ret < 0) {
		return ret;
	}

	ret = otm8009a_mcs_write(dev, OTM8009A_MCS_PAND2U3, pand2u3, sizeof(pand2u3));
	if (ret < 0) {
		return ret;
	}

	/* power control setting 1 */
	/* Pump 1 min and max DM */
	buf[0] = 0x08;
	buf[1] = 0x66;
	buf[2] = 0x83;
	buf[3] = 0x00;
	ret = otm8009a_mcs_write(dev, OTM8009A_MCS_PWR_CTRL1, buf, 4);
	if (ret < 0) {
		return ret;
	}

	/* not documented */
	buf[0] = 0x06;
	ret = otm8009a_mcs_write(dev, OTM8009A_MCS_NO_DOC3, buf, 1);
	if (ret < 0) {
		return ret;
	}

	/* PWM parameter 3 */
	/* Freq: 19.5 KHz */
	buf[0] = 0x06;
	ret = otm8009a_mcs_write(dev, OTM8009A_MCS_PWM_PARA3, buf, 1);
	if (ret < 0) {
		return ret;
	}

	/* gamma correction 2.2+ */
	ret = otm8009a_mcs_write(dev, OTM8009A_MCS_GMCT2_2P, pgamma, sizeof(pgamma));
	if (ret < 0) {
		return ret;
	}

	/* gamma correction 2.2- */
	ret = otm8009a_mcs_write(dev, OTM8009A_MCS_GMCT2_2N, ngamma, sizeof(ngamma));
	if (ret < 0) {
		return ret;
	}

	/* exit command 2 mode */
	buf[0] = 0xFF;
	buf[1] = 0xFF;
	buf[2] = 0xFF;
	ret = otm8009a_mcs_write(dev, OTM8009A_MCS_CMD2_ENA1, buf, 3);
	if (ret < 0) {
		return ret;
	}

	/* exit sleep mode */
	ret = otm8009a_dcs_write(dev, MIPI_DCS_EXIT_SLEEP_MODE, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	k_msleep(OTM8009A_EXIT_SLEEP_MODE_WAIT_TIME);

	/* set pixel color format */
	switch (data->dsi_pixel_format) {
	case MIPI_DSI_PIXFMT_RGB565:
		buf[0] = MIPI_DCS_PIXEL_FORMAT_16BIT;
		break;
	case MIPI_DSI_PIXFMT_RGB888:
		buf[0] = MIPI_DCS_PIXEL_FORMAT_24BIT;
		break;
	default:
		LOG_ERR("Unsupported pixel format 0x%x!", data->dsi_pixel_format);
		return -ENOTSUP;
	}

	ret = otm8009a_dcs_write(dev, MIPI_DCS_SET_PIXEL_FORMAT, buf, 1);
	if (ret < 0) {
		return ret;
	}

	/* configure address mode */
	if (data->orientation == DISPLAY_ORIENTATION_NORMAL) {
		buf[0] = 0x00;
	} else if (data->orientation == DISPLAY_ORIENTATION_ROTATED_90) {
		buf[0] = MIPI_DCS_ADDRESS_MODE_MIRROR_X | MIPI_DCS_ADDRESS_MODE_SWAP_XY;
	} else if (data->orientation == DISPLAY_ORIENTATION_ROTATED_180) {
		buf[0] = MIPI_DCS_ADDRESS_MODE_MIRROR_X | MIPI_DCS_ADDRESS_MODE_MIRROR_Y;
	} else if (data->orientation == DISPLAY_ORIENTATION_ROTATED_270) {
		buf[0] = MIPI_DCS_ADDRESS_MODE_MIRROR_Y | MIPI_DCS_ADDRESS_MODE_SWAP_XY;
	}

	ret = otm8009a_dcs_write(dev, MIPI_DCS_SET_ADDRESS_MODE, buf, 1);
	if (ret < 0) {
		return ret;
	}

	buf[0] = 0x00;
	buf[1] = 0x00;
	sys_put_be16(data->xres, (uint8_t *)&buf[2]);
	ret = otm8009a_dcs_write(dev, MIPI_DCS_SET_COLUMN_ADDRESS, buf, 4);
	if (ret < 0) {
		return ret;
	}

	buf[0] = 0x00;
	buf[1] = 0x00;
	sys_put_be16(data->yres, (uint8_t *)&buf[2]);
	ret = otm8009a_dcs_write(dev, MIPI_DCS_SET_PAGE_ADDRESS, buf, 4);
	if (ret < 0) {
		return ret;
	}

	/* backlight control */
	buf[0] = OTM8009A_WRCTRLD_BCTRL | OTM8009A_WRCTRLD_DD | OTM8009A_WRCTRLD_BL;
	ret = otm8009a_dcs_write(dev, MIPI_DCS_WRITE_CONTROL_DISPLAY, buf, 1);
	if (ret < 0) {
		return ret;
	}

	/* adaptive brightness control */
	buf[0] = OTM8009A_WRCABC_UI;
	ret = otm8009a_dcs_write(dev, MIPI_DCS_WRITE_POWER_SAVE, buf, 1);
	if (ret < 0) {
		return ret;
	}

	/* adaptive brightness control minimum brightness */
	buf[0] = 0xFF;
	ret = otm8009a_dcs_write(dev, MIPI_DCS_SET_CABC_MIN_BRIGHTNESS, buf, 1);
	if (ret < 0) {
		return ret;
	}

	/* brightness */
	buf[0] = 0xFF;
	ret = otm8009a_dcs_write(dev, MIPI_DCS_SET_DISPLAY_BRIGHTNESS, buf, 1);
	if (ret < 0) {
		return ret;
	}

	/* Display On */
	ret = otm8009a_dcs_write(dev, MIPI_DCS_SET_DISPLAY_ON, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	/* trigger display write (from data coming by DSI bus) */
	ret = otm8009a_dcs_write(dev, MIPI_DCS_WRITE_MEMORY_START, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int otm8009a_blanking_on(const struct device *dev)
{
	const struct otm8009a_config *cfg = dev->config;
	int ret;

	if (cfg->backlight.port != NULL) {
		ret = gpio_pin_set_dt(&cfg->backlight, 0);
		if (ret) {
			LOG_ERR("Disable backlight failed! (%d)", ret);
			return ret;
		}
	}

	return otm8009a_dcs_write(dev, MIPI_DCS_SET_DISPLAY_OFF, NULL, 0);
}

static int otm8009a_blanking_off(const struct device *dev)
{
	const struct otm8009a_config *cfg = dev->config;
	int ret;

	if (cfg->backlight.port != NULL) {
		ret = gpio_pin_set_dt(&cfg->backlight, 1);
		if (ret) {
			LOG_ERR("Enable backlight failed! (%d)", ret);
			return ret;
		}
	}

	return otm8009a_dcs_write(dev, MIPI_DCS_SET_DISPLAY_ON, NULL, 0);
}

static int otm8009a_write(const struct device *dev, uint16_t x, uint16_t y,
			  const struct display_buffer_descriptor *desc, const void *buf)
{
	return -ENOTSUP;
}

static int otm8009a_read(const struct device *dev, uint16_t x, uint16_t y,
			 const struct display_buffer_descriptor *desc, void *buf)
{
	return -ENOTSUP;
}

static void *otm8009a_get_framebuffer(const struct device *dev)
{
	return NULL;
}

static int otm8009a_set_brightness(const struct device *dev, uint8_t brightness)
{
	return otm8009a_dcs_write(dev, MIPI_DCS_SET_DISPLAY_BRIGHTNESS, &brightness, 1);
}

static int otm8009a_set_contrast(const struct device *dev, uint8_t contrast)
{
	return -ENOTSUP;
}

static void otm8009a_get_capabilities(const struct device *dev,
				      struct display_capabilities *capabilities)
{
	const struct otm8009a_config *cfg = dev->config;
	struct otm8009a_data *data = dev->data;

	memset(capabilities, 0, sizeof(struct display_capabilities));
	capabilities->x_resolution = cfg->width;
	capabilities->y_resolution = cfg->height;
	capabilities->supported_pixel_formats = data->pixel_format;
	capabilities->current_pixel_format = data->pixel_format;
	capabilities->current_orientation = data->orientation;
}

static int otm8009a_set_pixel_format(const struct device *dev,
				     enum display_pixel_format pixel_format)
{
	return -ENOTSUP;
}

static int otm8009a_set_orientation(const struct device *dev,
				    enum display_orientation orientation)
{
	return -ENOTSUP;
}

static int otm8009a_set_scroll_area(const struct device *dev,
			    uint16_t tfa,
				uint16_t bfa)
{
	return -ENOTSUP;
}

static int otm8009a_scroll(const struct device *dev,
			    uint16_t val)
{
	return -ENOTSUP;
}

static const struct display_driver_api otm8009a_api = {
	.blanking_on = otm8009a_blanking_on,
	.blanking_off = otm8009a_blanking_off,
	.write = otm8009a_write,
	.read = otm8009a_read,
	.get_framebuffer = otm8009a_get_framebuffer,
	.set_brightness = otm8009a_set_brightness,
	.set_contrast = otm8009a_set_contrast,
	.get_capabilities = otm8009a_get_capabilities,
	.set_pixel_format = otm8009a_set_pixel_format,
	.set_orientation = otm8009a_set_orientation,
	.set_scroll_area = otm8009a_set_scroll_area,
	.set_scroll = otm8009a_scroll,
};

static int otm8009a_init(const struct device *dev)
{
	const struct otm8009a_config *cfg = dev->config;
	struct otm8009a_data *data = dev->data;
	struct mipi_dsi_device mdev;
	int ret;

	if (cfg->reset.port) {
		if (!gpio_is_ready_dt(&cfg->reset)) {
			LOG_ERR("Reset GPIO device is not ready!");
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&cfg->reset, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Reset display failed! (%d)", ret);
			return ret;
		}
		k_msleep(OTM8009A_RESET_TIME);
		ret = gpio_pin_set_dt(&cfg->reset, 1);
		if (ret < 0) {
			LOG_ERR("Enable display failed! (%d)", ret);
			return ret;
		}
		k_msleep(OTM8009A_WAKE_TIME);
	}

	/* store x/y resolution & rotation */
	if (cfg->rotation == 0) {
		data->xres = cfg->width;
		data->yres = cfg->height;
		data->orientation = DISPLAY_ORIENTATION_NORMAL;
	} else if (cfg->rotation == 90) {
		data->xres = cfg->height;
		data->yres = cfg->width;
		data->orientation = DISPLAY_ORIENTATION_ROTATED_90;
	} else if (cfg->rotation == 180) {
		data->xres = cfg->width;
		data->yres = cfg->height;
		data->orientation = DISPLAY_ORIENTATION_ROTATED_180;
	} else if (cfg->rotation == 270) {
		data->xres = cfg->height;
		data->yres = cfg->width;
		data->orientation = DISPLAY_ORIENTATION_ROTATED_270;
	}

	/* attach to MIPI-DSI host */
	mdev.data_lanes = cfg->data_lanes;
	mdev.pixfmt = data->dsi_pixel_format;
	mdev.mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST | MIPI_DSI_MODE_LPM;

	mdev.timings.hactive = data->xres;
	mdev.timings.hbp = OTM8009A_HBP;
	mdev.timings.hfp = OTM8009A_HFP;
	mdev.timings.hsync = OTM8009A_HSYNC;
	mdev.timings.vactive = data->yres;
	mdev.timings.vbp = OTM8009A_VBP;
	mdev.timings.vfp = OTM8009A_VFP;
	mdev.timings.vsync = OTM8009A_VSYNC;

	ret = mipi_dsi_attach(cfg->mipi_dsi, cfg->channel, &mdev);
	if (ret < 0) {
		LOG_ERR("MIPI-DSI attach failed! (%d)", ret);
		return ret;
	}

	ret = otm8009a_check_id(dev);
	if (ret) {
		LOG_ERR("Panel ID check failed! (%d)", ret);
		return ret;
	}

	ret = otm8009a_configure(dev);
	if (ret) {
		LOG_ERR("DSI init sequence failed! (%d)", ret);
		return ret;
	}

	ret = otm8009a_blanking_off(dev);
	if (ret) {
		LOG_ERR("Display blanking off failed! (%d)", ret);
		return ret;
	}

	return 0;
}

#define OTM8009A_DEVICE(inst)									\
	static const struct otm8009a_config otm8009a_config_##inst = {				\
		.mipi_dsi = DEVICE_DT_GET(DT_INST_BUS(inst)),					\
		.reset = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),			\
		.backlight = GPIO_DT_SPEC_INST_GET_OR(inst, bl_gpios, {0}),			\
		.data_lanes = DT_INST_PROP_BY_IDX(inst, data_lanes, 0),				\
		.width = DT_INST_PROP(inst, width),						\
		.height = DT_INST_PROP(inst, height),						\
		.channel = DT_INST_REG_ADDR(inst),						\
		.rotation = DT_INST_PROP(inst, rotation),					\
	};											\
	static struct otm8009a_data otm8009a_data_##inst = {					\
		.dsi_pixel_format = DT_INST_PROP(inst, pixel_format),				\
	};											\
	DEVICE_DT_INST_DEFINE(inst, &otm8009a_init, NULL, &otm8009a_data_##inst,		\
			      &otm8009a_config_##inst, POST_KERNEL,				\
			      CONFIG_DISPLAY_OTM8009A_INIT_PRIORITY, &otm8009a_api);		\

DT_INST_FOREACH_STATUS_OKAY(OTM8009A_DEVICE)
