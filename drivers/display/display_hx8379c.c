/*
 * Copyright 2025, Charles Dias
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Driver implementation based on ST code sample DSI_VideoMode_SingleBuffer from :
 * https://github.com/STMicroelectronics/STM32CubeU5/tree/main/Projects/STM32U5x9J-DK/
 */

#define DT_DRV_COMPAT himax_hx8379c

#include <zephyr/drivers/display.h>
#include <zephyr/drivers/mipi_dsi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(display_hx8379c, CONFIG_DISPLAY_LOG_LEVEL);

/* MIPI DCS commands specific to this display driver */

/* Set power */
#define HX8379C_SETPOWER		0xB1
/* Set display related register */
#define HX8379C_SETDISP			0xB2
/* Set display cycle timing */
#define HX8379C_SETCYC			0xB4
/* Set VCOM voltage */
#define HX8379C_SETVCOM			0xB6
/* Set extended command set */
#define HX8379C_SETEXTC			0xB9
/* Set register bank partition index */
#define HX8379C_SETBANK			0xBD
/* Set DGC LUT */
#define HX8379C_SETDGC_LUT		0xC1
/*
 * Register 0xC7 is not mentioned in the datasheet, but searching the internet,
 * it is available for other Himax displays as SETTCON.
 */
#define HX8379C_SETTCON			0xC7
/* Set panel related register */
#define HX8379C_SETPANEL		0xCC
/* SETOFFSET */
#define HX8379C_SETOFFSET		0xD2
/* Set GIP timing */
#define HX8379C_SETGIP_0		0xD3
/* Set forward GIP sequence */
#define HX8379C_SETGIP_1		0xD5
/* Set backward GIP sequence */
#define HX8379C_SETGIP_2		0xD6
/* Set gamma curve related setting */
#define HX8379C_SETGAMMA		0xE0

struct hx8379c_config {
	const struct device *mipi_dsi;
	const struct gpio_dt_spec reset_gpio;
	uint16_t panel_width;
	uint16_t panel_height;
	uint16_t hsync;
	uint16_t hbp;
	uint16_t hfp;
	uint16_t vfp;
	uint16_t vbp;
	uint16_t vsync;
	uint8_t data_lanes;
	uint8_t pixel_format;
	uint8_t channel;
};

static int hx8379c_transmit(const struct device *dev, uint8_t cmd,
			    const void *tx_data, size_t tx_len)
{
	const struct hx8379c_config *config = dev->config;

	return mipi_dsi_dcs_write(config->mipi_dsi, config->channel, cmd, tx_data, tx_len);
}


static int hx8379c_blanking_on(const struct device *dev)
{
	int ret;

	ret = hx8379c_transmit(dev, MIPI_DCS_SET_DISPLAY_OFF, NULL, 0);
	if (ret < 0) {
		LOG_ERR("Failed to turn off display (%d)", ret);
		return ret;
	}

	return 0;
}

static int hx8379c_blanking_off(const struct device *dev)
{
	int ret;

	ret = hx8379c_transmit(dev, MIPI_DCS_SET_DISPLAY_ON, NULL, 0);
	if (ret < 0) {
		LOG_ERR("Failed to turn on display (%d)", ret);
		return ret;
	}

	return 0;
}

static void hx8379c_get_capabilities(const struct device *dev,
				     struct display_capabilities *capabilities)
{
	const struct hx8379c_config *config = dev->config;

	memset(capabilities, 0, sizeof(struct display_capabilities));
	capabilities->x_resolution = config->panel_width;
	capabilities->y_resolution = config->panel_height;
	capabilities->supported_pixel_formats = (PIXEL_FORMAT_RGB_565 | PIXEL_FORMAT_RGB_888);
	capabilities->current_pixel_format = config->pixel_format;
	capabilities->current_orientation = DISPLAY_ORIENTATION_NORMAL;
}

static DEVICE_API(display, hx8379c_api) = {
	.blanking_on = hx8379c_blanking_on,
	.blanking_off = hx8379c_blanking_off,
	.get_capabilities = hx8379c_get_capabilities,
};

