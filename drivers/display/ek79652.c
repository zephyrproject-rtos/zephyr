/*
 * Copyright (c) 2021 Linumiz
 *
 * Based on gd7965.c which is:
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT gooddisplay_ek79652

#include <string.h>
#include <drivers/display.h>
#include <drivers/gpio.h>
#include <drivers/spi.h>
#include <sys/byteorder.h>

#include "ek79652_regs.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(ek79652, CONFIG_DISPLAY_LOG_LEVEL);

/**
 * EK79652 compatible EPD controller driver.
 *
 */

#define EK79652_PIXELS_PER_BYTE		8U

struct ek79652_dt_array {
	uint8_t *data;
	uint8_t len;
};

struct ek79652_config {
	struct spi_dt_spec bus;
	struct gpio_dt_spec reset;
	struct gpio_dt_spec dc;
	struct gpio_dt_spec busy;
	struct ek79652_dt_array softstart;
	struct ek79652_dt_array pwr;
	struct ek79652_dt_array pwropt;
	struct ek79652_dt_array lut_vcom_dc;
	struct ek79652_dt_array lut_ww;
	struct ek79652_dt_array lut_wb;
	struct ek79652_dt_array lut_bw;
	struct ek79652_dt_array lut_bb;
	uint16_t height;
	uint16_t width;
	uint8_t cdi;
};

static bool blanking_on = true;

static int ek79652_write_cmd(const struct device *dev,
				   uint8_t cmd, uint8_t *data, size_t len)
{
	const struct ek79652_config *config = dev->config;
	struct spi_buf buf = {.buf = &cmd, .len = sizeof(cmd)};
	struct spi_buf_set buf_set = {.buffers = &buf, .count = 1};
	int err;

	err = gpio_pin_set_dt(&config->dc, 1);
	if (err < 0) {
		return err;
	}

	err = spi_write_dt(&config->bus, &buf_set);
	if (err < 0) {
		return err;
	}

	if (data != NULL) {
		for (int i = 0 ; i < len ; i++) {
			buf.buf = &data[i];
			buf.len = 1;
			err = gpio_pin_set_dt(&config->dc, 0);
			if (err < 0) {
				return err;
			}

			err = spi_write_dt(&config->bus, &buf_set);
			if (err < 0) {
				return err;
			}
		}
	}

	return 0;
}

static void ek79652_busy_wait(const struct device *dev)
{
	const struct ek79652_config *config = dev->config;
	int pin = gpio_pin_get_dt(&config->busy);

	while (pin > 0) {
		__ASSERT(pin >= 0, "Failed to get pin level");
		LOG_DBG("wait %u", pin);
		k_sleep(K_MSEC(EK79652_BUSY_DELAY));
		pin = gpio_pin_get_dt(&config->busy);
	}
}

static int ek79652_update_partial_display(const struct device *dev,
		uint8_t *data, size_t len)
{
	int err;

	LOG_DBG("Trigger partial update sequence");

	err = ek79652_write_cmd(dev, EK79652_CMD_PDRF, data, len);
	if (err < 0) {
		return err;
	}

	ek79652_busy_wait(dev);

	return 0;
}

static int ek79652_update_display(const struct device *dev)
{
	int err;

	LOG_DBG("Trigger full update sequence");

	err = ek79652_write_cmd(dev, EK79652_CMD_DRF, NULL, 0);
	if (err < 0) {
		return err;
	}

	k_sleep(K_MSEC(EK79652_BUSY_DELAY));

	return 0;
}

static int ek79652_blanking_off(const struct device *dev)
{
	int err;

	if (blanking_on) {
		ek79652_busy_wait(dev);
		err = ek79652_update_display(dev);
		if (err < 0) {
			return err;
		}
	}

	blanking_on = false;

	return 0;
}

static int ek79652_blanking_on(const struct device *dev)
{
	blanking_on = true;
	return 0;
}

