/*
 * Copyright 2023, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT himax_hx8394

#include <zephyr/drivers/display.h>
#include <zephyr/drivers/mipi_dsi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(hx8394, CONFIG_DISPLAY_LOG_LEVEL);

struct hx8394_config {
	const struct device *mipi_dsi;
	const struct gpio_dt_spec reset_gpio;
	const struct gpio_dt_spec bl_gpio;
	uint8_t num_of_lanes;
	uint8_t pixel_format;
	uint16_t panel_width;
	uint16_t panel_height;
	uint8_t channel;
};

/* MIPI DCS commands specific to this display driver */
#define HX8394_SETMIPI 0xBA
#define HX8394_MIPI_LPTX_BTA_READ BIT(6)
#define HX8394_MIPI_LP_CD_DIS BIT(5)
#define HX8394_MIPI_TA_6TL 0x3
#define HX8394_MIPI_DPHYCMD_LPRX_8NS 0x40
#define HX8394_MIPI_DPHYCMD_LPRX_66mV 0x20
#define HX8394_MIPI_DPHYCMD_LPTX_SRLIM 0x8
#define HX8394_MIPI_DPHYCMD_LDO_1_55V 0x60
#define HX8394_MIPI_DPHYCMD_HSRX_7X 0x8
#define HX8394_MIPI_DPHYCMD_HSRX_100OHM 0x2
#define HX8394_MIPI_DPHYCMD_LPCD_1X 0x1

#define HX8394_SET_ADDRESS 0x36
#define HX8394_FLIP_HORIZONTAL BIT(1)
#define HX8394_FLIP_VERTICAL BIT(0)

#define HX8394_SETPOWER 0xB1
#define HX8394_POWER_AP_1_0UA 0x8
#define HX8394_POWER_HX5186 0x40
#define HX8394_POWER_VRHP_4_8V 0x12
#define HX8394_POWER_VRHN_4_8V 0x12
#define HX8394_POWER_VPPS_8_25V 0x60
#define HX8394_POWER_XDK_X2 0x1
#define HX8394_POWER_VSP_FBOFF 0x8
#define HX8394_POWER_FS0_DIV_8 0x2
#define HX8394_POWER_CLK_OPT_VGH_HSYNC_RST 0x10
#define HX8394_POWER_CLK_OPT_VGL_HSYNC_RST 0x20
#define HX8394_POWER_FS2_DIV_192 0x4
#define HX8394_POWER_FS1_DIV_224 0x50
#define HX8394_POWER_BTP_5_55V 0x11
#define HX8394_POWER_VGH_RATIO_2VSPVSN 0x60
#define HX8394_POWER_BTN_5_55V 0x11
#define HX8394_POWER_VGL_RATIO_2VSPVSN 0x60
#define HX8394_POWER_VGHS_16V 0x57
#define HX8394_POWER_VGLS_12_4V 0x47

#define HX8394_SETDISP 0xB2
#define HX8394_DISP_COL_INV 0x0
#define HX8394_DISP_MESSI_ENB 0x80
#define HX8394_DISP_NL_1280 0x64
#define HX8394_DISP_BP_14 0xC
#define HX8394_DISP_FP_15 0xD
#define HX8394_DISP_RTN_144 0x2F

#define HX8394_SETCYC 0xB4

#define HX8394_SETGIP0 0xD3
#define HX8394_GIP0_EQ_OPT_BOTH 0x0
#define HX8394_GIP0_EQ_HSYNC_NORMAL 0x0
#define HX8394_GIP0_EQ_VSEL_VSSA 0x0
#define HX8394_SHP_START_4 0x40
#define HX8394_SCP_WIDTH_7X_HSYNC 0x7
#define HX8394_CHR0_12X_HSYNC 0xA
#define HX8394_CHR1_18X_HSYNC 0x10

#define HX8394_SETGIP1 0xD5

#define HX8394_SETGIP2 0xD6

#define HX8394_SETVCOM 0xB6
#define HX8394_VCMC_F_1_76V 0x92
#define HX8394_VCMC_B_1_76V 0x92

#define HX8394_SETGAMMA 0xE0

#define HX8394_SETPANEL 0xCC
#define HX8394_COLOR_BGR BIT(0)
#define HX8394_REV_PANEL BIT(1)

#define HX8394_SETBANK 0xBD

#define HX8394_SET_TEAR 0x35
#define HX8394_TEAR_VBLANK 0x0

