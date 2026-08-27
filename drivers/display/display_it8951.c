/*
 * Copyright (c) 2026 The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it8951

#include <stdint.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(it8951, CONFIG_DISPLAY_LOG_LEVEL);

#define IT8951_PREAMBLE_WRITE_CMD  0x6000U
#define IT8951_PREAMBLE_WRITE_DATA 0x0000U
#define IT8951_PREAMBLE_READ_DATA  0x1000U

#define IT8951_CMD_SYS_RUN      0x0001U
#define IT8951_CMD_SLEEP        0x0003U
#define IT8951_CMD_REG_READ     0x0010U
#define IT8951_CMD_REG_WRITE    0x0011U
#define IT8951_CMD_LOAD_AREA    0x0021U
#define IT8951_CMD_LOAD_END     0x0022U
#define IT8951_CMD_DISPLAY_AREA 0x0034U
/* User-defined: panel miscellaneous (temperature / calibration); see IT8951 programming guide. */
#define IT8951_CMD_PANEL_MISC   0x0040U
#define IT8951_CMD_VCOM         0x0039U

#define IT8951_PANEL_TEMP_SET_SEL 0x0001U
#define IT8951_CMD_GET_INFO       0x0302U

#define IT8951_REG_BASE      0x1000U
#define IT8951_REG_I80CPCR   0x0004U
#define IT8951_REG_LISAR     0x0208U
#define IT8951_REG_LUTAFSR   (IT8951_REG_BASE + 0x0224U)
#define IT8951_PACKED_ENABLE 0x0001U

#define IT8951_ENDIAN_LITTLE 0U
#define IT8951_ENDIAN_BIG    1U

/* IT8951 LD_IMG_AREA pixel-format field: 4 bpp packed (matches PIXEL_FORMAT_L_4). */
#define IT8951_LOAD_IMAGE_PIXFMT_4BPP 2U

struct it8951_dev_info {
	uint16_t panel_width;
	uint16_t panel_height;
	uint16_t img_buf_addr_l;
	uint16_t img_buf_addr_h;
	uint16_t fw_version[8];
	uint16_t lut_version[8];
};

struct it8951_config {
	struct spi_dt_spec spi;
	struct gpio_dt_spec busy_gpio;
	struct gpio_dt_spec reset_gpio;
	struct gpio_dt_spec enable_gpio;
	struct gpio_dt_spec ite_enable_gpio;
	uint16_t width;
	uint16_t height;
	uint16_t vcom_mv;
	uint16_t initial_refresh_waveform;
	uint16_t refresh_waveform;
	bool h_mirror;
	bool has_panel_temp;
	int16_t panel_temp_c;
	uint16_t reset_delay_ms;
	uint16_t reset_wait_ms;
	uint32_t ready_timeout_ms;
	uint32_t refresh_timeout_ms;
	uint8_t *row_spi_buf;
	size_t row_spi_buf_cap;
};

struct it8951_data {
	uint32_t img_buf_addr;
	bool blanking_on;
	bool dirty;
	uint16_t dirty_x_min;
	uint16_t dirty_y_min;
	uint16_t dirty_x_max;
	uint16_t dirty_y_max;
};

static uint16_t it8951_mirror_x(const struct it8951_config *cfg, uint16_t x, uint16_t w)
{
	if (!cfg->h_mirror) {
		return x;
	}

	return (cfg->width - 1U) - x - w + 1U;
}

static int it8951_wait_ready(const struct device *dev, uint32_t timeout_ms)
{
	const struct it8951_config *config = dev->config;
	int64_t end = k_uptime_get() + timeout_ms;
	int pin;

	do {
		pin = gpio_pin_get_dt(&config->busy_gpio);
		if (pin < 0) {
			LOG_ERR("Failed to read busy GPIO: %d", pin);
			return pin;
		}

		if (pin == 0) {
			return 0;
		}

		k_msleep(1);
	} while (k_uptime_get() < end);

	LOG_ERR("Timeout waiting for HRDY");
	return -ETIMEDOUT;
}

