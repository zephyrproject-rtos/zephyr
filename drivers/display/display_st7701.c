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
#include <zephyr/pm/device.h>
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
	bool inversion_on;
	uint8_t bk3_ef[4];
	uint8_t bk3_ef_len;
	uint8_t lneset[3];
	uint8_t lneset_len;
	uint8_t porctrl[3];
	uint8_t porctrl_len;
	uint8_t cc_control[2];
	uint8_t cc_control_len;
	uint8_t invsel[3];
	uint8_t invsel_len;
	uint8_t vrhs;
	uint8_t vcom;
	uint8_t vghss;
	uint8_t vgls;
	uint8_t pwctlr1;
	uint8_t pwctlr2;
	uint8_t spd1;
	uint8_t spd2;
	uint8_t mipiset1;
	uint8_t b9;
	bool has_b9;
	uint8_t gip_e0[4];
	uint8_t gip_e1[12];
	uint8_t gip_e2[14];
	uint8_t gip_e3[5];
	uint8_t gip_e4[3];
	uint8_t gip_e5[17];
	uint8_t gip_e6[5];
	uint8_t gip_e7[3];
	uint8_t gip_e8[17];
	uint8_t gip_e9[4];
	uint8_t gip_eb[8];
	uint8_t gip_ec[3];
	uint8_t gip_ed[17];
	uint8_t gip_ef[8];
	uint8_t pvgamctrl[17];
	uint8_t nvgamctrl[17];
	uint8_t gip_e0_len;
	uint8_t gip_e1_len;
	uint8_t gip_e2_len;
	uint8_t gip_e3_len;
	uint8_t gip_e4_len;
	uint8_t gip_e5_len;
	uint8_t gip_e6_len;
	uint8_t gip_e7_len;
	uint8_t gip_e8_len;
	uint8_t gip_e9_len;
	uint8_t gip_eb_len;
	uint8_t gip_ec_len;
	uint8_t gip_ed_len;
	uint8_t gip_ef_len;
	uint8_t pvgamctrl_len;
	uint8_t nvgamctrl_len;
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

	ret = mipi_dsi_generic_write(cfg->mipi_dsi, cfg->channel, buf, sizeof(buf));
	if (ret < 0) {
		LOG_ERR("Short write failed! (%d)", ret);
		return ret;
	}

	return 0;
}

/*
 * Write a panel setting provided through devicetree, using the number of
 * bytes actually set in the property. Settings without a devicetree value
 * are skipped so the controller's power-on defaults are kept.
 */
