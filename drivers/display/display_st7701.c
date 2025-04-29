/*
 * Copyright (c) 2025 Arduino SA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sitronix_st7701

#include <zephyr/kernel.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/mipi_dsi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(st7701, CONFIG_DISPLAY_LOG_LEVEL);

/* Command2 BKx selection command */
#define DSI_CMD2BKX_SEL      0xFF
#define DSI_CMD2BK1_SEL      0x11
#define DSI_CMD2BK0_SEL      0x10
#define DSI_CMD2BKX_SEL_NONE 0x00

/* Command2, BK0 commands */
#define DSI_CMD2_BK0_PVGAMCTRL 0xB0 /* Positive Voltage Gamma Control */
#define DSI_CMD2_BK0_NVGAMCTRL 0xB1 /* Negative Voltage Gamma Control */
#define DSI_CMD2_BK0_LNESET    0xC0 /* Display Line setting */
#define DSI_CMD2_BK0_PORCTRL   0xC1 /* Porch control */
#define DSI_CMD2_BK0_INVSEL    0xC2 /* Inversion selection, Frame Rate Control */

/* Command2, BK1 commands */
#define DSI_CMD2_BK1_VRHS     0xB0 /* Vop amplitude setting */
#define DSI_CMD2_BK1_VCOM     0xB1 /* VCOM amplitude setting */
#define DSI_CMD2_BK1_VGHSS    0xB2 /* VGH Voltage setting */
#define DSI_CMD2_BK1_TESTCMD  0xB3 /* TEST Command Setting */
#define DSI_CMD2_BK1_VGLS     0xB5 /* VGL Voltage setting */
#define DSI_CMD2_BK1_PWCTLR1  0xB7 /* Power Control 1 */
#define DSI_CMD2_BK1_PWCTLR2  0xB8 /* Power Control 2 */
#define DSI_CMD2_BK1_SPD1     0xC1 /* Source pre_drive timing set1 */
#define DSI_CMD2_BK1_SPD2     0xC2 /* Source EQ2 Setting */
#define DSI_CMD2_BK1_MIPISET1 0xD0 /* MIPI Setting 1 */

#define ST7701_CMD_ID1 0xDA
#define ST7701_ID      0xFF

/* Write Control Display: brightness control. */
#define ST7701_WRCTRLD_BCTRL BIT(5)
/* Write Control Display: display dimming. */
#define ST7701_WRCTRLD_DD    BIT(3)
/* Write Control Display: backlight. */
#define ST7701_WRCTRLD_BL    BIT(2)

/* Adaptive Brightness Control: off. */
#define ST7701_WRCABC_OFF 0x00U
/* Adaptive Brightness Control: user interface. */
#define ST7701_WRCABC_UI  0x01U
/* Adaptive Brightness Control: still picture. */
#define ST7701_WRCABC_ST  0x02U
/* Adaptive Brightness Control: moving image. */
#define ST7701_WRCABC_MV  0x03U

struct st7701_config {
	const struct device *mipi_dsi;
	const struct gpio_dt_spec reset;
	const struct gpio_dt_spec backlight;
	uint8_t data_lanes;
	uint16_t width;
	uint16_t height;
	uint8_t channel;
	uint16_t rotation;
	uint32_t hbp;
	uint32_t hsync;
	uint32_t hfp;
	uint32_t vbp;
	uint32_t vsync;
	uint32_t vfp;
	uint8_t gip_e0[4];
	uint8_t gip_e1[12];
	uint8_t gip_e2[14];
	uint8_t gip_e3[5];
	uint8_t gip_e4[3];
	uint8_t gip_e5[17];
	uint8_t gip_e6[5];
	uint8_t gip_e7[3];
	uint8_t gip_e8[17];
	uint8_t gip_eb[8];
	uint8_t gip_ec[3];
	uint8_t gip_ed[17];
	uint8_t pvgamctrl[17];
	uint8_t nvgamctrl[17];
};

struct st7701_data {
	uint16_t xres;
	uint16_t yres;
	uint8_t dsi_pixel_format;
	enum display_pixel_format pixel_format;
	enum display_orientation orientation;
};

static inline int st7701_dcs_write(const struct device *dev, uint8_t cmd, const void *buf,
				   size_t len)
{
	const struct st7701_config *cfg = dev->config;
	int ret;

	ret = mipi_dsi_dcs_write(cfg->mipi_dsi, cfg->channel, cmd, buf, len);
	if (ret < 0) {
		LOG_ERR("DCS 0x%x write failed! (%d)", cmd, ret);
		return ret;
	}

	return 0;
}