static int it8951_spi_write_with_config(const struct device *dev, const uint8_t *tx_buf, size_t len,
					const struct spi_config *spi_cfg)
{
	const struct it8951_config *config = dev->config;
	const struct spi_buf tx = {
		.buf = (void *)tx_buf,
		.len = len,
	};
	const struct spi_buf_set tx_set = {
		.buffers = &tx,
		.count = 1,
	};

	return spi_write(config->spi.bus, spi_cfg, &tx_set);
}

static int it8951_write_word_with_config(const struct device *dev, uint16_t word,
					 const struct spi_config *spi_cfg)
{
	uint8_t tx_buf[2];

	sys_put_be16(word, tx_buf);
	return it8951_spi_write_with_config(dev, tx_buf, sizeof(tx_buf), spi_cfg);
}

static int it8951_write_preamble_word(const struct device *dev, uint16_t preamble, uint16_t word)
{
	const struct it8951_config *config = dev->config;
	struct spi_config spi_cfg = config->spi.config;
	uint8_t tx_buf[2];
	int ret;

	ret = it8951_wait_ready(dev, config->ready_timeout_ms);
	if (ret < 0) {
		return ret;
	}

	spi_cfg.operation |= SPI_HOLD_ON_CS | SPI_LOCK_ON;

	sys_put_be16(preamble, tx_buf);
	ret = it8951_spi_write_with_config(dev, tx_buf, sizeof(tx_buf), &spi_cfg);
	if (ret < 0) {
		spi_release(config->spi.bus, &spi_cfg);
		return ret;
	}

	ret = it8951_wait_ready(dev, config->ready_timeout_ms);
	if (ret < 0) {
		spi_release(config->spi.bus, &spi_cfg);
		return ret;
	}

	sys_put_be16(word, tx_buf);
	ret = it8951_spi_write_with_config(dev, tx_buf, sizeof(tx_buf), &spi_cfg);
	spi_release(config->spi.bus, &spi_cfg);
	return ret;
}

static int it8951_write_cmd(const struct device *dev, uint16_t cmd)
{
	return it8951_write_preamble_word(dev, IT8951_PREAMBLE_WRITE_CMD, cmd);
}

static int it8951_write_data(const struct device *dev, uint16_t data)
{
	return it8951_write_preamble_word(dev, IT8951_PREAMBLE_WRITE_DATA, data);
}

static int it8951_read_n_data(const struct device *dev, uint16_t *buf, size_t count)
{
	const struct it8951_config *config = dev->config;
	struct spi_config spi_cfg = config->spi.config;
	uint8_t tx_buf[2];
	uint8_t rx_buf[2];
	const struct spi_buf tx = {
		.buf = tx_buf,
		.len = sizeof(uint16_t),
	};
	const struct spi_buf rx = {
		.buf = rx_buf,
		.len = sizeof(uint16_t),
	};
	const struct spi_buf_set tx_set = {
		.buffers = &tx,
		.count = 1,
	};
	const struct spi_buf_set rx_set = {
		.buffers = &rx,
		.count = 1,
	};
	int ret;

	ret = it8951_wait_ready(dev, config->ready_timeout_ms);
	if (ret < 0) {
		return ret;
	}

	spi_cfg.operation |= SPI_HOLD_ON_CS | SPI_LOCK_ON;

	/* Send read preamble (CS asserted and held throughout the burst) */
	ret = it8951_write_word_with_config(dev, IT8951_PREAMBLE_READ_DATA, &spi_cfg);
	if (ret < 0) {
		goto out;
	}

	/* Wait ready, then clock out the mandatory dummy word */
	ret = it8951_wait_ready(dev, config->ready_timeout_ms);
	if (ret < 0) {
		goto out;
	}

	ret = spi_transceive(config->spi.bus, &spi_cfg, &tx_set, &rx_set);
	if (ret < 0) {
		goto out;
	}

	/* Wait ready, then read all data words in a single burst */
	ret = it8951_wait_ready(dev, config->ready_timeout_ms);
	if (ret < 0) {
		goto out;
	}

	for (size_t i = 0; i < count; i++) {
		ret = spi_transceive(config->spi.bus, &spi_cfg, &tx_set, &rx_set);
		if (ret < 0) {
			goto out;
		}
		buf[i] = sys_get_be16(rx_buf);
	}

	ret = 0;

out:
	spi_release(config->spi.bus, &spi_cfg);

	return ret;
}

