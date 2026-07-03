/*
 * Copyright (c) 2026 Qingsong Gou <gouqs@hotmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT jdi_ls013b7dh03

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/jdi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(display_ls013b7dh03, CONFIG_DISPLAY_LOG_LEVEL);

/* LS013B7DH03 Display Characteristics */
#define LS013B7DH03_WIDTH    DT_INST_PROP(0, width)
#define LS013B7DH03_HEIGHT   DT_INST_PROP(0, height)
#define LS013B7DH03_PIXELS_PER_BYTE 8U

/* Display buffer size: line number (1 byte) + data + dummy byte */
#define LS013B7DH03_BYTES_PER_LINE ((LS013B7DH03_WIDTH / LS013B7DH03_PIXELS_PER_BYTE) + 2)
#define LS013B7DH03_BUF_SIZE (LS013B7DH03_HEIGHT * LS013B7DH03_BYTES_PER_LINE)

/* JDI LS013B7DH03 Commands */
#define LS0XX_CMD_DISPLAY_ON      0x01
#define LS0XX_CMD_ALL_PIXEL_ON    0x02
#define LS0XX_CMD_ALL_PIXEL_OFF   0x03
#define LS0XX_CMD_INVERSION_OFF   0x04
#define LS0XX_CMD_INVERSION_ON    0x05
#define LS0XX_CMD_SET_LINE        0x06
#define LS0XX_CMD_SET_COLUMN      0x0A
#define LS0XX_CMD_WRITE_PIXEL     0x0C
#define LS0XX_CMD_READ_PIXEL      0x0D
#define LS0XX_CMD_VCOM_TOGGLE     0x12

#ifndef RC10K_FREQ
#define RC10K_FREQ 9000
#endif
#define LPTIM_INTCLOCKSOURCE_LPCLOCK            0x00U

/* VCOM Toggle Interval (in milliseconds) */
#define LS0XX_VCOM_TOGGLE_INTERVAL 60000  /* 60 seconds */

struct ls013b7dh03_data {
};

struct ls013b7dh03_config {
	const struct device *jdi_bus;
	struct jdi_config jdi_cfg;
	struct gpio_dt_spec vlcd_gpio;
	struct gpio_dt_spec vddp_gpio;
	uint16_t power_seq_delay_ms;
	bool serial_vcom_inversion;
};

/* Static helper functions */
static inline uint8_t reverse_byte(uint8_t b)
{
	uint8_t r = 0;
	for (int i = 0; i < 8; i++) {
		r = (r << 1) | (b & 1);
		b >>= 1;
	}
	return r;
}

static int ls013b7dh03_write_data(const struct device *dev, uint16_t x, uint16_t y,
				  uint16_t width, uint16_t height,
				  const uint8_t *data, size_t len, const struct jdi_config *jdi_cfg)
{
	const struct ls013b7dh03_config *config = dev->config;

	return jdi_write_data(config->jdi_bus, x, y, width, height, data, len, jdi_cfg);
}

static int ls013b7dh03_blanking_on(const struct device *dev)
{
	const struct ls013b7dh03_config *config = dev->config;
	struct ls013b7dh03_data *data = dev->data;

	LOG_DBG("Turning display off");

	return 0;
}