static int ek79652_write(const struct device *dev, const uint16_t x,
		const uint16_t y,
		const struct display_buffer_descriptor *desc,
		const void *buf)
{
	const struct ek79652_config *config = dev->config;
	uint16_t xs = (x % 8) ? x : (x - (x % 8));
	uint16_t w = (desc->width % 8) ? desc->width : (desc->width - (desc->width % 8));
	uint16_t h = desc->height;
	size_t buf_len, buf_ptd_len;
	int err;

	LOG_DBG("x %u, y %u, height %u, width %u, pitch %u",
		x, y, desc->height, desc->width, desc->pitch);

	buf_len = MIN(desc->buf_size,
		      desc->height * desc->width / EK79652_PIXELS_PER_BYTE);
	__ASSERT(desc->width <= desc->pitch, "Pitch is smaller then width");
	__ASSERT(buf != NULL, "Buffer is not available");
	__ASSERT(buf_len != 0U, "Buffer of length zero");
	__ASSERT(!(desc->width % EK79652_PIXELS_PER_BYTE),
		"Buffer width not multiple of %d", EK79652_PIXELS_PER_BYTE);

	if ((h > config->height) || (w > config->width)) {
		LOG_ERR("Position out of bounds");
		return -EINVAL;
	}

	buf_ptd_len = buf_len + EK79652_PDT_REG_LENGTH;
	uint8_t ptd[buf_ptd_len];

	/* Setup Partial Window and enable Partial Mode */
	sys_put_be16(xs, &ptd[EK79652_PDT_X_IDX]);
	sys_put_be16(y, &ptd[EK79652_PDT_Y_IDX]);
	sys_put_be16(w, &ptd[EK79652_PDT_W_IDX]);
	sys_put_be16(h, &ptd[EK79652_PDT_H_IDX]);

	memcpy((ptd + EK79652_PDT_REG_LENGTH), (uint8_t *)buf, buf_len);

	err = ek79652_write_cmd(dev, EK79652_CMD_PTIN, NULL, 0);
	if (err < 0) {
		return err;
	}

	err = ek79652_write_cmd(dev, EK79652_CMD_PDTM2, ptd, buf_ptd_len);
	if (err < 0) {
		return err;
	}

	if (blanking_on == false) {
		err = ek79652_update_partial_display(dev, ptd, EK79652_PDT_REG_LENGTH);
		if (err < 0) {
			return err;
		}
	}
	return 0;
}

static int ek79652_read(const struct device *dev, const uint16_t x, const uint16_t y,
		       const struct display_buffer_descriptor *desc, void *buf)
{
	LOG_ERR("not supported");
	return -ENOTSUP;
}

static void *ek79652_get_framebuffer(const struct device *dev)
{
	LOG_ERR("not supported");
	return NULL;
}

static int ek79652_set_brightness(const struct device *dev,
				 const uint8_t brightness)
{
	LOG_WRN("not supported");
	return -ENOTSUP;
}

static int ek79652_set_contrast(const struct device *dev, uint8_t contrast)
{
	LOG_WRN("not supported");
	return -ENOTSUP;
}

static void ek79652_get_capabilities(const struct device *dev,
				    struct display_capabilities *caps)
{
	const struct ek79652_config *config = dev->config;

	memset(caps, 0, sizeof(struct display_capabilities));
	caps->x_resolution = config->width;
	caps->y_resolution = config->height;
	caps->supported_pixel_formats = PIXEL_FORMAT_MONO10;
	caps->current_pixel_format = PIXEL_FORMAT_MONO10;
	caps->screen_info = SCREEN_INFO_MONO_MSB_FIRST | SCREEN_INFO_EPD;
	caps->current_orientation = DISPLAY_ORIENTATION_NORMAL;
}

static int ek79652_set_orientation(const struct device *dev,
				  const enum display_orientation
				  orientation)
{
	uint8_t tmp;
	int err;

	if (orientation == DISPLAY_ORIENTATION_NORMAL) {
		tmp = EK79652_PSR_LUT_EN | EK79652_PSR_UD | EK79652_PSR_SHL |
			EK79652_PSR_BW |
			EK79652_PSR_SHD | EK79652_PSR_RST;
	} else if (orientation == DISPLAY_ORIENTATION_ROTATED_180) {
		tmp = EK79652_PSR_LUT_EN | EK79652_PSR_BW |
			EK79652_PSR_SHD | EK79652_PSR_RST;
	} else {
		LOG_ERR("orientation not supported\n");
	}

	err = ek79652_write_cmd(dev, EK79652_CMD_PSR, &tmp, 1);
	if (err < 0) {
		return err;
	}

	return 0;
}