static int hx8379c_configure(const struct device *dev)
{
	int ret;

	LOG_DBG("Configuring HX8379C DSI controller...");

	/* CMD Mode */
	static const uint8_t enable_extension[3] = {0xFF, 0x83, 0x79};

	ret = hx8379c_transmit(dev, HX8379C_SETEXTC, enable_extension, sizeof(enable_extension));
	if (ret < 0) {
		LOG_ERR("Controller init step 1 failed (%d)", ret);
		return ret;
	}

	static const uint8_t power_config[16] = {0x44, 0x1C, 0x1C, 0x37, 0x57, 0x90, 0xD0, 0xE2,
						 0x58, 0x80, 0x38, 0x38, 0xF8, 0x33, 0x34, 0x42};

	ret = hx8379c_transmit(dev, HX8379C_SETPOWER, power_config, sizeof(power_config));
	if (ret < 0) {
		LOG_ERR("Controller SETPOWER failed (%d)", ret);
		return ret;
	}

	/*
	 * NOTE: Parameter count discrepancy with datasheet HX8379C datasheet specifies 6
	 * parameters for SETDISP (0xB2) command, but STM32Cube reference implementation uses 9
	 * parameters. This follows the STM32 implementation which has been validated on actual
	 * hardware. The extra parameters may be undocumented extensions or revision-specific.
	 */
	static const uint8_t line_config[9] = {0x80, 0x14, 0x0C, 0x30, 0x20, 0x50, 0x11, 0x42,
					       0x1D};

	ret = hx8379c_transmit(dev, HX8379C_SETDISP, line_config, sizeof(line_config));
	if (ret < 0) {
		LOG_ERR("Controller SETDISP failed (%d)", ret);
		return ret;
	}

	static const uint8_t cycle_config[10] = {0x01, 0xAA, 0x01, 0xAF, 0x01, 0xAF, 0x10, 0xEA,
						 0x1C, 0xEA};

	ret = hx8379c_transmit(dev, HX8379C_SETCYC, cycle_config, sizeof(cycle_config));
	if (ret < 0) {
		LOG_ERR("Controller timing config failed (%d)", ret);
		return ret;
	}

	static const uint8_t tcon_config[4] = {0x00, 0x00, 0x00, 0xC0};

	ret = hx8379c_transmit(dev, HX8379C_SETTCON, tcon_config, sizeof(tcon_config));
	if (ret < 0) {
		LOG_ERR("Controller SETTCON failed (%d)", ret);
		return ret;
	}

	static const uint8_t panel_config[1] = {0x02};

	ret = hx8379c_transmit(dev, HX8379C_SETPANEL, panel_config, sizeof(panel_config));
	if (ret < 0) {
		LOG_ERR("Controller register SETPANEL failed (%d)", ret);
		return ret;
	}

	static const uint8_t offset_config[1] = {0x77};

	ret = hx8379c_transmit(dev, HX8379C_SETOFFSET, offset_config, sizeof(offset_config));
	if (ret < 0) {
		LOG_ERR("Controller register SETOFFSET failed (%d)", ret);
		return ret;
	}

	/*
	 * NOTE: Parameter count discrepancy with datasheet HX8379C datasheet specifies 29
	 * parameters for SETGIP_0 (0xD3) command, but STM32Cube reference implementation uses 37
	 * parameters. This follows the STM32 implementation which has been validated on actual
	 * hardware. The difference may be due to datasheet version or implementation optimization.
	 */
	static const uint8_t gip0_config[37] = {0x00, 0x07, 0x00, 0x00, 0x00, 0x08, 0x08, 0x32,
						0x10, 0x01, 0x00, 0x01, 0x03, 0x72, 0x03, 0x72,
						0x00, 0x08, 0x00, 0x08, 0x33, 0x33, 0x05, 0x05,
						0x37, 0x05, 0x05, 0x37, 0x0A, 0x00, 0x00, 0x00,
						0x0A, 0x00, 0x01, 0x00, 0x0E};

	ret = hx8379c_transmit(dev, HX8379C_SETGIP_0, gip0_config, sizeof(gip0_config));
	if (ret < 0) {
		LOG_ERR("Controller register SETGIP_0 failed (%d)", ret);
		return ret;
	}

	/*
	 * NOTE: Parameter count discrepancy with datasheet HX8379C datasheet specifies 35
	 * parameters for SETGIP_1 (0xD5) command, but STM32Cube reference implementation uses 34
	 * parameters. This follows the STM32 implementation which has been validated on actual
	 * hardware. The difference may be due to datasheet version or implementation optimization.
	 */
	static const uint8_t gip1_config[34] = {0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
						0x19, 0x19, 0x18, 0x18, 0x18, 0x18, 0x19, 0x19,
						0x01, 0x00, 0x03, 0x02, 0x05, 0x04, 0x07, 0x06,
						0x23, 0x22, 0x21, 0x20, 0x18, 0x18, 0x18, 0x18,
						0x00, 0x00};

	ret = hx8379c_transmit(dev, HX8379C_SETGIP_1, gip1_config, sizeof(gip1_config));
	if (ret < 0) {
		LOG_ERR("Controller register SETGIP_1 failed (%d)", ret);
		return ret;
	}

	static const uint8_t gip2_config[32] = {0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
						0x19, 0x19, 0x18, 0x18, 0x19, 0x19, 0x18, 0x18,
						0x06, 0x07, 0x04, 0x05, 0x02, 0x03, 0x00, 0x01,
						0x20, 0x21, 0x22, 0x23, 0x18, 0x18, 0x18, 0x18};

	ret = hx8379c_transmit(dev, HX8379C_SETGIP_2, gip2_config, sizeof(gip2_config));
	if (ret < 0) {
		LOG_ERR("Controller register SETGIP_2 failed (%d)", ret);
		return ret;
	}

	static const uint8_t gamma_config[42] = {0x00, 0x16, 0x1B, 0x30, 0x36, 0x3F, 0x24, 0x40,
						 0x09, 0x0D, 0x0F, 0x18, 0x0E, 0x11, 0x12, 0x11,
						 0x14, 0x07, 0x12, 0x13, 0x18, 0x00, 0x17, 0x1C,
						 0x30, 0x36, 0x3F, 0x24, 0x40, 0x09, 0x0C, 0x0F,
						 0x18, 0x0E, 0x11, 0x14, 0x11, 0x12, 0x07, 0x12,
						 0x14, 0x18};

	ret = hx8379c_transmit(dev, HX8379C_SETGAMMA, gamma_config, sizeof(gamma_config));
	if (ret < 0) {
		LOG_ERR("Controller gamma configuration failed (%d)", ret);
		return ret;
	}

	static const uint8_t vcom_config[3] = {0x2C, 0x2C, 0x00};

	ret = hx8379c_transmit(dev, HX8379C_SETVCOM, vcom_config, sizeof(vcom_config));
	if (ret < 0) {
		LOG_ERR("Controller register SETVCOM failed (%d)", ret);
		return ret;
	}

	static const uint8_t bank0_config[1] = {0x00};

	ret = hx8379c_transmit(dev, HX8379C_SETBANK, bank0_config, sizeof(bank0_config));
	if (ret < 0) {
		LOG_ERR("Controller register HX8379C_SETBANK(0) failed (%d)", ret);
		return ret;
	}

	static const uint8_t lut1_config[43] = {0x01, 0x00, 0x07, 0x0F, 0x16, 0x1F, 0x27, 0x30,
						0x38, 0x40, 0x47, 0x4E, 0x56, 0x5D, 0x65, 0x6D,
						0x74, 0x7D, 0x84, 0x8A, 0x90, 0x99, 0xA1, 0xA9,
						0xB0, 0xB6, 0xBD, 0xC4, 0xCD, 0xD4, 0xDD, 0xE5,
						0xEC, 0xF3, 0x36, 0x07, 0x1C, 0xC0, 0x1B, 0x01,
						0xF1, 0x34, 0x00};

	ret = hx8379c_transmit(dev, HX8379C_SETDGC_LUT, lut1_config, sizeof(lut1_config));
	if (ret < 0) {
		LOG_ERR("Controller color LUT 1 failed (%d)", ret);
		return ret;
	}

	static const uint8_t bank1_config[1] = {0x01};

	ret = hx8379c_transmit(dev, HX8379C_SETBANK, bank1_config, sizeof(bank1_config));
	if (ret < 0) {
		LOG_ERR("Controller register HX8379C_SETBANK(1) failed (%d)", ret);
		return ret;
	}

	static const uint8_t lut2_config[42] = {0x00, 0x08, 0x0F, 0x16, 0x1F, 0x28, 0x31, 0x39,
						0x41, 0x48, 0x51, 0x59, 0x60, 0x68, 0x70, 0x78,
						0x7F, 0x87, 0x8D, 0x94, 0x9C, 0xA3, 0xAB, 0xB3,
						0xB9, 0xC1, 0xC8, 0xD0, 0xD8, 0xE0, 0xE8, 0xEE,
						0xF5, 0x3B, 0x1A, 0xB6, 0xA0, 0x07, 0x45, 0xC5,
						0x37, 0x00};

	ret = hx8379c_transmit(dev, HX8379C_SETDGC_LUT, lut2_config, sizeof(lut2_config));
	if (ret < 0) {
		LOG_ERR("Controller color LUT 2 failed (%d)", ret);
		return ret;
	}

	static const uint8_t bank2_config[1] = {0x02};

	ret = hx8379c_transmit(dev, HX8379C_SETBANK, bank2_config, sizeof(bank2_config));
	if (ret < 0) {
		LOG_ERR("Controller register HX8379C_SETBANK(2) failed (%d)", ret);
		return ret;
	}

	static const uint8_t lut3_config[42] = {0x00, 0x09, 0x0F, 0x18, 0x21, 0x2A, 0x34, 0x3C,
						0x45, 0x4C, 0x56, 0x5E, 0x66, 0x6E, 0x76, 0x7E,
						0x87, 0x8E, 0x95, 0x9D, 0xA6, 0xAF, 0xB7, 0xBD,
						0xC5, 0xCE, 0xD5, 0xDF, 0xE7, 0xEE, 0xF4, 0xFA,
						0xFF, 0x0C, 0x31, 0x83, 0x3C, 0x5B, 0x56, 0x1E,
						0x5A, 0xFF};

	ret = hx8379c_transmit(dev, HX8379C_SETDGC_LUT, lut3_config, sizeof(lut3_config));
	if (ret < 0) {
		LOG_ERR("Controller color LUT 3 failed (%d)", ret);
		return ret;
	}

	static const uint8_t bank00_config[1] = {0x00};

	ret = hx8379c_transmit(dev, HX8379C_SETBANK, bank00_config, sizeof(bank00_config));
	if (ret < 0) {
		LOG_ERR("Controller register HX8379C_SETBANK(0) failed (%d)", ret);
		return ret;
	}

	/* Exit Sleep Mode */
	ret = hx8379c_transmit(dev, MIPI_DCS_EXIT_SLEEP_MODE, NULL, 0);
	if (ret < 0) {
		LOG_ERR("Exit sleep mode failed (%d)", ret);
		return ret;
	}

	k_msleep(120);

	/* Display On */
	ret = hx8379c_blanking_off(dev);
	if (ret < 0) {
		LOG_ERR("Display blanking off failed (%d)", ret);
		return ret;
	}

	k_msleep(120);

	LOG_DBG("Display Controller configured successfully");
	return 0;
}