#define HX8394_SETEXTC 0xB9
#define HX8394_EXTC1_MAGIC 0xFF
#define HX8394_EXTC2_MAGIC 0x83
#define HX8394_EXTC3_MAGIC 0x94


const uint8_t enable_extension[] = {
	HX8394_SETEXTC,
	HX8394_EXTC1_MAGIC,
	HX8394_EXTC2_MAGIC,
	HX8394_EXTC3_MAGIC,
};

const uint8_t address_config[] = {
	HX8394_SET_ADDRESS,
	HX8394_FLIP_HORIZONTAL
};

const uint8_t power_config[] = {
	HX8394_SETPOWER,
	(HX8394_POWER_HX5186 | HX8394_POWER_AP_1_0UA),
	HX8394_POWER_VRHP_4_8V,
	(HX8394_POWER_VPPS_8_25V | HX8394_POWER_VRHN_4_8V),
	(HX8394_POWER_VSP_FBOFF | HX8394_POWER_XDK_X2),
	(HX8394_POWER_CLK_OPT_VGL_HSYNC_RST |
		    HX8394_POWER_CLK_OPT_VGH_HSYNC_RST |
		    HX8394_POWER_FS0_DIV_8),
	(HX8394_POWER_FS1_DIV_224 | HX8394_POWER_FS2_DIV_192),
	(HX8394_POWER_VGH_RATIO_2VSPVSN | HX8394_POWER_BTP_5_55V),
	(HX8394_POWER_VGL_RATIO_2VSPVSN | HX8394_POWER_BTN_5_55V),
	HX8394_POWER_VGHS_16V,
	HX8394_POWER_VGLS_12_4V
};

const uint8_t line_config[] = {
	HX8394_SETDISP,
	HX8394_DISP_COL_INV,
	HX8394_DISP_MESSI_ENB,
	HX8394_DISP_NL_1280,
	HX8394_DISP_BP_14,
	HX8394_DISP_FP_15,
	HX8394_DISP_RTN_144
};

const uint8_t cycle_config[] = {
	HX8394_SETCYC,
	0x73, /* SPON delay */
	0x74, /* SPOFF delay */
	0x73, /* CON delay */
	0x74, /* COFF delay */
	0x73, /* CON1 delay */
	0x74, /* COFF1 delay */
	0x1, /* EQON time */
	0xC, /* SON time */
	0x86, /* SOFF time */
	0x75, /* SAP1_P, SAP2 (1st and second stage op amp bias) */
	0x00, /* DX2 off, EQ off, EQ_MI off */
	0x3F, /* DX2 off period setting */
	0x73, /* SPON_MPU delay */
	0x74, /* SPOFF_MPU delay */
	0x73, /* CON_MPU delay */
	0x74, /* COFF_MPU delay */
	0x73, /* CON1_MPU delay */
	0x74, /* COFF1_MPU delay */
	0x1, /* EQON_MPU time */
	0xC, /* SON_MPU time */
	0x86 /* SOFF_MPU time */
};

const uint8_t gip0_config[] = {
	HX8394_SETGIP0,
	(HX8394_GIP0_EQ_OPT_BOTH | HX8394_GIP0_EQ_HSYNC_NORMAL),
	HX8394_GIP0_EQ_VSEL_VSSA,
	0x7, /* EQ_DELAY_ON1 (in cycles of TCON CLK */
	0x7, /* EQ_DELAY_OFF1 (in cycles of TCON CLK */
	0x40, /* GPWR signal frequency (64x per frame) */
	0x7, /* GPWR signal non overlap timing (in cycles of TCON */
	0xC, /* GIP dummy clock for first CKV */
	0x00, /* GIP dummy clock for second CKV */
	/* Group delays. Sets start/end signal delay from VYSNC
	 * falling edge in multiples of HSYNC
	 */
	0x8, /* SHR0_2 = 8, SHR0_3 = 0 */
	0x10, /* SHR0_1 = 1, SHR0[11:8] = 0x0 */
	0x8, /* SHR0 = 0x8 */
	0x0, /* SHR0_GS[11:8]. Unset. */
	0x8, /* SHR0_GS = 0x8 */
	0x54, /* SHR1_3 = 0x5, SHR1_2 = 0x4 */
	0x15, /* SHR1_1 = 0x1, SHR1[11:8] = 0x5 */
	0xA, /* SHR1[7:0] = 0xA (SHR1 = 0x50A) */
	0x5, /* SHR1_GS[11:8] = 0x5 */
	0xA, /* SHR1_GS[7:0] = 0xA (SHR1_GS = 0x50A) */
	0x2, /* SHR2_3 = 0x0, SHR2_2 = 0x2 */
	0x15, /* SHR2_1 = 0x1, SHR2[11:8] = 0x5 */
	0x6, /* SHR2[7:0] = 0x6 (SHR2 = 0x506) */
	0x5, /* SHR2_GS[11:8] = 0x5 */
	0x6, /* SHR2_GS[7:0 = 0x6 (SHR2_GS = 0x506) */
	(HX8394_SHP_START_4 | HX8394_SCP_WIDTH_7X_HSYNC),
	0x44, /* SHP2 = 0x4, SHP1 = 0x4 */
	HX8394_CHR0_12X_HSYNC,
	HX8394_CHR0_12X_HSYNC,
	0x4B, /* CHP0 = 4x hsync, CCP0 = 0xB */
	HX8394_CHR1_18X_HSYNC,
	0x7, /* CHR1_GS = 9x hsync */
	0x7, /* CHP1 = 1x hsync, CCP1 = 0x7 */
	/* These parameters are not documented in datasheet */
	0xC,
	0x40
};