static int it8951_read_data(const struct device *dev, uint16_t *data)
{
	return it8951_read_n_data(dev, data, 1);
}

static int it8951_send_cmd_args(const struct device *dev, uint16_t cmd, const uint16_t *args,
				size_t arg_count)
{
	int ret;

	ret = it8951_write_cmd(dev, cmd);
	if (ret < 0) {
		return ret;
	}

	for (size_t i = 0; i < arg_count; i++) {
		ret = it8951_write_data(dev, args[i]);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int it8951_read_reg(const struct device *dev, uint16_t reg, uint16_t *value)
{
	int ret;

	ret = it8951_write_cmd(dev, IT8951_CMD_REG_READ);
	if (ret < 0) {
		return ret;
	}

	ret = it8951_write_data(dev, reg);
	if (ret < 0) {
		return ret;
	}

	return it8951_read_data(dev, value);
}

static int it8951_write_reg(const struct device *dev, uint16_t reg, uint16_t value)
{
	const uint16_t args[] = {reg, value};

	return it8951_send_cmd_args(dev, IT8951_CMD_REG_WRITE, args, ARRAY_SIZE(args));
}

static int it8951_get_info(const struct device *dev, struct it8951_dev_info *info)
{
	int ret;

	ret = it8951_write_cmd(dev, IT8951_CMD_GET_INFO);
	if (ret < 0) {
		return ret;
	}

	return it8951_read_n_data(dev, (uint16_t *)info, sizeof(*info) / sizeof(uint16_t));
}

static int it8951_set_img_buf_base_addr(const struct device *dev, uint32_t addr)
{
	int ret;

	ret = it8951_write_reg(dev, IT8951_REG_LISAR + 2U, (uint16_t)(addr >> 16));
	if (ret < 0) {
		return ret;
	}

	return it8951_write_reg(dev, IT8951_REG_LISAR, (uint16_t)addr);
}

static int it8951_set_vcom(const struct device *dev)
{
	const struct it8951_config *config = dev->config;
	const uint16_t args[] = {0x0002U, config->vcom_mv};

	return it8951_send_cmd_args(dev, IT8951_CMD_VCOM, args, ARRAY_SIZE(args));
}

static int it8951_write_packed_l4(const struct device *dev, const uint8_t *buf, uint16_t x,
				  uint16_t y, uint16_t width, uint16_t height)
{
	const struct it8951_config *config = dev->config;
	struct it8951_data *data = dev->data;
	struct spi_config spi_cfg = config->spi.config;
	uint16_t load_arg = (IT8951_ENDIAN_LITTLE << 8) | (IT8951_LOAD_IMAGE_PIXFMT_4BPP << 4);
	uint16_t ld_x = it8951_mirror_x(config, x, width);
	uint16_t args[] = {load_arg, ld_x, y, width, height};
	uint16_t words_per_line = DIV_ROUND_UP(width, 4U);
	size_t row_buf_len = (size_t)words_per_line * sizeof(uint16_t);
	uint8_t *row_buf = config->row_spi_buf;
	int ret;

	if (row_buf_len > config->row_spi_buf_cap) {
		return -EINVAL;
	}

	ret = it8951_set_img_buf_base_addr(dev, data->img_buf_addr);
	if (ret < 0) {
		return ret;
	}

	ret = it8951_send_cmd_args(dev, IT8951_CMD_LOAD_AREA, args, ARRAY_SIZE(args));
	if (ret < 0) {
		return ret;
	}

	ret = it8951_wait_ready(dev, config->ready_timeout_ms);
	if (ret < 0) {
		return ret;
	}

	spi_cfg.operation |= SPI_HOLD_ON_CS | SPI_LOCK_ON;

	ret = it8951_write_word_with_config(dev, IT8951_PREAMBLE_WRITE_DATA, &spi_cfg);
	if (ret < 0) {
		spi_release(config->spi.bus, &spi_cfg);
		return ret;
	}

	ret = it8951_wait_ready(dev, config->ready_timeout_ms);
	if (ret < 0) {
		spi_release(config->spi.bus, &spi_cfg);
		return ret;
	}

	for (uint16_t row = 0; row < height; row++) {
		const size_t row_off = (size_t)row * words_per_line * sizeof(uint16_t);
		uint16_t *words = (uint16_t *)(void *)row_buf;

		memcpy(row_buf, buf + row_off, row_buf_len);
		for (uint16_t widx = 0; widx < words_per_line; widx++) {
			words[widx] = sys_cpu_to_be16(words[widx]);
		}

		ret = it8951_spi_write_with_config(dev, row_buf, row_buf_len, &spi_cfg);
		if (ret < 0) {
			spi_release(config->spi.bus, &spi_cfg);
			return ret;
		}
	}

	spi_release(config->spi.bus, &spi_cfg);

	return it8951_write_cmd(dev, IT8951_CMD_LOAD_END);
}

static int it8951_set_panel_temp(const struct device *dev, uint16_t temp_c)
{
	const uint16_t args[] = {IT8951_PANEL_TEMP_SET_SEL, temp_c};

	return it8951_send_cmd_args(dev, IT8951_CMD_PANEL_MISC, args, ARRAY_SIZE(args));
}

static int it8951_apply_wakeup_calibration(const struct device *dev)
{
	const struct it8951_config *config = dev->config;

	if (!config->has_panel_temp) {
		return 0;
	}

	return it8951_set_panel_temp(dev, (uint16_t)config->panel_temp_c);
}

static int it8951_display_refresh(const struct device *dev, uint16_t x, uint16_t y, uint16_t width,
				  uint16_t height, uint16_t waveform)
{
	const struct it8951_config *config = dev->config;
	uint16_t disp_x = it8951_mirror_x(config, x, width);
	const uint16_t args[] = {
		disp_x, y, width, height, waveform,
	};

	return it8951_send_cmd_args(dev, IT8951_CMD_DISPLAY_AREA, args, ARRAY_SIZE(args));
}

static int it8951_wait_display_ready(const struct device *dev)
{
	const struct it8951_config *config = dev->config;
	k_timepoint_t deadline = sys_timepoint_calc(K_MSEC(config->refresh_timeout_ms));
	uint16_t status;
	bool logged_busy = false;
	int ret;

	do {
		ret = it8951_read_reg(dev, IT8951_REG_LUTAFSR, &status);
		if (ret < 0) {
			return ret;
		}

		if (status == 0U) {
			return 0;
		}

		if (!logged_busy) {
			LOG_DBG("LUTAFSR busy status=0x%04x", status);
			logged_busy = true;
		}
		k_msleep(10);
	} while (!sys_timepoint_expired(deadline));

	LOG_ERR("Timeout waiting for display refresh");
	return -ETIMEDOUT;
}

static void it8951_mark_dirty(const struct device *dev, uint16_t x, uint16_t y, uint16_t width,
			      uint16_t height)
{
	struct it8951_data *data = dev->data;
	uint16_t x_max = x + width;
	uint16_t y_max = y + height;

	if (!data->dirty) {
		data->dirty_x_min = x;
		data->dirty_y_min = y;
		data->dirty_x_max = x_max;
		data->dirty_y_max = y_max;
		data->dirty = true;
		return;
	}

	if (x < data->dirty_x_min) {
		data->dirty_x_min = x;
	}

	if (y < data->dirty_y_min) {
		data->dirty_y_min = y;
	}

	if (x_max > data->dirty_x_max) {
		data->dirty_x_max = x_max;
	}

	if (y_max > data->dirty_y_max) {
		data->dirty_y_max = y_max;
	}
}

static int it8951_assert_panel_enables(const struct device *dev)
{
	const struct it8951_config *config = dev->config;
	int ret;

	if (config->enable_gpio.port != NULL) {
		ret = gpio_pin_set_dt(&config->enable_gpio, 1);
		if (ret < 0) {
			return ret;
		}
	}

	if (config->ite_enable_gpio.port != NULL) {
		ret = gpio_pin_set_dt(&config->ite_enable_gpio, 1);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int it8951_flush_dirty(const struct device *dev)
{
	const struct it8951_config *config = dev->config;
	struct it8951_data *data = dev->data;
	uint16_t width;
	uint16_t height;
	int ret;

	if (!data->dirty) {
		return 0;
	}

	ret = it8951_assert_panel_enables(dev);
	if (ret < 0) {
		LOG_ERR("Failed to assert panel enables: %d", ret);
		return ret;
	}

	ret = it8951_wait_display_ready(dev);
	if (ret < 0) {
		return ret;
	}

	width = data->dirty_x_max - data->dirty_x_min;
	height = data->dirty_y_max - data->dirty_y_min;

	ret = it8951_display_refresh(dev, data->dirty_x_min, data->dirty_y_min, width, height,
				     config->refresh_waveform);
	if (ret < 0) {
		return ret;
	}

	ret = it8951_wait_ready(dev, config->ready_timeout_ms);
	if (ret < 0) {
		return ret;
	}

	ret = it8951_wait_display_ready(dev);

	if (ret == 0) {
		LOG_DBG("refresh ok x=%u y=%u w=%u h=%u waveform=%u", data->dirty_x_min,
			data->dirty_y_min, width, height, config->refresh_waveform);
		data->dirty = false;
	}

	return ret;
}

static int it8951_hw_reset(const struct device *dev)
{
	const struct it8951_config *config = dev->config;
	int ret;

	ret = it8951_assert_panel_enables(dev);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_set_dt(&config->reset_gpio, 0);
	if (ret < 0) {
		return ret;
	}

	k_msleep(5);

	ret = gpio_pin_set_dt(&config->reset_gpio, 1);
	if (ret < 0) {
		return ret;
	}

	k_msleep(config->reset_delay_ms);

	ret = gpio_pin_set_dt(&config->reset_gpio, 0);
	if (ret < 0) {
		return ret;
	}

	k_msleep(config->reset_wait_ms);

	return it8951_wait_ready(dev, config->ready_timeout_ms);
}

static int it8951_controller_init(const struct device *dev, bool full_init)
{
	const struct it8951_config *config = dev->config;
	struct it8951_data *data = dev->data;
	struct it8951_dev_info info;
	int ret;

	ret = it8951_assert_panel_enables(dev);
	if (ret < 0) {
		LOG_ERR("Failed to assert panel enables at controller init: %d", ret);
		return ret;
	}

	if (full_init) {
		ret = it8951_set_vcom(dev);
		if (ret < 0) {
			LOG_ERR("Failed to set VCOM: %d", ret);
			return ret;
		}
	}

	ret = it8951_get_info(dev, &info);
	if (ret < 0) {
		LOG_ERR("Failed to read device info: %d", ret);
		return ret;
	}

	if (info.panel_width == 0U || info.panel_height == 0U) {
		LOG_ERR("Invalid panel size from IT8951");
		return -ENODEV;
	}

	if (info.panel_width != config->width || info.panel_height != config->height) {
		LOG_WRN("IT8951 reports %ux%u panel, devicetree config is %ux%u", info.panel_width,
			info.panel_height, config->width, config->height);
	}

	data->img_buf_addr = info.img_buf_addr_l | ((uint32_t)info.img_buf_addr_h << 16);
	data->dirty = false;

	LOG_DBG("IT8951 panel=%ux%u img_buf=0x%08x fw='%.*s' lut='%.*s'", info.panel_width,
		info.panel_height, data->img_buf_addr, (int)sizeof(info.fw_version),
		(char *)info.fw_version, (int)sizeof(info.lut_version), (char *)info.lut_version);

	ret = it8951_write_reg(dev, IT8951_REG_I80CPCR, IT8951_PACKED_ENABLE);
	if (ret < 0) {
		LOG_ERR("Failed to enable packed mode: %d", ret);
		return ret;
	}

	if (full_init) {
		ret = it8951_display_refresh(dev, 0U, 0U, config->width, config->height,
					     config->initial_refresh_waveform);
		if (ret < 0) {
			return ret;
		}

		ret = it8951_wait_ready(dev, config->ready_timeout_ms);
		if (ret < 0) {
			return ret;
		}

		ret = it8951_wait_display_ready(dev);
		if (ret < 0) {
			return ret;
		}

		ret = it8951_write_reg(dev, IT8951_REG_I80CPCR, IT8951_PACKED_ENABLE);
		if (ret < 0) {
			LOG_ERR("Failed to re-enable packed mode after init refresh: %d", ret);
			return ret;
		}
	}

	return 0;
}

static int it8951_enter_run_mode(const struct device *dev, bool full_init)
{
	const struct it8951_config *config = dev->config;
	int ret;

	ret = it8951_write_cmd(dev, IT8951_CMD_SYS_RUN);
	if (ret < 0) {
		LOG_ERR("SYS_RUN failed: %d", ret);
		return ret;
	}

	ret = it8951_wait_ready(dev, config->ready_timeout_ms);
	if (ret < 0) {
		return ret;
	}

	ret = it8951_apply_wakeup_calibration(dev);
	if (ret < 0) {
		LOG_ERR("Panel wakeup calibration failed: %d", ret);
		return ret;
	}

	return it8951_controller_init(dev, full_init);
}

static int it8951_write(const struct device *dev, const uint16_t x, const uint16_t y,
			const struct display_buffer_descriptor *desc, const void *buf)
{
	const struct it8951_config *config = dev->config;
	struct it8951_data *data = dev->data;
	size_t line_bytes;
	size_t buf_len;
	int ret;

	if (buf == NULL) {
		return -EINVAL;
	}

	if (desc->pitch != desc->width) {
		LOG_ERR("Pitch must match width");
		return -EINVAL;
	}

	if ((desc->width % 4U) != 0U) {
		LOG_ERR("Write width must be a multiple of 4 pixels");
		return -EINVAL;
	}

	if ((x + desc->width) > config->width || (y + desc->height) > config->height) {
		LOG_ERR("Write area outside panel bounds");
		return -EINVAL;
	}

	line_bytes = DIV_ROUND_UP(desc->width, 2U);
	buf_len = line_bytes * desc->height;
	if (desc->buf_size < buf_len) {
		LOG_ERR("Invalid buffer size: %zu < %zu", desc->buf_size, buf_len);
		return -EINVAL;
	}

	ret = it8951_write_packed_l4(dev, buf, x, y, desc->width, desc->height);
	if (ret < 0) {
		return ret;
	}

	it8951_mark_dirty(dev, x, y, desc->width, desc->height);

	if (!data->blanking_on && !desc->frame_incomplete) {
		ret = it8951_flush_dirty(dev);
	}

	return ret;
}

static int it8951_blanking_on(const struct device *dev)
{
	struct it8951_data *data = dev->data;

	data->blanking_on = true;
	return 0;
}

static int it8951_blanking_off(const struct device *dev)
{
	struct it8951_data *data = dev->data;

	data->blanking_on = false;
	return it8951_flush_dirty(dev);
}

static void it8951_get_capabilities(const struct device *dev, struct display_capabilities *caps)
{
	const struct it8951_config *config = dev->config;

	memset(caps, 0, sizeof(*caps));
	caps->x_resolution = config->width;
	caps->y_resolution = config->height;
	caps->supported_pixel_formats = PIXEL_FORMAT_L_4;
	caps->current_pixel_format = PIXEL_FORMAT_L_4;
	caps->screen_info = SCREEN_INFO_EPD;
}

static int it8951_set_pixel_format(const struct device *dev, const enum display_pixel_format pf)
{
	ARG_UNUSED(dev);

	if (pf != PIXEL_FORMAT_L_4) {
		return -ENOTSUP;
	}

	return 0;
}

static int it8951_init(const struct device *dev)
{
	const struct it8951_config *config = dev->config;
	struct it8951_data *data = dev->data;
	int ret;

	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR_DEVICE_NOT_READY(config->spi.bus);
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&config->busy_gpio)) {
		LOG_ERR_DEVICE_NOT_READY(config->busy_gpio.port);
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&config->reset_gpio)) {
		LOG_ERR_DEVICE_NOT_READY(config->reset_gpio.port);
		return -ENODEV;
	}

	if (config->enable_gpio.port != NULL && !gpio_is_ready_dt(&config->enable_gpio)) {
		LOG_ERR_DEVICE_NOT_READY(config->enable_gpio.port);
		return -ENODEV;
	}

	if (config->ite_enable_gpio.port != NULL && !gpio_is_ready_dt(&config->ite_enable_gpio)) {
		LOG_ERR_DEVICE_NOT_READY(config->ite_enable_gpio.port);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->busy_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure busy GPIO: %d", ret);
		return ret;
	}

	ret = gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure reset GPIO: %d", ret);
		return ret;
	}

	if (config->enable_gpio.port != NULL) {
		/*
		 * ACTIVE_HIGH panel / rail enables must release low-side switches early.
		 * GPIO_OUTPUT_INACTIVE keeps these pins at logical 0 (physical low) until
		 * software explicitly asserts them, which can leave the EPD analog rails off
		 * even though the IT8951 responds on SPI.
		 */
		ret = gpio_pin_configure_dt(&config->enable_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to configure enable GPIO: %d", ret);
			return ret;
		}
	}

	if (config->ite_enable_gpio.port != NULL) {
		ret = gpio_pin_configure_dt(&config->ite_enable_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to configure ITE enable GPIO: %d", ret);
			return ret;
		}
	}

	data->blanking_on = true;

	ret = it8951_hw_reset(dev);
	if (ret < 0) {
		return ret;
	}

	return it8951_enter_run_mode(dev, true);
}