static int hx8379c_init(const struct device *dev)
{
	const struct hx8379c_config *config = dev->config;
	struct mipi_dsi_device mdev;
	int ret;

	if (config->reset_gpio.port) {
		if (!gpio_is_ready_dt(&config->reset_gpio)) {
			LOG_ERR("Reset GPIO device is not ready");
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to configure reset GPIO (%d)", ret);
			return ret;
		}
		k_msleep(11);
		ret = gpio_pin_set_dt(&config->reset_gpio, 1);
		if (ret < 0) {
			LOG_ERR("Failed to activate reset GPIO (%d)", ret);
			return ret;
		}
		k_msleep(150);
	}

	/* attach to MIPI-DSI host */
	mdev.data_lanes = config->data_lanes;
	mdev.pixfmt = config->pixel_format;
	mdev.mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST | MIPI_DSI_MODE_LPM;

	mdev.timings.hactive = config->panel_width;
	mdev.timings.hsync = config->hsync;
	mdev.timings.hbp = config->hbp;
	mdev.timings.hfp = config->hfp;
	mdev.timings.vactive = config->panel_height;
	mdev.timings.vfp = config->vfp;
	mdev.timings.vbp = config->vbp;
	mdev.timings.vsync = config->vsync;

	ret = mipi_dsi_attach(config->mipi_dsi, config->channel, &mdev);
	if (ret < 0) {
		LOG_ERR("Failed to attach to MIPI-DSI host (%d)", ret);
		return ret;
	}

	ret = hx8379c_configure(dev);
	if (ret < 0) {
		LOG_ERR("Failed to configure display (%d)", ret);
		return ret;
	}

	LOG_DBG("HX8379C display controller initialized successfully");
	return 0;
}