const uint8_t gip1_config[] = {
	HX8394_SETGIP1,
	/* Select output clock sources
	 * See COSn_L/COSn_R values in datasheet
	 */
	0x1C, /* COS1_L */
	0x1C, /* COS1_R */
	0x1D, /* COS2_L */
	0x1D, /* COS2_R */
	0x00, /* COS3_L */
	0x01, /* COS3_R */
	0x02, /* COS4_L */
	0x03, /* COS4_R */
	0x04, /* COS5_L */
	0x05, /* COS5_R */
	0x06, /* COS6_L */
	0x07, /* COS6_R */
	0x08, /* COS7_L */
	0x09, /* COS7_R */
	0x0A, /* COS8_L */
	0x0B, /* COS8_R */
	0x24, /* COS9_L */
	0x25, /* COS9_R */
	0x18, /* COS10_L */
	0x18, /* COS10_R */
	0x26, /* COS11_L */
	0x27, /* COS11_R */
	0x18, /* COS12_L */
	0x18, /* COS12_R */
	0x18, /* COS13_L */
	0x18, /* COS13_R */
	0x18, /* COS14_L */
	0x18, /* COS14_R */
	0x18, /* COS15_L */
	0x18, /* COS15_R */
	0x18, /* COS16_L */
	0x18, /* COS16_R */
	0x18, /* COS17_L */
	0x18, /* COS17_R */
	0x18, /* COS18_L */
	0x18, /* COS18_R */
	0x18, /* COS19_L */
	0x18, /* COS19_R */
	0x20, /* COS20_L */
	0x21, /* COS20_R */
	0x18, /* COS21_L */
	0x18, /* COS21_R */
	0x18, /* COS22_L */
	0x18 /* COS22_R */
};

const uint8_t gip2_config[] = {
	HX8394_SETGIP2,
	/* Select output clock sources for GS mode.
	 * See COSn_L_GS/COSn_R_GS values in datasheet
	 */
	0x1C, /* COS1_L_GS */
	0x1C, /* COS1_R_GS */
	0x1D, /* COS2_L_GS */
	0x1D, /* COS2_R_GS */
	0x07, /* COS3_L_GS */
	0x06, /* COS3_R_GS */
	0x05, /* COS4_L_GS */
	0x04, /* COS4_R_GS */
	0x03, /* COS5_L_GS */
	0x02, /* COS5_R_GS */
	0x01, /* COS6_L_GS */
	0x00, /* COS6_R_GS */
	0x0B, /* COS7_L_GS */
	0x0A, /* COS7_R_GS */
	0x09, /* COS8_L_GS */
	0x08, /* COS8_R_GS */
	0x21, /* COS9_L_GS */
	0x20, /* COS9_R_GS */
	0x18, /* COS10_L_GS */
	0x18, /* COS10_R_GS */
	0x27, /* COS11_L_GS */
	0x26, /* COS11_R_GS */
	0x18, /* COS12_L_GS */
	0x18, /* COS12_R_GS */
	0x18, /* COS13_L_GS */
	0x18, /* COS13_R_GS */
	0x18, /* COS14_L_GS */
	0x18, /* COS14_R_GS */
	0x18, /* COS15_L_GS */
	0x18, /* COS15_R_GS */
	0x18, /* COS16_L_GS */
	0x18, /* COS16_R_GS */
	0x18, /* COS17_L_GS */
	0x18, /* COS17_R_GS */
	0x18, /* COS18_L_GS */
	0x18, /* COS18_R_GS */
	0x18, /* COS19_L_GS */
	0x18, /* COS19_R_GS */
	0x25, /* COS20_L_GS */
	0x24, /* COS20_R_GS */
	0x18, /* COS21_L_GS */
	0x18, /* COS21_R_GS */
	0x18, /* COS22_L_GS */
	0x18  /* COS22_R_GS */
};