static int ek79652_set_pixel_format(const struct device *dev,
				   const enum display_pixel_format pf)
{
	if (pf == PIXEL_FORMAT_MONO10) {
		return 0;
	}

	LOG_ERR("not supported");
	return -ENOTSUP;
}

static int ek79652_clear_display(const struct device *dev,
					 uint8_t pattern, bool update)
{
	const struct ek79652_config *config = dev->config;
	uint32_t buf_size = (config->width / EK79652_PIXELS_PER_BYTE);
	int err;

	struct display_buffer_descriptor desc = {
		.buf_size = buf_size,
		.width = config->width,
		.height = 1,
		.pitch = config->width,
	};
	uint8_t line[buf_size];

	memset(line, pattern, buf_size);

	for (int i = 0; i < config->height; i++) {
		ek79652_write(dev, 0, i, &desc, line);
	}

	if (update == true) {
		err = ek79652_update_display(dev);
		if (err < 0) {
			return err;
		}
	}

	return 0;
}

static int ek79652_controller_init(const struct device *dev)
{
	const struct ek79652_config *config = dev->config;
	uint8_t tmp[EK79652_PWROPT_LENGTH];
	int err;

	err = gpio_pin_set_dt(&config->reset, 0);
	if (err < 0) {
		return err;
	}
	k_sleep(K_MSEC(EK79652_RESET_DELAY));

	err = gpio_pin_set_dt(&config->reset, 1);
	if (err < 0) {
		return err;
	}
	k_sleep(K_MSEC(EK79652_RESET_DELAY));

	err = gpio_pin_set_dt(&config->reset, 0);
	if (err < 0) {
		return err;
	}
	k_sleep(K_MSEC(EK79652_RESET_DELAY));
	ek79652_busy_wait(dev);

	LOG_DBG("Initialize EK79652 controller");

	err = ek79652_write_cmd(dev, EK79652_CMD_PWR, config->pwr.data,
				config->pwr.len);
	if (err < 0) {
		return err;
	}

	err = ek79652_write_cmd(dev, EK79652_CMD_BTST, config->softstart.data,
					config->softstart.len);
	if (err < 0) {
		return err;
	}

	/* Power optimization */
	if (config->pwropt.len) {
		tmp[0] = config->pwropt.data[0];
		tmp[1] = config->pwropt.data[1];
		err = ek79652_write_cmd(dev, EK79652_CMD_PWROPT, tmp,
			EK79652_PWROPT_LENGTH);
		if (err < 0) {
			return err;
		}

		tmp[0] = config->pwropt.data[2];
		tmp[1] = config->pwropt.data[3];
		err = ek79652_write_cmd(dev, EK79652_CMD_PWROPT, tmp,
			EK79652_PWROPT_LENGTH);
		if (err < 0) {
			return err;
		}

		tmp[0] = config->pwropt.data[4];
		tmp[1] = config->pwropt.data[5];
		err = ek79652_write_cmd(dev, EK79652_CMD_PWROPT, tmp,
			EK79652_PWROPT_LENGTH);
		if (err < 0) {
			return err;
		}

		tmp[0] = config->pwropt.data[6];
		tmp[1] = config->pwropt.data[7];
		err = ek79652_write_cmd(dev, EK79652_CMD_PWROPT, tmp,
			EK79652_PWROPT_LENGTH);
		if (err < 0) {
			return err;
		}

		tmp[0] = config->pwropt.data[8];
		tmp[1] = config->pwropt.data[9];
		err = ek79652_write_cmd(dev, EK79652_CMD_PWROPT, tmp,
			EK79652_PWROPT_LENGTH);
		if (err < 0) {
			return err;
		}

		tmp[0] = config->pwropt.data[10];
		tmp[1] = config->pwropt.data[11];
		err = ek79652_write_cmd(dev, EK79652_CMD_PWROPT, tmp,
			EK79652_PWROPT_LENGTH);
		if (err < 0) {
			return err;
		}
	}

	/* Reset DFV_EN */
	tmp[0] = EK79652_PDRF_VAL;
	err = ek79652_write_cmd(dev, EK79652_CMD_PDRF, tmp, 1);
	if (err < 0) {
		return err;
	}

	/* Turn on: booster, controller, regulators, and sensor. */
	err = ek79652_write_cmd(dev, EK79652_CMD_PON, NULL, 0);
	if (err < 0) {
		return err;
	}
	ek79652_busy_wait(dev);

	/* Pannel settings : BWOTP-1F
	 * KWR-AF BWROTP-0F KW-BF
	 */
	tmp[0] = EK79652_PSR_BW |
		EK79652_PSR_UD | EK79652_PSR_SHL |
		EK79652_PSR_SHD | EK79652_PSR_RST;
	err = ek79652_write_cmd(dev, EK79652_CMD_PSR, tmp, 1);
	if (err < 0) {
		return err;
	}

	tmp[0] = config->cdi;
	err = ek79652_write_cmd(dev, EK79652_CMD_CDI, tmp, 1);
	if (err < 0) {
		return err;
	}

	err = ek79652_clear_display(dev, 0xFF, false);
	if (err < 0) {
		return err;
	}

	return 0;
}