#define HX8379C_CONTROLLER_DEVICE(inst)							\
	static const struct hx8379c_config hx8379c_config_##inst = {			\
		.mipi_dsi = DEVICE_DT_GET(DT_INST_BUS(inst)),				\
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),		\
		.data_lanes = DT_INST_PROP_BY_IDX(inst, data_lanes, 0),			\
		.panel_width = DT_INST_PROP(inst, width),				\
		.panel_height = DT_INST_PROP(inst, height),				\
		.pixel_format = DT_INST_PROP(inst, pixel_format),			\
		.channel = DT_INST_REG_ADDR(inst),					\
		.hsync = DT_PROP(DT_INST_CHILD(inst, display_timings), hsync_len),	\
		.hbp = DT_PROP(DT_INST_CHILD(inst, display_timings), hback_porch),	\
		.hfp = DT_PROP(DT_INST_CHILD(inst, display_timings), hfront_porch),	\
		.vsync = DT_PROP(DT_INST_CHILD(inst, display_timings), vsync_len),	\
		.vbp = DT_PROP(DT_INST_CHILD(inst, display_timings), vback_porch),	\
		.vfp = DT_PROP(DT_INST_CHILD(inst, display_timings), vfront_porch),	\
	};										\
	DEVICE_DT_INST_DEFINE(inst, &hx8379c_init, NULL,				\
			    NULL, &hx8379c_config_##inst, POST_KERNEL,			\
			    CONFIG_DISPLAY_HX8379C_INIT_PRIORITY, &hx8379c_api);

DT_INST_FOREACH_STATUS_OKAY(HX8379C_CONTROLLER_DEVICE)