const uint8_t vcom_config[] = {
	HX8394_SETVCOM,
	HX8394_VCMC_F_1_76V,
	HX8394_VCMC_B_1_76V
};

const uint8_t gamma_config[] = {
	HX8394_SETGAMMA,
	0x00, /* VHP0 */
	0x0A, /* VHP1 */
	0x15, /* VHP2 */
	0x1B, /* VHP3 */
	0x1E, /* VHP4 */
	0x21, /* VHP5 */
	0x24, /* VHP6 */
	0x22, /* VHP7 */
	0x47, /* VMP0 */
	0x56, /* VMP1 */
	0x65, /* VMP2 */
	0x66, /* VMP3 */
	0x6E, /* VMP4 */
	0x82, /* VMP5 */
	0x88, /* VMP6 */
	0x8B, /* VMP7 */
	0x9A, /* VMP8 */
	0x9D, /* VMP9 */
	0x98, /* VMP10 */
	0xA8, /* VMP11 */
	0xB9, /* VMP12 */
	0x5D, /* VLP0 */
	0x5C, /* VLP1 */
	0x61, /* VLP2 */
	0x66, /* VLP3 */
	0x6A, /* VLP4 */
	0x6F, /* VLP5 */
	0x7F, /* VLP6 */
	0x7F, /* VLP7 */
	0x00, /* VHN0 */
	0x0A, /* VHN1 */
	0x15, /* VHN2 */
	0x1B, /* VHN3 */
	0x1E, /* VHN4 */
	0x21, /* VHN5 */
	0x24, /* VHN6 */
	0x22, /* VHN7 */
	0x47, /* VMN0 */
	0x56, /* VMN1 */
	0x65, /* VMN2 */
	0x65, /* VMN3 */
	0x6E, /* VMN4 */
	0x81, /* VMN5 */
	0x87, /* VMN6 */
	0x8B, /* VMN7 */
	0x98, /* VMN8 */
	0x9D, /* VMN9 */
	0x99, /* VMN10 */
	0xA8, /* VMN11 */
	0xBA, /* VMN12 */
	0x5D, /* VLN0 */
	0x5D, /* VLN1 */
	0x62, /* VLN2 */
	0x67, /* VLN3 */
	0x6B, /* VLN4 */
	0x72, /* VLN5 */
	0x7F, /* VLN6 */
	0x7F  /* VLN7 */
};

const uint8_t hx8394_cmd1[] = {0xC0U, 0x1FU, 0x31U};

const uint8_t panel_config[] = {
	HX8394_SETPANEL,
	(HX8394_COLOR_BGR | HX8394_REV_PANEL)
};

const uint8_t hx8394_cmd2[] = {0xD4, 0x2};

const uint8_t hx8394_bank2[] = {
	0xD8U, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
	0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
	0xFFU
};

const uint8_t hx8394_bank1[] = {0xB1U, 0x00U};

const uint8_t hx8394_bank0[] = {
	0xBFU, 0x40U, 0x81U, 0x50U,
	0x00U, 0x1AU, 0xFCU, 0x01
};

const uint8_t hx8394_cmd3[] = {0xC6U, 0xEDU};

const uint8_t tear_config[] = {HX8394_SET_TEAR, HX8394_TEAR_VBLANK};

static ssize_t hx8394_mipi_tx(const struct device *mipi_dev, uint8_t channel,
			      const void *buf, size_t len)
{
	/* Send MIPI transfers using low power mode */
	struct mipi_dsi_msg msg = {
		.tx_buf = buf,
		.tx_len = len,
		.flags = MIPI_DSI_MSG_USE_LPM,
	};

	switch (len) {
	case 0U:
		msg.type = MIPI_DSI_GENERIC_SHORT_WRITE_0_PARAM;
		break;

	case 1U:
		msg.type = MIPI_DSI_GENERIC_SHORT_WRITE_1_PARAM;
		break;

	case 2U:
		msg.type = MIPI_DSI_GENERIC_SHORT_WRITE_2_PARAM;
		break;

	default:
		msg.type = MIPI_DSI_GENERIC_LONG_WRITE;
		break;
	}

	return mipi_dsi_transfer(mipi_dev, channel, &msg);
}