static int ek79652_init(const struct device *dev)
{
	const struct ek79652_config *config = dev->config;
	int err;

	if (!spi_is_ready(&config->bus)) {
		LOG_ERR("SPI bus %s not ready", config->bus.bus->name);
		return -ENODEV;
	}

	if (!device_is_ready(config->reset.port)) {
		LOG_ERR("GPIO port for EK79652 reset is not ready");
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&config->reset, GPIO_OUTPUT_INACTIVE);
	if (err < 0) {
		LOG_ERR("Failed to configure reset GPIO");
		return err;
	}

	if (!device_is_ready(config->dc.port)) {
		LOG_ERR("GPIO port for EK79652 DC signal is not ready");
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&config->dc, GPIO_OUTPUT_INACTIVE);
	if (err < 0) {
		LOG_ERR("Failed to configure DC GPIO");
		return err;
	}

	if (!device_is_ready(config->busy.port)) {
		LOG_ERR("GPIO port for EK79652 busy signal is not ready");
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&config->busy, GPIO_INPUT);
	if (err < 0) {
		LOG_ERR("Failed to configure busy GPIO");
		return err;
	}

	return ek79652_controller_init(dev);
}

static struct display_driver_api ek79652_driver_api = {
	.blanking_on = ek79652_blanking_on,
	.blanking_off = ek79652_blanking_off,
	.write = ek79652_write,
	.read = ek79652_read,
	.get_framebuffer = ek79652_get_framebuffer,
	.set_brightness = ek79652_set_brightness,
	.set_contrast = ek79652_set_contrast,
	.get_capabilities = ek79652_get_capabilities,
	.set_pixel_format = ek79652_set_pixel_format,
	.set_orientation = ek79652_set_orientation,
};

#define LUT_VCOM_DC_DEFINE(n)						\
	static uint8_t lut_vcom_dc_##n[] = DT_INST_PROP(n, lut_vcom_dc);
#define LUT_WW_DEFINE(n)						\
	static uint8_t lut_ww_##n[] = DT_INST_PROP(n, lut_ww);
#define LUT_BW_DEFINE(n)						\
	static uint8_t lut_bw_##n[] = DT_INST_PROP(n, lut_bw);
#define LUT_WB_DEFINE(n)						\
	static uint8_t lut_wb_##n[] = DT_INST_PROP(n, lut_wb);
#define LUT_BB_DEFINE(n)						\
	static uint8_t lut_bb_##n[] = DT_INST_PROP(n, lut_bb);
#define PWROPT_DEFINE(n)						\
	static uint8_t pwropt_##n[] = DT_INST_PROP(n, pwropt);