static int st7701_short_write_1p(const struct device *dev, uint8_t cmd, uint8_t val)
{
	const struct st7701_config *cfg = dev->config;
	int ret;
	uint8_t buf[] = {cmd, val};

	ret = mipi_dsi_generic_write(cfg->mipi_dsi, cfg->channel, buf, sizeof(val));
	if (ret < 0) {
		LOG_ERR("Short write failed! (%d)", ret);
		return ret;
	}

	return 0;
}

static int st7701_generic_write(const struct device *dev, const void *buf, size_t len)
{
	const struct st7701_config *cfg = dev->config;
	int ret;

	ret = mipi_dsi_generic_write(cfg->mipi_dsi, cfg->channel, buf, len);
	if (ret < 0) {
		LOG_ERR("Generic write failed! (%d)", ret);
		return ret;
	}

	return 0;
}

static int st7701_check_id(const struct device *dev)
{
	const struct st7701_config *cfg = dev->config;
	uint32_t id = 0;
	int ret;

	ret = mipi_dsi_dcs_read(cfg->mipi_dsi, cfg->channel, ST7701_CMD_ID1, &id, sizeof(id));
	if (ret != sizeof(id)) {
		LOG_ERR("Read panel ID failed! (%d)", ret);
		return -EIO;
	}

	if (id != ST7701_ID) {
		LOG_ERR("ID 0x%x (should 0x%x)", id, ST7701_ID);
		return -EINVAL;
	}

	return 0;
}