static void st7701_write_dt_setting(const struct device *dev, const uint8_t *buf, uint8_t len)
{
	const struct st7701_config *cfg = dev->config;
	int ret;

	if (len == 0U) {
		return;
	}

	ret = mipi_dsi_generic_write(cfg->mipi_dsi, cfg->channel, buf, len);
	if (ret < 0) {
		LOG_ERR("Setting 0x%x write failed! (%d)", buf[0], ret);
	}
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
	if (ret == -ENOTSUP || ret == -ENOSYS) {
		LOG_WRN("MIPI-DSI host does not support DCS read, skipping panel ID check");
		return 0;
	}
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
	/*
	 * Number of lines NL = (Line[6:0] + 1) * 8 + (LDE_EN ? Line_delta[1:0] * 2 : 0).
	 * When the height is not a multiple of 8, enable the extra-line delta to
	 * encode the remainder. Panel heights are always even, so the delta (in
	 * units of 2 lines) can represent any remainder.
	 */
	const uint8_t lde_line = (uint8_t)(cfg->height / 8 - 1);
	const uint8_t lde_delta = (cfg->height % 8) / 2;
	const uint8_t control1[] = {DSI_CMD2_BK0_LNESET,
				    (uint8_t)(lde_delta ? (BIT(7) | lde_line) : lde_line),
				    lde_delta};
	const uint8_t control2[] = {DSI_CMD2_BK0_PORCTRL, 0x11, 0x02};
	const uint8_t control3[] = {DSI_CMD2_BK0_INVSEL, 0x01, 0x08};
	const uint8_t control4[] = {0xCC, 0x18};

	/* Optional Command2 BK3 setting */
	if (cfg->bk3_ef_len > 0U) {
		const uint8_t ff3[] = {DSI_CMD2BKX_SEL, 0x77, 0x01, 0x00, 0x00, 0x13};

		st7701_generic_write(dev, ff3, sizeof(ff3));
		st7701_write_dt_setting(dev, cfg->bk3_ef, cfg->bk3_ef_len);
	}

	st7701_generic_write(dev, control0, sizeof(control0));
	if (cfg->lneset_len > 0U) {
		st7701_write_dt_setting(dev, cfg->lneset, cfg->lneset_len);
	} else {
		st7701_generic_write(dev, control1, sizeof(control1));
	}
	if (cfg->porctrl_len > 0U) {
		st7701_write_dt_setting(dev, cfg->porctrl, cfg->porctrl_len);
	} else {
		st7701_generic_write(dev, control2, sizeof(control2));
	}
	if (cfg->invsel_len > 0U) {
		st7701_write_dt_setting(dev, cfg->invsel, cfg->invsel_len);
	} else {
		st7701_generic_write(dev, control3, sizeof(control3));
	}
	if (cfg->cc_control_len > 0U) {
		st7701_write_dt_setting(dev, cfg->cc_control, cfg->cc_control_len);
	} else {
		st7701_generic_write(dev, control4, sizeof(control4));
	}

	/* Gamma Cluster Setting */
	st7701_write_dt_setting(dev, cfg->pvgamctrl, cfg->pvgamctrl_len);
	st7701_write_dt_setting(dev, cfg->nvgamctrl, cfg->nvgamctrl_len);

	/* Initial power control registers */
	st7701_generic_write(dev, ff1, sizeof(ff1));

	st7701_short_write_1p(dev, DSI_CMD2_BK1_VRHS, cfg->vrhs);
	st7701_short_write_1p(dev, DSI_CMD2_BK1_VCOM, cfg->vcom);
	st7701_short_write_1p(dev, DSI_CMD2_BK1_VGHSS, cfg->vghss);
	st7701_short_write_1p(dev, DSI_CMD2_BK1_TESTCMD, 0x80);

	st7701_short_write_1p(dev, DSI_CMD2_BK1_VGLS, cfg->vgls);
	st7701_short_write_1p(dev, DSI_CMD2_BK1_PWCTLR1, cfg->pwctlr1);

	st7701_short_write_1p(dev, DSI_CMD2_BK1_PWCTLR2, cfg->pwctlr2);
	if (cfg->has_b9) {
		st7701_short_write_1p(dev, 0xB9, cfg->b9);
	}
	st7701_short_write_1p(dev, DSI_CMD2_BK1_SPD1, cfg->spd1);
	st7701_short_write_1p(dev, DSI_CMD2_BK1_SPD2, cfg->spd2);
	st7701_short_write_1p(dev, DSI_CMD2_BK1_MIPISET1, cfg->mipiset1);
	k_msleep(100);

	/* GIP Setting */
	st7701_write_dt_setting(dev, cfg->gip_e0, cfg->gip_e0_len);
	st7701_write_dt_setting(dev, cfg->gip_e1, cfg->gip_e1_len);
	st7701_write_dt_setting(dev, cfg->gip_e2, cfg->gip_e2_len);
	st7701_write_dt_setting(dev, cfg->gip_e3, cfg->gip_e3_len);
	st7701_write_dt_setting(dev, cfg->gip_e4, cfg->gip_e4_len);
	st7701_write_dt_setting(dev, cfg->gip_e5, cfg->gip_e5_len);
	st7701_write_dt_setting(dev, cfg->gip_e6, cfg->gip_e6_len);
	st7701_write_dt_setting(dev, cfg->gip_e7, cfg->gip_e7_len);
	st7701_write_dt_setting(dev, cfg->gip_e8, cfg->gip_e8_len);
	st7701_write_dt_setting(dev, cfg->gip_e9, cfg->gip_e9_len);
	st7701_write_dt_setting(dev, cfg->gip_eb, cfg->gip_eb_len);
	st7701_write_dt_setting(dev, cfg->gip_ec, cfg->gip_ec_len);
	st7701_write_dt_setting(dev, cfg->gip_ed, cfg->gip_ed_len);
	st7701_write_dt_setting(dev, cfg->gip_ef, cfg->gip_ef_len);

	/* Bank1 setting */
	st7701_generic_write(dev, ff2, sizeof(ff2));

	/* Exit sleep mode */
	ret = st7701_dcs_write(dev, MIPI_DCS_EXIT_SLEEP_MODE, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	/* The controller requires 120 ms after Sleep Out before further
	 * commands are accepted.
	 */
	k_msleep(120);

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

	/* Always program the inversion state explicitly, the power-on
	 * state of some panels is not deterministic.
	 */
	ret = st7701_dcs_write(dev, cfg->inversion_on ? MIPI_DCS_ENTER_INVERT_MODE
						      : MIPI_DCS_EXIT_INVERT_MODE,
			       NULL, 0);
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

#ifdef CONFIG_PM_DEVICE
static int st7701_pm_action(const struct device *dev, enum pm_device_action action)
{
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = st7701_dcs_write(dev, MIPI_DCS_EXIT_SLEEP_MODE, NULL, 0);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		ret = st7701_dcs_write(dev, MIPI_DCS_ENTER_SLEEP_MODE, NULL, 0);
		break;
	default:
		return -ENOTSUP;
	}

	return ret;
}
#endif

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

/* Fail the build if a devicetree array property does not fit its config field. */
#define ST7701_ASSERT_LEN(inst, prop, field)                                                       \
	BUILD_ASSERT(DT_INST_PROP_LEN_OR(inst, prop, 0) <=                                         \
		     sizeof(((struct st7701_config *)0)->field),                                   \
		     "devicetree property " #prop " is too long for " #field)

#define ST7701_DEVICE(inst)                                                                        \
	BUILD_ASSERT((DT_INST_PROP(inst, height) % 2) == 0,                                        \
		     "Panel height must be even for the line setting encoding");                   \
	ST7701_ASSERT_LEN(inst, bk3_ef, bk3_ef);                                                   \
	ST7701_ASSERT_LEN(inst, lneset, lneset);                                                   \
	ST7701_ASSERT_LEN(inst, porctrl, porctrl);                                                 \
	ST7701_ASSERT_LEN(inst, cc_control, cc_control);                                           \
	ST7701_ASSERT_LEN(inst, invsel, invsel);                                                   \
	ST7701_ASSERT_LEN(inst, gip_e0, gip_e0);                                                   \
	ST7701_ASSERT_LEN(inst, gip_e1, gip_e1);                                                   \
	ST7701_ASSERT_LEN(inst, gip_e2, gip_e2);                                                   \
	ST7701_ASSERT_LEN(inst, gip_e3, gip_e3);                                                   \
	ST7701_ASSERT_LEN(inst, gip_e4, gip_e4);                                                   \
	ST7701_ASSERT_LEN(inst, gip_e5, gip_e5);                                                   \
	ST7701_ASSERT_LEN(inst, gip_e6, gip_e6);                                                   \
	ST7701_ASSERT_LEN(inst, gip_e7, gip_e7);                                                   \
	ST7701_ASSERT_LEN(inst, gip_e8, gip_e8);                                                   \
	ST7701_ASSERT_LEN(inst, gip_e9, gip_e9);                                                   \
	ST7701_ASSERT_LEN(inst, gip_eb, gip_eb);                                                   \
	ST7701_ASSERT_LEN(inst, gip_ec, gip_ec);                                                   \
	ST7701_ASSERT_LEN(inst, gip_ed, gip_ed);                                                   \
	ST7701_ASSERT_LEN(inst, gip_ef, gip_ef);                                                   \
	ST7701_ASSERT_LEN(inst, pvgamctrl, pvgamctrl);                                             \
	ST7701_ASSERT_LEN(inst, nvgamctrl, nvgamctrl);                                             \
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
		.inversion_on = DT_INST_PROP(inst, inversion_on),                                  \
		.bk3_ef = DT_INST_PROP_OR(inst, bk3_ef, {}),                                       \
		.bk3_ef_len = DT_INST_PROP_LEN_OR(inst, bk3_ef, 0),                                \
		.lneset = DT_INST_PROP_OR(inst, lneset, {}),                                       \
		.lneset_len = DT_INST_PROP_LEN_OR(inst, lneset, 0),                                \
		.porctrl = DT_INST_PROP_OR(inst, porctrl, {}),                                     \
		.porctrl_len = DT_INST_PROP_LEN_OR(inst, porctrl, 0),                              \
		.cc_control = DT_INST_PROP_OR(inst, cc_control, {}),                               \
		.cc_control_len = DT_INST_PROP_LEN_OR(inst, cc_control, 0),                        \
		.invsel = DT_INST_PROP_OR(inst, invsel, {}),                                       \
		.invsel_len = DT_INST_PROP_LEN_OR(inst, invsel, 0),                                \
		.vrhs = DT_INST_PROP(inst, vrhs),                                                  \
		.vcom = DT_INST_PROP(inst, vcom),                                                  \
		.vghss = DT_INST_PROP(inst, vghss),                                                \
		.vgls = DT_INST_PROP(inst, vgls),                                                  \
		.pwctlr1 = DT_INST_PROP(inst, pwctlr1),                                            \
		.pwctlr2 = DT_INST_PROP(inst, pwctlr2),                                            \
		.spd1 = DT_INST_PROP(inst, spd1),                                                  \
		.spd2 = DT_INST_PROP(inst, spd2),                                                  \
		.mipiset1 = DT_INST_PROP(inst, mipiset1),                                          \
		.b9 = DT_INST_PROP_OR(inst, b9, 0),                                                \
		.has_b9 = DT_INST_NODE_HAS_PROP(inst, b9),                                         \
		.gip_e0 = DT_INST_PROP_OR(inst, gip_e0, {}),                                       \
		.gip_e1 = DT_INST_PROP_OR(inst, gip_e1, {}),                                       \
		.gip_e2 = DT_INST_PROP_OR(inst, gip_e2, {}),                                       \
		.gip_e3 = DT_INST_PROP_OR(inst, gip_e3, {}),                                       \
		.gip_e4 = DT_INST_PROP_OR(inst, gip_e4, {}),                                       \
		.gip_e5 = DT_INST_PROP_OR(inst, gip_e5, {}),                                       \
		.gip_e6 = DT_INST_PROP_OR(inst, gip_e6, {}),                                       \
		.gip_e7 = DT_INST_PROP_OR(inst, gip_e7, {}),                                       \
		.gip_e8 = DT_INST_PROP_OR(inst, gip_e8, {}),                                       \
		.gip_e9 = DT_INST_PROP_OR(inst, gip_e9, {}),                                       \
		.gip_eb = DT_INST_PROP_OR(inst, gip_eb, {}),                                       \
		.gip_ec = DT_INST_PROP_OR(inst, gip_ec, {}),                                       \
		.gip_ed = DT_INST_PROP_OR(inst, gip_ed, {}),                                       \
		.gip_ef = DT_INST_PROP_OR(inst, gip_ef, {}),                                       \
		.pvgamctrl = DT_INST_PROP_OR(inst, pvgamctrl, {}),                                 \
		.nvgamctrl = DT_INST_PROP_OR(inst, nvgamctrl, {}),                                 \
		.gip_e0_len = DT_INST_PROP_LEN_OR(inst, gip_e0, 0),                                \
		.gip_e1_len = DT_INST_PROP_LEN_OR(inst, gip_e1, 0),                                \
		.gip_e2_len = DT_INST_PROP_LEN_OR(inst, gip_e2, 0),                                \
		.gip_e3_len = DT_INST_PROP_LEN_OR(inst, gip_e3, 0),                                \
		.gip_e4_len = DT_INST_PROP_LEN_OR(inst, gip_e4, 0),                                \
		.gip_e5_len = DT_INST_PROP_LEN_OR(inst, gip_e5, 0),                                \
		.gip_e6_len = DT_INST_PROP_LEN_OR(inst, gip_e6, 0),                                \
		.gip_e7_len = DT_INST_PROP_LEN_OR(inst, gip_e7, 0),                                \
		.gip_e8_len = DT_INST_PROP_LEN_OR(inst, gip_e8, 0),                                \
		.gip_e9_len = DT_INST_PROP_LEN_OR(inst, gip_e9, 0),                                \
		.gip_eb_len = DT_INST_PROP_LEN_OR(inst, gip_eb, 0),                                \
		.gip_ec_len = DT_INST_PROP_LEN_OR(inst, gip_ec, 0),                                \
		.gip_ed_len = DT_INST_PROP_LEN_OR(inst, gip_ed, 0),                                \
		.gip_ef_len = DT_INST_PROP_LEN_OR(inst, gip_ef, 0),                                \
		.pvgamctrl_len = DT_INST_PROP_LEN_OR(inst, pvgamctrl, 0),                          \
		.nvgamctrl_len = DT_INST_PROP_LEN_OR(inst, nvgamctrl, 0),                          \
	};                                                                                         \
	static struct st7701_data st7701_data_##inst = {                                           \
		.dsi_pixel_format = DT_INST_PROP(inst, pixel_format),                              \
	};                                                                                         \
	PM_DEVICE_DT_INST_DEFINE(inst, st7701_pm_action);                                          \
	DEVICE_DT_INST_DEFINE(inst, &st7701_init, PM_DEVICE_DT_INST_GET(inst), &st7701_data_##inst,\
			      &st7701_config_##inst, POST_KERNEL,                                  \
			      CONFIG_DISPLAY_INIT_PRIORITY, &st7701_api);

DT_INST_FOREACH_STATUS_OKAY(ST7701_DEVICE)