#define LUT_VCOM_DC_ASSIGN(n)						\
		.lut_vcom_dc = {					\
			.data = lut_vcom_dc_##n,			\
			.len = sizeof(lut_vcom_dc_##n),			\
		},

#define LUT_WW_ASSIGN(n)						\
		.lut_ww = {						\
			.data = lut_ww_##n,				\
			.len = sizeof(lut_ww_##n),			\
		},

#define LUT_BW_ASSIGN(n)						\
		.lut_bw = {						\
			.data = lut_bw_##n,				\
			.len = sizeof(lut_bw_##n),			\
		},

#define LUT_WB_ASSIGN(n)						\
		.lut_wb = {						\
			.data = lut_wb_##n,				\
			.len = sizeof(lut_wb_##n),			\
		},

#define LUT_BB_ASSIGN(n)						\
		.lut_bb = {						\
			.data = lut_bb_##n,				\
			.len = sizeof(lut_bb_##n),			\
		},

#define PWROPT_ASSIGN(n)						\
		.pwropt = {						\
			.data = pwropt_##n,				\
			.len = sizeof(pwropt_##n),			\
		},

#define CMD_ARRAY_DEFINE(n)						\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, lut_vcom_dc),		\
		    (LUT_VCOM_DC_DEFINE(n)),				\
		    ())							\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, lut_ww),			\
		    (LUT_WW_DEFINE(n)),					\
		    ())							\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, lut_bw),			\
		    (LUT_BW_DEFINE(n)),					\
		    ())							\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, lut_wb),			\
		    (LUT_WB_DEFINE(n)),					\
		    ())							\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, lut_bb),			\
		    (LUT_BB_DEFINE(n)),					\
		    ())							\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, pwropt),			\
		    (PWROPT_DEFINE(n)),					\
		    ())

#define CMD_ARRAY_COND(n)						\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, lut_vcom_dc),		\
		    (LUT_VCOM_DC_ASSIGN(n)),				\
		    ())							\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, lut_ww),			\
		    (LUT_WW_ASSIGN(n)),					\
		    ())							\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, lut_bw),			\
		    (LUT_BW_ASSIGN(n)),					\
		    ())							\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, lut_wb),			\
		    (LUT_WB_ASSIGN(n)),					\
		    ())							\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, lut_bb),			\
		    (LUT_BB_ASSIGN(n)),					\
		    ())							\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, pwropt),			\
		    (PWROPT_ASSIGN(n)),					\
		    ())

#define EK79652_DEFINE(n)						\
	static uint8_t softstart_##n[] = DT_INST_PROP(n, softstart);	\
	static uint8_t pwr_##n[] = DT_INST_PROP(n, pwr);		\
									\
	CMD_ARRAY_DEFINE(n)						\
									\
	static const struct ek79652_config ek79652_cfg_##n = {		\
		.bus = SPI_DT_SPEC_INST_GET(n,				\
			SPI_OP_MODE_MASTER | SPI_WORD_SET(8), 0),	\
		.reset = GPIO_DT_SPEC_INST_GET(n, reset_gpios),		\
		.dc = GPIO_DT_SPEC_INST_GET(n, dc_gpios),		\
		.busy = GPIO_DT_SPEC_INST_GET(n, busy_gpios),		\
		.height = DT_INST_PROP(n, height),			\
		.width = DT_INST_PROP(n, width),			\
		.cdi = DT_INST_PROP(n, cdi),				\
		.softstart = {						\
			.data = softstart_##n,				\
			.len = sizeof(softstart_##n),			\
		},							\
		.pwr = {						\
			.data = pwr_##n,				\
			.len = sizeof(pwr_##n),				\
		},							\
		CMD_ARRAY_COND(n)					\
	};								\
									\
	DEVICE_DT_INST_DEFINE(n, ek79652_init, NULL, NULL,		\
			      &ek79652_cfg_##n,				\
			      POST_KERNEL,				\
			      CONFIG_DISPLAY_INIT_PRIORITY,		\
			      &ek79652_driver_api);			\

DT_INST_FOREACH_STATUS_OKAY(EK79652_DEFINE)