static int hx8394_write(const struct device *dev, const uint16_t x,
			 const uint16_t y,
			 const struct display_buffer_descriptor *desc,
			 const void *buf)
{
	LOG_WRN("Write not supported, use LCD controller display driver");
	return 0;
}

static int hx8394_blanking_off(const struct device *dev)
{
	const struct hx8394_config *config = dev->config;

	if (config->bl_gpio.port != NULL) {
		return gpio_pin_set_dt(&config->bl_gpio, 1);
	} else {
		return -ENOTSUP;
	}
}

static int hx8394_blanking_on(const struct device *dev)
{
	const struct hx8394_config *config = dev->config;

	if (config->bl_gpio.port != NULL) {
		return gpio_pin_set_dt(&config->bl_gpio, 0);
	} else {
		return -ENOTSUP;
	}
}

static int hx8394_set_pixel_format(const struct device *dev,
				    const enum display_pixel_format pixel_format)
{
	const struct hx8394_config *config = dev->config;

	if (pixel_format == config->pixel_format) {
		return 0;
	}
	LOG_WRN("Pixel format change not implemented");
	return -ENOTSUP;
}

static int hx8394_set_orientation(const struct device *dev,
				   const enum display_orientation orientation)
{
	const struct hx8394_config *config = dev->config;
	uint8_t param[2] = {0};

	/* Note- this simply flips the scan direction of the display
	 * driver. Can be useful if your application needs the display
	 * flipped on the X or Y axis
	 */
	param[0] = HX8394_SET_ADDRESS;
	switch (orientation) {
	case DISPLAY_ORIENTATION_NORMAL:
		/* Default orientation for this display flips image on x axis */
		param[1] = HX8394_FLIP_HORIZONTAL;
		break;
	case DISPLAY_ORIENTATION_ROTATED_90:
		param[1] = HX8394_FLIP_VERTICAL;
		break;
	case DISPLAY_ORIENTATION_ROTATED_180:
		param[1] = 0;
		break;
	case DISPLAY_ORIENTATION_ROTATED_270:
		param[1] = HX8394_FLIP_HORIZONTAL | HX8394_FLIP_VERTICAL;
		break;
	default:
		return -ENOTSUP;
	}
	return hx8394_mipi_tx(config->mipi_dsi, config->channel, param, 2);
}

static void hx8394_get_capabilities(const struct device *dev,
				     struct display_capabilities *capabilities)
{
	const struct hx8394_config *config = dev->config;

	memset(capabilities, 0, sizeof(struct display_capabilities));
	capabilities->x_resolution = config->panel_width;
	capabilities->y_resolution = config->panel_height;
	capabilities->supported_pixel_formats = config->pixel_format;
	capabilities->current_pixel_format = config->pixel_format;
	capabilities->current_orientation = DISPLAY_ORIENTATION_NORMAL;
}

static DEVICE_API(display, hx8394_api) = {
	.blanking_on = hx8394_blanking_on,
	.blanking_off = hx8394_blanking_off,
	.write = hx8394_write,
	.get_capabilities = hx8394_get_capabilities,
	.set_pixel_format = hx8394_set_pixel_format,
	.set_orientation = hx8394_set_orientation,
};