static int ls013b7dh03_blanking_off(const struct device *dev)
{
	const struct ls013b7dh03_config *config = dev->config;
	struct ls013b7dh03_data *data = dev->data;

	LOG_DBG("Turning display on");

	if (config->vlcd_gpio.port) {
		if (device_is_ready(config->vlcd_gpio.port)) {
			gpio_pin_set_dt(&config->vlcd_gpio, 1);
			k_msleep(config->power_seq_delay_ms);
		}
	}

	if (config->vddp_gpio.port) {
		if (device_is_ready(config->vddp_gpio.port)) {
			gpio_pin_set_dt(&config->vddp_gpio, 1);
			k_msleep(config->power_seq_delay_ms);
		}
	}
__IO uint32_t *rtc_reg;
uint32_t pinout_sel = 0;
uint32_t pinout_sel_mask;
uint32_t pbr_sel;

//LPTIM PA24
rtc_reg = &hwp_rtc->PBR0R;

pinout_sel = HPSYS_AON_CR1_PINOUT_SEL0_LPTIM2_OUT;
pinout_sel_mask = HPSYS_AON_CR1_PINOUT_SEL0_Msk;
pbr_sel = RTC_PBR0R_SEL_PINOUT_SEL0;

MODIFY_REG(hwp_hpsys_aon->CR1, pinout_sel_mask, pinout_sel);
/* select function */
MODIFY_REG(*rtc_reg, RTC_PBR0R_SEL_Msk, pbr_sel);
/* enable output */
MODIFY_REG(*rtc_reg, RTC_PBR0R_OE_Msk, RTC_PBR0R_OE_Msk);

// LPTIM PA25
rtc_reg = &hwp_rtc->PBR1R;

pinout_sel = HPSYS_AON_CR1_PINOUT_SEL1_LPTIM2_INV_OUT;
pinout_sel_mask = HPSYS_AON_CR1_PINOUT_SEL1_Msk;
pbr_sel = RTC_PBR0R_SEL_PINOUT_SEL1;
MODIFY_REG(hwp_hpsys_aon->CR1, pinout_sel_mask, pinout_sel);
/* select function */
MODIFY_REG(*rtc_reg, RTC_PBR0R_SEL_Msk, pbr_sel);
/* enable output */
MODIFY_REG(*rtc_reg, RTC_PBR0R_OE_Msk, RTC_PBR0R_OE_Msk);

	LPTIM_TypeDef *lptim = hwp_lptim2;

	lptim->CFGR |= LPTIM_INTCLOCKSOURCE_LPCLOCK;
	lptim->ARR = RC10K_FREQ / 60U;
	lptim->CMP = lptim->ARR / 2;
	lptim->CR |= LPTIM_CR_ENABLE;
	lptim->CR |= LPTIM_CR_CNTSTRT;

	return 0;
}

static void ls013b7dh03_get_capabilities(const struct device *dev,
					   struct display_capabilities *caps)
{
	memset(caps, 0, sizeof(*caps));

	caps->x_resolution = LS013B7DH03_WIDTH;
	caps->y_resolution = LS013B7DH03_HEIGHT;
//	caps->supported_formats = PIXEL_FORMAT_MONO01;
	caps->current_orientation = DISPLAY_ORIENTATION_NORMAL;
	caps->screen_info = SCREEN_INFO_MONO_VTILED;
}

static int ls013b7dh03_write(const struct device *dev, const uint16_t x,
			       const uint16_t y, const struct display_buffer_descriptor *desc,
			       const void *buf)
{
	const struct ls013b7dh03_config *config = dev->config;
	const uint8_t *src = buf;

	if (x >= LS013B7DH03_WIDTH || y >= LS013B7DH03_HEIGHT) {
		return -EINVAL;
	}

	ls013b7dh03_write_data(dev, x, y, desc->width, desc->height, src, desc->buf_size, &config->jdi_cfg);

	return 0;
}

static int ls013b7dh03_read(const struct device *dev, const uint16_t x,
			      const uint16_t y, const struct display_buffer_descriptor *desc,
			      void *buf)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(x);
	ARG_UNUSED(y);
	ARG_UNUSED(desc);
	ARG_UNUSED(buf);
	return -ENOTSUP;
}

static int ls013b7dh03_set_pixel_format(const struct device *dev,
					 enum display_pixel_format format)
{
	if (format != PIXEL_FORMAT_MONO01) {
		LOG_ERR("Unsupported pixel format: %d", format);
		return -ENOTSUP;
	}

	return 0;
}