#ifdef CONFIG_PM_DEVICE
static int it8951_pm_action(const struct device *dev, enum pm_device_action action)
{
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = it8951_hw_reset(dev);
		if (ret < 0) {
			return ret;
		}
		return it8951_enter_run_mode(dev, false);
	case PM_DEVICE_ACTION_SUSPEND:
		return it8951_write_cmd(dev, IT8951_CMD_SLEEP);
	default:
		return -ENOTSUP;
	}
}
#endif

static DEVICE_API(display, it8951_api) = {
	.blanking_on = it8951_blanking_on,
	.blanking_off = it8951_blanking_off,
	.write = it8951_write,
	.get_capabilities = it8951_get_capabilities,
	.set_pixel_format = it8951_set_pixel_format,
};

#define IT8951_ROW_SPI_BUF_SIZE(inst)                                                              \
	((size_t)DIV_ROUND_UP(DT_INST_PROP(inst, width), 4U) * sizeof(uint16_t))

#define IT8951_DEFINE(inst)                                                                        \
	static uint8_t it8951_row_spi_##inst[IT8951_ROW_SPI_BUF_SIZE(inst)];                       \
	static const struct it8951_config it8951_cfg_##inst = {                                    \
		.spi = SPI_DT_SPEC_INST_GET(inst, SPI_OP_MODE_MASTER | SPI_WORD_SET(8U)),          \
		.busy_gpio = GPIO_DT_SPEC_INST_GET(inst, busy_gpios),                              \
		.reset_gpio = GPIO_DT_SPEC_INST_GET(inst, reset_gpios),                            \
		.enable_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, enable_gpios, {0}),                  \
		.ite_enable_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, ite_enable_gpios, {0}),          \
		.width = DT_INST_PROP(inst, width),                                                \
		.height = DT_INST_PROP(inst, height),                                              \
		.vcom_mv = DT_INST_PROP(inst, vcom_mv),                                            \
		.initial_refresh_waveform = DT_INST_PROP(inst, initial_refresh_waveform),          \
		.refresh_waveform = DT_INST_PROP(inst, refresh_waveform),                          \
		.h_mirror = DT_INST_PROP(inst, h_mirror),                                          \
		.has_panel_temp = DT_INST_NODE_HAS_PROP(inst, panel_temp_c),                       \
		.panel_temp_c = DT_INST_PROP_OR(inst, panel_temp_c, 0),                            \
		.reset_delay_ms = DT_INST_PROP(inst, reset_delay_ms),                              \
		.reset_wait_ms = DT_INST_PROP(inst, reset_wait_ms),                                \
		.ready_timeout_ms = DT_INST_PROP(inst, ready_timeout_ms),                          \
		.refresh_timeout_ms = DT_INST_PROP(inst, refresh_timeout_ms),                      \
		.row_spi_buf = it8951_row_spi_##inst,                                              \
		.row_spi_buf_cap = sizeof(it8951_row_spi_##inst),                                  \
	};                                                                                         \
	static struct it8951_data it8951_data_##inst;                                              \
	PM_DEVICE_DT_INST_DEFINE(inst, it8951_pm_action);                                          \
	DEVICE_DT_INST_DEFINE(inst, it8951_init, PM_DEVICE_DT_INST_GET(inst), &it8951_data_##inst, \
			      &it8951_cfg_##inst, POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY,       \
			      &it8951_api);

DT_INST_FOREACH_STATUS_OKAY(IT8951_DEFINE)