static int st7701_configure(const struct device *dev)
{
	struct st7701_data *data = dev->data;
	const struct st7701_config *cfg = dev->config;
	uint8_t buf[4];
	int ret;

	const uint8_t ff1[] = {DSI_CMD2BKX_SEL, 0x77, 0x01, 0x00, 0x00, DSI_CMD2BK1_SEL};
	const uint8_t ff2[] = {DSI_CMD2BKX_SEL, 0x77, 0x01, 0x00, 0x00, DSI_CMD2BKX_SEL_NONE};

	const uint8_t control0[] = {DSI_CMD2BKX_SEL, 0x77, 0x01, 0x00, 0x00, DSI_CMD2BK0_SEL};
	const uint8_t control1[] = {0xC0, 0x63, 0x00};
	const uint8_t control2[] = {0xC1, 0x11, 0x02};
	const uint8_t control3[] = {0xC2, 0x01, 0x08};
	const uint8_t control4[] = {0xCC, 0x18};

	st7701_generic_write(dev, control0, sizeof(control0));
	st7701_generic_write(dev, control1, sizeof(control1));
	st7701_generic_write(dev, control2, sizeof(control2));
	st7701_generic_write(dev, control3, sizeof(control3));
	st7701_generic_write(dev, control4, sizeof(control4));

	/* Gamma Cluster Setting */
	st7701_generic_write(dev, cfg->pvgamctrl, sizeof(cfg->pvgamctrl));
	st7701_generic_write(dev, cfg->nvgamctrl, sizeof(cfg->nvgamctrl));

	/* Initial power control registers */
	st7701_generic_write(dev, ff1, sizeof(ff1));

	st7701_short_write_1p(dev, DSI_CMD2_BK1_VRHS, 0x65);
	st7701_short_write_1p(dev, DSI_CMD2_BK1_VCOM, 0x34);
	st7701_short_write_1p(dev, DSI_CMD2_BK1_VGHSS, 0x87);
	st7701_short_write_1p(dev, DSI_CMD2_BK1_TESTCMD, 0x80);

	st7701_short_write_1p(dev, DSI_CMD2_BK1_VGLS, 0x49);
	st7701_short_write_1p(dev, DSI_CMD2_BK1_PWCTLR1, 0x85);

	st7701_short_write_1p(dev, DSI_CMD2_BK1_PWCTLR2, 0x20);
	st7701_short_write_1p(dev, 0xB9, 0x10);
	st7701_short_write_1p(dev, DSI_CMD2_BK1_SPD1, 0x78);
	st7701_short_write_1p(dev, DSI_CMD2_BK1_SPD2, 0x78);
	st7701_short_write_1p(dev, DSI_CMD2_BK1_MIPISET1, 0x88);
	k_msleep(100);

	/* GIP Setting */
	st7701_generic_write(dev, cfg->gip_e0, sizeof(cfg->gip_e0));
	st7701_generic_write(dev, cfg->gip_e1, sizeof(cfg->gip_e1));
	st7701_generic_write(dev, cfg->gip_e2, sizeof(cfg->gip_e2));
	st7701_generic_write(dev, cfg->gip_e3, sizeof(cfg->gip_e3));
	st7701_generic_write(dev, cfg->gip_e4, sizeof(cfg->gip_e4));
	st7701_generic_write(dev, cfg->gip_e5, sizeof(cfg->gip_e5));
	st7701_generic_write(dev, cfg->gip_e6, sizeof(cfg->gip_e6));
	st7701_generic_write(dev, cfg->gip_e7, sizeof(cfg->gip_e7));
	st7701_generic_write(dev, cfg->gip_e8, sizeof(cfg->gip_e8));
	st7701_generic_write(dev, cfg->gip_eb, sizeof(cfg->gip_eb));
	st7701_generic_write(dev, cfg->gip_ec, sizeof(cfg->gip_ec));
	st7701_generic_write(dev, cfg->gip_ed, sizeof(cfg->gip_ed));

	/* Bank1 setting */
	st7701_generic_write(dev, ff2, sizeof(ff2));

	/* Exit sleep mode */
	ret = st7701_dcs_write(dev, MIPI_DCS_EXIT_SLEEP_MODE, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	k_msleep(50);

	/* Set pixel color format */
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

	ret = st7701_dcs_write(dev, MIPI_DCS_SET_PIXEL_FORMAT, buf, 1);
	if (ret < 0) {
		return ret;
	}

	buf[0] = 0x00;
	buf[1] = 0x00;
	sys_put_be16(data->xres, (uint8_t *)&buf[2]);
	ret = st7701_dcs_write(dev, MIPI_DCS_SET_COLUMN_ADDRESS, buf, 4);
	if (ret < 0) {
		return ret;
	}

	buf[0] = 0x00;
	buf[1] = 0x00;
	sys_put_be16(data->yres, (uint8_t *)&buf[2]);
	ret = st7701_dcs_write(dev, MIPI_DCS_SET_PAGE_ADDRESS, buf, 4);
	if (ret < 0) {
		return ret;
	}

	/* Backlight control */
	buf[0] = ST7701_WRCTRLD_BCTRL | ST7701_WRCTRLD_DD | ST7701_WRCTRLD_BL;
	ret = st7701_dcs_write(dev, MIPI_DCS_WRITE_CONTROL_DISPLAY, buf, 1);
	if (ret < 0) {
		return ret;
	}

	/* Adaptive brightness control */
	buf[0] = ST7701_WRCABC_UI;
	ret = st7701_dcs_write(dev, MIPI_DCS_WRITE_POWER_SAVE, buf, 1);
	if (ret < 0) {
		return ret;
	}

	/* Adaptive brightness control minimum brightness */
	buf[0] = 0xFF;
	ret = st7701_dcs_write(dev, MIPI_DCS_SET_CABC_MIN_BRIGHTNESS, buf, 1);
	if (ret < 0) {
		return ret;
	}

	/* Brightness */
	buf[0] = 0xFF;
	ret = st7701_dcs_write(dev, MIPI_DCS_SET_DISPLAY_BRIGHTNESS, buf, 1);
	if (ret < 0) {
		return ret;
	}

	/* Display On */
	ret = st7701_dcs_write(dev, MIPI_DCS_SET_DISPLAY_ON, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int st7701_blanking_on(const struct device *dev)
{
	const struct st7701_config *cfg = dev->config;
	int ret;

	if (cfg->backlight.port != NULL) {
		ret = gpio_pin_set_dt(&cfg->backlight, 0);
		if (ret) {
			LOG_ERR("Disable backlight failed! (%d)", ret);
			return ret;
		}
	}

	return st7701_dcs_write(dev, MIPI_DCS_SET_DISPLAY_OFF, NULL, 0);
}

static int st7701_blanking_off(const struct device *dev)
{
	const struct st7701_config *cfg = dev->config;
	int ret;

	if (cfg->backlight.port != NULL) {
		ret = gpio_pin_set_dt(&cfg->backlight, 1);
		if (ret) {
			LOG_ERR("Enable backlight failed! (%d)", ret);
			return ret;
		}
	}

	return st7701_dcs_write(dev, MIPI_DCS_SET_DISPLAY_ON, NULL, 0);
}

static int st7701_set_brightness(const struct device *dev, uint8_t brightness)
{
	return st7701_dcs_write(dev, MIPI_DCS_SET_DISPLAY_BRIGHTNESS, &brightness, 1);
}

static void st7701_get_capabilities(const struct device *dev,
				    struct display_capabilities *capabilities)
{
	const struct st7701_config *cfg = dev->config;
	struct st7701_data *data = dev->data;

	if (!capabilities) {
		return;
	}

	memset(capabilities, 0, sizeof(struct display_capabilities));
	capabilities->x_resolution = cfg->width;
	capabilities->y_resolution = cfg->height;
	capabilities->supported_pixel_formats = data->pixel_format;
	capabilities->current_pixel_format = data->pixel_format;
	capabilities->current_orientation = data->orientation;
}

static DEVICE_API(display, st7701_api) = {
	.blanking_on = st7701_blanking_on,
	.blanking_off = st7701_blanking_off,
	.set_brightness = st7701_set_brightness,
	.get_capabilities = st7701_get_capabilities,
};

static int st7701_init(const struct device *dev)
{
	const struct st7701_config *cfg = dev->config;
	struct st7701_data *data = dev->data;
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

		k_msleep(10);
		ret = gpio_pin_set_dt(&cfg->reset, 1);
		if (ret < 0) {
			LOG_ERR("Enable display failed! (%d)", ret);
			return ret;
		}

		k_msleep(100);
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

	mdev.timings.hactive = cfg->width;
	mdev.timings.hbp = cfg->hbp;
	mdev.timings.hsync = cfg->hsync;
	mdev.timings.hfp = cfg->hfp;
	mdev.timings.vactive = cfg->height;
	mdev.timings.vbp = cfg->vbp;
	mdev.timings.vsync = cfg->vsync;
	mdev.timings.vfp = cfg->vfp;

	ret = mipi_dsi_attach(cfg->mipi_dsi, cfg->channel, &mdev);
	if (ret < 0) {
		LOG_ERR("MIPI-DSI attach failed! (%d)", ret);
		return ret;
	}

	ret = st7701_check_id(dev);
	if (ret) {
		LOG_ERR("Panel ID check failed! (%d)", ret);
		return ret;
	}

	ret = st7701_configure(dev);
	if (ret) {
		LOG_ERR("DSI init sequence failed! (%d)", ret);
		return ret;
	}

	ret = st7701_blanking_off(dev);
	if (ret) {
		LOG_ERR("Display blanking off failed! (%d)", ret);
		return ret;
	}

	return 0;
}

#define ST7701_DEVICE(inst)                                                                        \
	static const struct st7701_config st7701_config_##inst = {                                 \
		.mipi_dsi = DEVICE_DT_GET(DT_INST_BUS(inst)),                                      \
		.reset = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),                         \
		.backlight = GPIO_DT_SPEC_INST_GET_OR(inst, bl_gpios, {0}),                        \
		.data_lanes = DT_INST_PROP_BY_IDX(inst, data_lanes, 0),                            \
		.width = DT_INST_PROP(inst, width),                                                \
		.height = DT_INST_PROP(inst, height),                                              \
		.channel = DT_INST_REG_ADDR(inst),                                                 \
		.rotation = DT_INST_PROP(inst, rotation),                                          \
		.hbp = DT_PROP(DT_INST_CHILD(inst, display_timings), hback_porch),                 \
		.hsync = DT_PROP(DT_INST_CHILD(inst, display_timings), hsync_len),                 \
		.hfp = DT_PROP(DT_INST_CHILD(inst, display_timings), hfront_porch),                \
		.vbp = DT_PROP(DT_INST_CHILD(inst, display_timings), vback_porch),                 \
		.vsync = DT_PROP(DT_INST_CHILD(inst, display_timings), vsync_len),                 \
		.vfp = DT_PROP(DT_INST_CHILD(inst, display_timings), vfront_porch),                \
		.gip_e0 = DT_INST_PROP_OR(inst, gip_e0, {}),                                       \
		.gip_e1 = DT_INST_PROP_OR(inst, gip_e1, {}),                                       \
		.gip_e2 = DT_INST_PROP_OR(inst, gip_e2, {}),                                       \
		.gip_e3 = DT_INST_PROP_OR(inst, gip_e3, {}),                                       \
		.gip_e4 = DT_INST_PROP_OR(inst, gip_e4, {}),                                       \
		.gip_e5 = DT_INST_PROP_OR(inst, gip_e5, {}),                                       \
		.gip_e6 = DT_INST_PROP_OR(inst, gip_e6, {}),                                       \
		.gip_e7 = DT_INST_PROP_OR(inst, gip_e7, {}),                                       \
		.gip_e8 = DT_INST_PROP_OR(inst, gip_e8, {}),                                       \
		.gip_eb = DT_INST_PROP_OR(inst, gip_eb, {}),                                       \
		.gip_ec = DT_INST_PROP_OR(inst, gip_ec, {}),                                       \
		.gip_ed = DT_INST_PROP_OR(inst, gip_ed, {}),                                       \
		.pvgamctrl = DT_INST_PROP_OR(inst, pvgamctrl, {}),                                 \
		.nvgamctrl = DT_INST_PROP_OR(inst, nvgamctrl, {}),                                 \
	};                                                                                         \
	static struct st7701_data st7701_data_##inst = {                                           \
		.dsi_pixel_format = DT_INST_PROP(inst, pixel_format),                              \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, &st7701_init, NULL, &st7701_data_##inst,                       \
			      &st7701_config_##inst, POST_KERNEL,                                  \
			      CONFIG_DISPLAY_INIT_PRIORITY, &st7701_api);

DT_INST_FOREACH_STATUS_OKAY(ST7701_DEVICE)