static const struct display_driver_api ls013b7dh03_display_api = {
	.blanking_on = ls013b7dh03_blanking_on,
	.blanking_off = ls013b7dh03_blanking_off,
	.write = ls013b7dh03_write,
	.read = ls013b7dh03_read,
	.get_capabilities = ls013b7dh03_get_capabilities,
	.set_pixel_format = ls013b7dh03_set_pixel_format,
};

static int ls013b7dh03_init(const struct device *dev)
{
	const struct ls013b7dh03_config *config = dev->config;
	struct ls013b7dh03_data *data = dev->data;
	int ret;

	LOG_DBG("Initializing LS013B7DH03 display");

	/* Check if JDI bus is ready */
	if (!device_is_ready(config->jdi_bus)) {
		LOG_ERR("JDI bus device not ready");
		return -ENODEV;
	}

	/* Configure power GPIOs (inactive state initially) */
	if (config->vlcd_gpio.port) {
		if (!gpio_is_ready_dt(&config->vlcd_gpio)) {
			LOG_ERR("VLCD GPIO device not ready");
			return -ENODEV;
		}
	}
	if (config->vddp_gpio.port) {
		if (!gpio_is_ready_dt(&config->vddp_gpio)) {
			LOG_ERR("VDDP GPIO device not ready");
			return -ENODEV;
		}
	}

	/* Configure JDI bus */
	ret = jdi_config(config->jdi_bus, &config->jdi_cfg);
	if (ret < 0) {
		LOG_ERR("Failed to configure JDI bus: %d", ret);
		return ret;
	}

	LOG_DBG("LS013B7DH03 display initialized successfully");

	return 0;
}

#define LS013B7DH03_DEFINE(n)                                                                      \
	static struct ls013b7dh03_data ls013b7dh03_data_##n;                                       \
	static const struct ls013b7dh03_config ls013b7dh03_config_##n = {                          \
		.jdi_bus = DEVICE_DT_GET(DT_INST_BUS(n)),                                          \
		.serial_vcom_inversion = DT_INST_PROP_OR(n, serial_vcom_inversion, 0),             \
		.jdi_cfg = {                                                                       \
			.bank_col_head = DT_INST_PROP_OR(n, bank_col_head, 0),                     \
			.valid_columns = DT_INST_PROP_OR(n, valid_columns, 0),                     \
			.bank_col_tail = DT_INST_PROP_OR(n, bank_col_tail, 0),                     \
			.bank_row_head = DT_INST_PROP_OR(n, bank_row_head, 0),                     \
			.valid_rows = DT_INST_PROP_OR(n, valid_rows, 0),                           \
			.bank_row_tail = DT_INST_PROP_OR(n, bank_row_tail, 0),                     \
			.enb_start_col = DT_INST_PROP_OR(n, enb_start_col, 0),                     \
			.enb_end_col = DT_INST_PROP_OR(n, enb_end_col, 0),                         \
			.enb_pol_invert = DT_INST_PROP_OR(n, enb_pol_invert, 0),                   \
			.hck_pol_invert = DT_INST_PROP_OR(n, hck_pol_invert, 0),                   \
			.hst_pol_invert = DT_INST_PROP_OR(n, hst_pol_invert, 0),                   \
			.vck_pol_invert = DT_INST_PROP_OR(n, vck_pol_invert, 0),                   \
			.vst_pol_invert = DT_INST_PROP_OR(n, vst_pol_invert, 0),                   \
		},                                                                                 \
		.vlcd_gpio = GPIO_DT_SPEC_INST_GET_OR(n, vlcd_gpios, {0}),                         \
		.vddp_gpio = GPIO_DT_SPEC_INST_GET_OR(n, vddp_gpios, {0}),                         \
		.power_seq_delay_ms = DT_INST_PROP_OR(n, power_seq_delay_ms, 11),                  \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, ls013b7dh03_init, NULL,                                           \
			      &ls013b7dh03_data_##n, &ls013b7dh03_config_##n,                      \
			      POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY,                           \
			      &ls013b7dh03_display_api);
DT_INST_FOREACH_STATUS_OKAY(LS013B7DH03_DEFINE)