static int hx8394_init(const struct device *dev)
{
	const struct hx8394_config *config = dev->config;
	int ret;
	struct mipi_dsi_device mdev;
	uint8_t param[2];
	uint8_t setmipi[7] = {
		HX8394_SETMIPI,
		(HX8394_MIPI_LPTX_BTA_READ | HX8394_MIPI_LP_CD_DIS),
		HX8394_MIPI_TA_6TL,
		(HX8394_MIPI_DPHYCMD_LPRX_8NS |
		 HX8394_MIPI_DPHYCMD_LPRX_66mV |
		 HX8394_MIPI_DPHYCMD_LPTX_SRLIM),
		(HX8394_MIPI_DPHYCMD_LDO_1_55V |
		HX8394_MIPI_DPHYCMD_HSRX_7X |
		HX8394_MIPI_DPHYCMD_HSRX_100OHM |
		HX8394_MIPI_DPHYCMD_LPCD_1X),
		/* The remaining parameters here are not documented */
		0xB2U, 0xC0U};

	mdev.data_lanes = config->num_of_lanes;
	mdev.pixfmt = config->pixel_format;
	/* HX8394 runs in video mode */
	mdev.mode_flags = MIPI_DSI_MODE_VIDEO;

	ret = mipi_dsi_attach(config->mipi_dsi, config->channel, &mdev);
	if (ret < 0) {
		LOG_ERR("Could not attach to MIPI-DSI host");
		return ret;
	}

	if (gpio_is_ready_dt(&config->reset_gpio)) {
		/* Regulator API will have supplied power to the display
		 * driver. Per datasheet, we must wait 1ms for the RESX
		 * pin to be valid.
		 */
		k_sleep(K_MSEC(1));
		/* Initialize reset GPIO */
		ret = gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			return ret;
		}
		/* Pull reset GPIO low */
		gpio_pin_set_dt(&config->reset_gpio, 0);
		/* Datasheet says we must keep reset pin low at least 10us.
		 * hold it low for 1ms to be safe.
		 */
		k_sleep(K_MSEC(1));
		gpio_pin_set_dt(&config->reset_gpio, 1);
		/* Per datasheet, we must delay at least 50ms before first
		 * host command
		 */
		k_sleep(K_MSEC(50));
	}
	/* Enable extended commands */
	ret = hx8394_mipi_tx(config->mipi_dsi, config->channel,
			     enable_extension, sizeof(enable_extension));
	if (ret < 0) {
		return ret;
	}

	/* Set the number of lanes to DSISETUP0 parameter */
	setmipi[1] |= (config->num_of_lanes - 1);
	ret = hx8394_mipi_tx(config->mipi_dsi, config->channel,
			     setmipi, sizeof(setmipi));
	if (ret < 0) {
		return ret;
	}

	/* Set scan direction */
	ret = hx8394_mipi_tx(config->mipi_dsi, config->channel,
			     address_config, sizeof(address_config));
	if (ret < 0) {
		return ret;
	}

	/* Set voltage and current targets */
	ret = hx8394_mipi_tx(config->mipi_dsi, config->channel,
			     power_config, sizeof(power_config));
	if (ret < 0) {
		return ret;
	}

	/* Setup display line count and front/back porch size */
	ret = hx8394_mipi_tx(config->mipi_dsi, config->channel,
			     line_config, sizeof(line_config));
	if (ret < 0) {
		return ret;
	}

	/* Setup display cycle counts (in counts of TCON CLK) */
	ret = hx8394_mipi_tx(config->mipi_dsi, config->channel,
			     cycle_config, sizeof(cycle_config));
	if (ret < 0) {
		return ret;
	}

	/* Set group delay values */
	ret = hx8394_mipi_tx(config->mipi_dsi, config->channel,
			     gip0_config, sizeof(gip0_config));
	if (ret < 0) {
		return ret;
	}


	/* Set group clock selections */
	ret = hx8394_mipi_tx(config->mipi_dsi, config->channel,
			     gip1_config, sizeof(gip1_config));
	if (ret < 0) {
		return ret;
	}

	/* Set group clock selections for GS mode */
	ret = hx8394_mipi_tx(config->mipi_dsi, config->channel,
			     gip2_config, sizeof(gip2_config));
	if (ret < 0) {
		return ret;
	}

	/* Delay for a moment before setting VCOM. It is not clear
	 * from the datasheet why this is required, but without this
	 * delay the panel stops responding to additional commands
	 */
	k_msleep(1);
	/* Set VCOM voltage config */
	ret = hx8394_mipi_tx(config->mipi_dsi, config->channel,
			     vcom_config, sizeof(vcom_config));
	if (ret < 0) {
		return ret;
	}

	/* Set manufacturer supplied gamma values */
	ret = hx8394_mipi_tx(config->mipi_dsi, config->channel,
			     gamma_config, sizeof(gamma_config));
	if (ret < 0) {
		return ret;
	}

	/* This command is not documented in datasheet, but is included
	 * in the display initialization done by MCUXpresso SDK
	 */
	ret = hx8394_mipi_tx(config->mipi_dsi, config->channel,
			     hx8394_cmd1, sizeof(hx8394_cmd1));
	if (ret < 0) {
		return ret;
	}

	/* Set panel to BGR mode, and reverse colors */
	ret = hx8394_mipi_tx(config->mipi_dsi, config->channel,
			     panel_config, sizeof(panel_config));
	if (ret < 0) {
		return ret;
	}

	/* This command is not documented in datasheet, but is included
	 * in the display initialization done by MCUXpresso SDK
	 */
	ret = hx8394_mipi_tx(config->mipi_dsi, config->channel,
			     hx8394_cmd2, sizeof(hx8394_cmd2));
	if (ret < 0) {
		return ret;
	}

	/* Write values to manufacturer register banks */
	param[0] = HX8394_SETBANK;
	param[1] = 0x2;
	ret = hx8394_mipi_tx(config->mipi_dsi, config->channel,
			     param, 2);
	if (ret < 0) {
		return ret;
	}
	ret = hx8394_mipi_tx(config->mipi_dsi, config->channel,
			     hx8394_bank2, sizeof(hx8394_bank2));
	if (ret < 0) {
		return ret;
	}
	param[1] = 0x0;
	ret = hx8394_mipi_tx(config->mipi_dsi, config->channel,
			     param, 2);
	if (ret < 0) {
		return ret;
	}
	/* Select bank 1 */
	param[1] = 0x1;
	ret = hx8394_mipi_tx(config->mipi_dsi, config->channel,
			     param, 2);
	if (ret < 0) {
		return ret;
	}
	ret = hx8394_mipi_tx(config->mipi_dsi, config->channel,
			     hx8394_bank1, sizeof(hx8394_bank1));
	if (ret < 0) {
		return ret;
	}
	/* Select bank 0 */
	param[1] = 0x0;
	ret = hx8394_mipi_tx(config->mipi_dsi, config->channel,
			     param, 2);
	if (ret < 0) {
		return ret;
	}
	ret = hx8394_mipi_tx(config->mipi_dsi, config->channel,
			     hx8394_bank0, sizeof(hx8394_bank0));
	if (ret < 0) {
		return ret;
	}

	/* This command is not documented in datasheet, but is included
	 * in the display initialization done by MCUXpresso SDK
	 */
	ret = hx8394_mipi_tx(config->mipi_dsi, config->channel,
			     hx8394_cmd3, sizeof(hx8394_cmd3));
	if (ret < 0) {
		return ret;
	}

	ret = hx8394_mipi_tx(config->mipi_dsi, config->channel,
			     tear_config, sizeof(tear_config));
	if (ret < 0) {
		return ret;
	}

	param[0] = MIPI_DCS_EXIT_SLEEP_MODE;

	ret = hx8394_mipi_tx(config->mipi_dsi, config->channel,
			     param, 1);
	if (ret < 0) {
		return ret;
	}
	/* We must delay 120ms after exiting sleep mode per datasheet */
	k_sleep(K_MSEC(120));

	param[0] = MIPI_DCS_SET_DISPLAY_ON;
	ret = hx8394_mipi_tx(config->mipi_dsi, config->channel,
			     param, 1);

	if (config->bl_gpio.port != NULL) {
		ret = gpio_pin_configure_dt(&config->bl_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Could not configure bl GPIO (%d)", ret);
			return ret;
		}
	}

	return ret;
}

#define HX8394_PANEL(id)							\
	static const struct hx8394_config hx8394_config_##id = {		\
		.mipi_dsi = DEVICE_DT_GET(DT_INST_BUS(id)),			\
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(id, reset_gpios, {0}),	\
		.bl_gpio = GPIO_DT_SPEC_INST_GET_OR(id, bl_gpios, {0}),		\
		.num_of_lanes = DT_INST_PROP_BY_IDX(id, data_lanes, 0),		\
		.pixel_format = DT_INST_PROP(id, pixel_format),			\
		.panel_width = DT_INST_PROP(id, width),				\
		.panel_height = DT_INST_PROP(id, height),			\
		.channel = DT_INST_REG_ADDR(id),				\
	};									\
	DEVICE_DT_INST_DEFINE(id,						\
			    &hx8394_init,					\
			    NULL,						\
			    NULL,						\
			    &hx8394_config_##id,				\
			    POST_KERNEL,					\
			    CONFIG_APPLICATION_INIT_PRIORITY,			\
			    &hx8394_api);

DT_INST_FOREACH_STATUS_OKAY(HX8394_PANEL)
