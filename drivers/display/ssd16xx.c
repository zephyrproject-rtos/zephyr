/*
 * Copyright (c) 2022 Andreas Sandberg
 * Copyright (c) 2018-2020 PHYTEC Messtechnik GmbH
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL CONFIG_DISPLAY_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ssd16xx);

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mipi_dbi.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/display/ssd16xx.h>
#include "ssd16xx_regs.h"

/**
 * SSD16xx compatible EPD controller driver.
 */

#define EPD_PANEL_NUMOF_ROWS_PER_PAGE	8
#define SSD16XX_PANEL_FIRST_PAGE	0
#define SSD16XX_PANEL_FIRST_GATE	0
#define SSD16XX_PIXELS_PER_BYTE		8
#define SSD16XX_DEFAULT_TR_VALUE	25U
#define SSD16XX_TR_SCALE_FACTOR		256U


enum ssd16xx_profile_type {
	SSD16XX_PROFILE_FULL = 0,
	SSD16XX_PROFILE_PARTIAL,
	SSD16XX_NUM_PROFILES,
	SSD16XX_PROFILE_INVALID = SSD16XX_NUM_PROFILES,
};

struct ssd16xx_quirks {
	/* Gates */
	uint16_t max_width;
	/* Sources */
	uint16_t max_height;
	/* Width (bits) of integer type representing an x coordinate */
	uint8_t pp_width_bits;
	/* Width (bits) of integer type representing a y coordinate */
	uint8_t pp_height_bits;

	/*
	 * Device specific flags to be included in
	 * SSD16XX_CMD_UPDATE_CTRL2 for a full refresh.
	 */
	uint8_t ctrl2_full;
	/*
	 * Device specific flags to be included in
	 * SSD16XX_CMD_UPDATE_CTRL2 for a partial refresh.
	 */
	uint8_t ctrl2_partial;
};

struct ssd16xx_data {
	bool read_supported;
	uint8_t scan_mode;
	bool blanking_on;
	enum ssd16xx_profile_type profile;
	enum display_orientation orientation;
};

struct ssd16xx_dt_array {
	uint8_t *data;
	uint8_t len;
};

struct ssd16xx_profile {
	struct ssd16xx_dt_array lut;
	struct ssd16xx_dt_array gdv;
	struct ssd16xx_dt_array sdv;
	uint8_t vcom;
	uint8_t bwf;
	uint8_t dummy_line;
	uint8_t gate_line_width;

	bool override_vcom;
	bool override_bwf;
	bool override_dummy_line;
	bool override_gate_line_width;
};

struct ssd16xx_config {
	const struct device *mipi_dev;
	const struct mipi_dbi_config dbi_config;
	struct gpio_dt_spec busy_gpio;

	const struct ssd16xx_quirks *quirks;

	struct ssd16xx_dt_array softstart;

	const struct ssd16xx_profile *profiles[SSD16XX_NUM_PROFILES];

	uint16_t rotation;
	uint16_t height;
	uint16_t width;
	uint8_t tssv;
};

static int ssd16xx_set_profile(const struct device *dev,
			       enum ssd16xx_profile_type type);

static inline void ssd16xx_busy_wait(const struct device *dev)
{
	const struct ssd16xx_config *config = dev->config;
	int pin = gpio_pin_get_dt(&config->busy_gpio);

	while (pin > 0) {
		__ASSERT(pin >= 0, "Failed to get pin level");
		k_msleep(SSD16XX_BUSY_DELAY);
		pin = gpio_pin_get_dt(&config->busy_gpio);
	}
}

static inline int ssd16xx_write_cmd(const struct device *dev, uint8_t cmd,
				    const uint8_t *data, size_t len)
{
	const struct ssd16xx_config *config = dev->config;
	int err;

	ssd16xx_busy_wait(dev);

	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config,
				      cmd, data, len);
	mipi_dbi_release(config->mipi_dev, &config->dbi_config);
	return err;
}

static inline int ssd16xx_write_uint8(const struct device *dev, uint8_t cmd,
				      uint8_t data)
{
	return ssd16xx_write_cmd(dev, cmd, &data, 1);
}

static inline int ssd16xx_read_cmd(const struct device *dev, uint8_t cmd,
				    uint8_t *data, size_t len)
{
	const struct ssd16xx_config *config = dev->config;
	const struct ssd16xx_data *dev_data = dev->data;

	if (!dev_data->read_supported) {
		return -ENOTSUP;
	}

	ssd16xx_busy_wait(dev);

	return mipi_dbi_command_read(config->mipi_dev, &config->dbi_config,
				     &cmd, 1, data, len);
}

static inline size_t push_x_param(const struct device *dev,
				  uint8_t *data, uint16_t x)
{
	const struct ssd16xx_config *config = dev->config;

	if (config->quirks->pp_width_bits == 8) {
		data[0] = (uint8_t)x;
		return 1;
	}

	if (config->quirks->pp_width_bits == 16) {
		sys_put_le16(sys_cpu_to_le16(x), data);
		return 2;
	}

	LOG_ERR("Unsupported pp_width_bits %u",
		config->quirks->pp_width_bits);
	return 0;
}

static inline size_t push_y_param(const struct device *dev,
				  uint8_t *data, uint16_t y)
{
	const struct ssd16xx_config *config = dev->config;

	if (config->quirks->pp_height_bits == 8) {
		data[0] = (uint8_t)y;
		return 1;
	}

	if (config->quirks->pp_height_bits == 16) {
		sys_put_le16(sys_cpu_to_le16(y), data);
		return 2;
	}

	LOG_ERR("Unsupported pp_height_bitsa %u",
		config->quirks->pp_height_bits);
	return 0;
}



static inline int ssd16xx_set_ram_param(const struct device *dev,
					uint16_t sx, uint16_t ex,
					uint16_t sy, uint16_t ey)
{
	int err;
	uint8_t tmp[4];
	size_t len;

	len  = push_x_param(dev, tmp, sx);
	len += push_x_param(dev, tmp + len, ex);
	err = ssd16xx_write_cmd(dev, SSD16XX_CMD_RAM_XPOS_CTRL, tmp, len);
	if (err < 0) {
		return err;
	}

	len  = push_y_param(dev, tmp, sy);
	len += push_y_param(dev, tmp + len, ey);
	err = ssd16xx_write_cmd(dev, SSD16XX_CMD_RAM_YPOS_CTRL, tmp,	len);
	if (err < 0) {
		return err;
	}

	return 0;
}

static inline int ssd16xx_set_ram_ptr(const struct device *dev, uint16_t x,
				      uint16_t y)
{
	int err;
	uint8_t tmp[2];
	size_t len;

	len = push_x_param(dev, tmp, x);
	err = ssd16xx_write_cmd(dev, SSD16XX_CMD_RAM_XPOS_CNTR, tmp, len);
	if (err < 0) {
		return err;
	}

	len = push_y_param(dev, tmp, y);
	return ssd16xx_write_cmd(dev, SSD16XX_CMD_RAM_YPOS_CNTR, tmp, len);
}

static int ssd16xx_activate(const struct device *dev, uint8_t ctrl2)
{
	int err;

	err = ssd16xx_write_uint8(dev, SSD16XX_CMD_UPDATE_CTRL2, ctrl2);
	if (err < 0) {
		return err;
	}

	return ssd16xx_write_cmd(dev, SSD16XX_CMD_MASTER_ACTIVATION, NULL, 0);
}

static int ssd16xx_update_display(const struct device *dev)
{
	const struct ssd16xx_config *config = dev->config;
	const struct ssd16xx_data *data = dev->data;
	const struct ssd16xx_profile *p = config->profiles[data->profile];
	const struct ssd16xx_quirks *quirks = config->quirks;
	const bool load_lut = !p || p->lut.len == 0;
	const bool load_temp = load_lut && config->tssv;
	const bool partial = data->profile == SSD16XX_PROFILE_PARTIAL;
	const uint8_t update_cmd =
		SSD16XX_CTRL2_ENABLE_CLK |
		SSD16XX_CTRL2_ENABLE_ANALOG |
		(load_lut ? SSD16XX_CTRL2_LOAD_LUT : 0) |
		(load_temp ? SSD16XX_CTRL2_LOAD_TEMPERATURE : 0) |
		(partial ? quirks->ctrl2_partial : quirks->ctrl2_full) |
		SSD16XX_CTRL2_DISABLE_ANALOG |
		SSD16XX_CTRL2_DISABLE_CLK;

	return ssd16xx_activate(dev, update_cmd);
}

static int ssd16xx_blanking_off(const struct device *dev)
{
	struct ssd16xx_data *data = dev->data;

	if (data->blanking_on) {
		data->blanking_on = false;
		return ssd16xx_update_display(dev);
	}

	return 0;
}

static int ssd16xx_blanking_on(const struct device *dev)
{
	struct ssd16xx_data *data = dev->data;

	if (!data->blanking_on) {
		if (ssd16xx_set_profile(dev, SSD16XX_PROFILE_FULL)) {
			return -EIO;
		}
	}

	data->blanking_on = true;

	return 0;
}

static int ssd16xx_set_window(const struct device *dev,
			      const uint16_t x, const uint16_t y,
			      const struct display_buffer_descriptor *desc)
{
	const struct ssd16xx_config *config = dev->config;
	const struct ssd16xx_data *data = dev->data;
	int err;
	uint16_t x_start;
	uint16_t x_end;
	uint16_t y_start;
	uint16_t y_end;
	uint16_t panel_h = config->height -
			   config->height % EPD_PANEL_NUMOF_ROWS_PER_PAGE;

	if (desc->pitch < desc->width) {
		LOG_ERR("Pitch is smaller than width");
		return -EINVAL;
	}

	if (desc->pitch > desc->width) {
		LOG_ERR("Unsupported mode");
		return -ENOTSUP;
	}

	if (data->orientation == DISPLAY_ORIENTATION_NORMAL ||
	    data->orientation == DISPLAY_ORIENTATION_ROTATED_180) {
		if ((y + desc->height) > panel_h) {
			LOG_ERR("Buffer out of bounds (height)");
			return -EINVAL;
		}

		if ((x + desc->width) > config->width) {
			LOG_ERR("Buffer out of bounds (width)");
			return -EINVAL;
		}

		if ((desc->height % EPD_PANEL_NUMOF_ROWS_PER_PAGE) != 0U) {
			LOG_ERR("Buffer height not multiple of %d", EPD_PANEL_NUMOF_ROWS_PER_PAGE);
			return -EINVAL;
		}

		if ((y % EPD_PANEL_NUMOF_ROWS_PER_PAGE) != 0U) {
			LOG_ERR("Y coordinate not multiple of %d", EPD_PANEL_NUMOF_ROWS_PER_PAGE);
			return -EINVAL;
		}
	} else {
		if ((y + desc->height) > config->width) {
			LOG_ERR("Buffer out of bounds (height)");
			return -EINVAL;
		}

		if ((x + desc->width) > panel_h) {
			LOG_ERR("Buffer out of bounds (width)");
			return -EINVAL;
		}

		if ((desc->width % SSD16XX_PIXELS_PER_BYTE) != 0U) {
			LOG_ERR("Buffer width not multiple of %d", SSD16XX_PIXELS_PER_BYTE);
			return -EINVAL;
		}

		if ((x % SSD16XX_PIXELS_PER_BYTE) != 0U) {
			LOG_ERR("X coordinate not multiple of %d", SSD16XX_PIXELS_PER_BYTE);
			return -EINVAL;
		}
	}

	switch (data->orientation) {
	case DISPLAY_ORIENTATION_NORMAL:
		x_start = (panel_h - 1 - y) / SSD16XX_PIXELS_PER_BYTE;
		x_end = (panel_h - 1 - (y + desc->height - 1)) / SSD16XX_PIXELS_PER_BYTE;
		y_start = x;
		y_end = (x + desc->width - 1);
		break;
	case DISPLAY_ORIENTATION_ROTATED_90:
		x_start = (panel_h - 1 - x) / SSD16XX_PIXELS_PER_BYTE;
		x_end = (panel_h - 1 - (x + desc->width - 1)) / SSD16XX_PIXELS_PER_BYTE;
		y_start = (config->width - 1 - y);
		y_end = (config->width - 1 - (y + desc->height - 1));
		break;
	case DISPLAY_ORIENTATION_ROTATED_180:
		x_start = y / SSD16XX_PIXELS_PER_BYTE;
		x_end = (y + desc->height - 1) / SSD16XX_PIXELS_PER_BYTE;
		y_start = (x + desc->width - 1);
		y_end = x;
		break;
	case DISPLAY_ORIENTATION_ROTATED_270:
		x_start = x / SSD16XX_PIXELS_PER_BYTE;
		x_end = (x + desc->width - 1) / SSD16XX_PIXELS_PER_BYTE;
		y_start = y;
		y_end = (y + desc->height - 1);
		break;
	default:
		return -EINVAL;
	}

	err = ssd16xx_set_ram_param(dev, x_start, x_end, y_start, y_end);
	if (err < 0) {
		return err;
	}

	err = ssd16xx_set_ram_ptr(dev, x_start, y_start);
	if (err < 0) {
		return err;
	}

	return 0;
}

static int ssd16xx_write(const struct device *dev, const uint16_t x,
			 const uint16_t y,
			 const struct display_buffer_descriptor *desc,
			 const void *buf)
{
	const struct ssd16xx_config *config = dev->config;
	const struct ssd16xx_data *data = dev->data;
	const bool have_partial_refresh =
		config->profiles[SSD16XX_PROFILE_PARTIAL] != NULL;
	const bool partial_refresh = !data->blanking_on && have_partial_refresh;
	const size_t buf_len = MIN(desc->buf_size,
				   desc->height * desc->width / 8);
	int err;

	if (buf == NULL || buf_len == 0U) {
		LOG_ERR("Display buffer is not available");
		return -EINVAL;
	}

	if (partial_refresh) {
		/*
		 * Request the partial profile. This operation becomes
		 * a no-op if the profile is already active.
		 */
		err = ssd16xx_set_profile(dev, SSD16XX_PROFILE_PARTIAL);
		if (err < 0) {
			return -EIO;
		}
	}

	err = ssd16xx_set_window(dev, x, y, desc);
	if (err < 0) {
		return err;
	}

	err = ssd16xx_write_cmd(dev, SSD16XX_CMD_WRITE_RAM, (uint8_t *)buf,
				buf_len);
	if (err < 0) {
		return err;
	}

	if (!data->blanking_on) {
		err = ssd16xx_update_display(dev);
		if (err < 0) {
			return err;
		}
	}

	if (data->blanking_on && have_partial_refresh) {
		/*
		 * We will trigger a full refresh when blanking is
		 * turned off. The controller won't keep track of the
		 * old frame buffer, which is needed to perform a
		 * partial update, when this happens. Maintain the old
		 * frame buffer manually here to make sure future
		 * partial updates will work as expected.
		 */
		err = ssd16xx_write_cmd(dev, SSD16XX_CMD_WRITE_RED_RAM,
					(uint8_t *)buf, buf_len);
		if (err < 0) {
			return err;
		}
	} else if (partial_refresh) {
		/*
		 * We just performed a partial refresh. After the
		 * refresh, the controller swaps the black/red buffers
		 * containing the current and new image. We need to
		 * perform a second write here to ensure that future
		 * updates work on an up-to-date framebuffer.
		 */
		err = ssd16xx_write_cmd(dev, SSD16XX_CMD_WRITE_RAM,
					(uint8_t *)buf, buf_len);
		if (err < 0) {
			return err;
		}
	}

	return 0;
}

int ssd16xx_read_ram(const struct device *dev, enum ssd16xx_ram ram_type,
		     const uint16_t x, const uint16_t y,
		     const struct display_buffer_descriptor *desc,
		     void *buf)
{
	const struct ssd16xx_data *data = dev->data;
	const size_t buf_len = MIN(desc->buf_size,
				   desc->height * desc->width / 8);
	int err;
	uint8_t ram_ctrl;

	if (!data->read_supported) {
		return -ENOTSUP;
	}

	switch (ram_type) {
	case SSD16XX_RAM_BLACK:
		ram_ctrl = SSD16XX_RAM_READ_CTRL_BLACK;
		break;

	case SSD16XX_RAM_RED:
		ram_ctrl = SSD16XX_RAM_READ_CTRL_RED;
		break;

	default:
		return -EINVAL;
	}

	if (buf == NULL || buf_len == 0U) {
		LOG_ERR("Display buffer is not available");
		return -EINVAL;
	}

	err = ssd16xx_set_window(dev, x, y, desc);
	if (err < 0) {
		return err;
	}

	err = ssd16xx_write_cmd(dev, SSD16XX_CMD_RAM_READ_CTRL,
				&ram_ctrl, sizeof(ram_ctrl));
	if (err < 0) {
		return err;
	}

	err = ssd16xx_read_cmd(dev, SSD16XX_CMD_READ_RAM, (uint8_t *)buf,
			       buf_len);
	if (err < 0) {
		return err;
	}

	return 0;
}

static int ssd16xx_read(const struct device *dev,
			const uint16_t x, const uint16_t y,
			const struct display_buffer_descriptor *desc,
			void *buf)
{
	return ssd16xx_read_ram(dev, SSD16XX_RAM_BLACK, x, y, desc, buf);
}

static void ssd16xx_get_capabilities(const struct device *dev,
				     struct display_capabilities *caps)
{
	const struct ssd16xx_config *config = dev->config;
	struct ssd16xx_data *data = dev->data;

	memset(caps, 0, sizeof(struct display_capabilities));
	caps->x_resolution = config->width;
	caps->y_resolution = config->height -
			     config->height % EPD_PANEL_NUMOF_ROWS_PER_PAGE;
	caps->supported_pixel_formats = PIXEL_FORMAT_MONO10;
	caps->current_pixel_format = PIXEL_FORMAT_MONO10;
	caps->screen_info = SCREEN_INFO_MONO_MSB_FIRST | SCREEN_INFO_EPD;

	if (data->orientation == DISPLAY_ORIENTATION_NORMAL ||
	    data->orientation == DISPLAY_ORIENTATION_ROTATED_180) {
		caps->screen_info |= SCREEN_INFO_MONO_VTILED;
	}

	caps->current_orientation = data->orientation;
}

static int ssd16xx_set_pixel_format(const struct device *dev,
				    const enum display_pixel_format pf)
{
	if (pf == PIXEL_FORMAT_MONO10) {
		return 0;
	}

	LOG_ERR("not supported");
	return -ENOTSUP;
}

static int ssd16xx_set_orientation(const struct device *dev,
				   const enum display_orientation orientation)
{
	struct ssd16xx_data *data = dev->data;
	int err;

	if (orientation == DISPLAY_ORIENTATION_NORMAL) {
		data->scan_mode = SSD16XX_DATA_ENTRY_XDYIY;
	} else if (orientation == DISPLAY_ORIENTATION_ROTATED_90) {
		data->scan_mode = SSD16XX_DATA_ENTRY_XDYDX;
	} else if (orientation == DISPLAY_ORIENTATION_ROTATED_180) {
		data->scan_mode = SSD16XX_DATA_ENTRY_XIYDY;
	} else if (orientation == DISPLAY_ORIENTATION_ROTATED_270) {
		data->scan_mode = SSD16XX_DATA_ENTRY_XIYIX;
	}

	err = ssd16xx_write_uint8(dev, SSD16XX_CMD_ENTRY_MODE, data->scan_mode);
	if (err < 0) {
		return err;
	}

	data->orientation = orientation;

	return 0;
}

static int ssd16xx_clear_cntlr_mem(const struct device *dev, uint8_t ram_cmd)
{
	const struct ssd16xx_config *config = dev->config;
	uint16_t panel_h = config->height / EPD_PANEL_NUMOF_ROWS_PER_PAGE;
	uint16_t last_gate = config->width - 1;
	uint8_t clear_page[64];
	int err;

	/*
	 * Clear unusable memory area when the resolution of the panel is not
	 * multiple of an octet.
	 */
	if (config->height % EPD_PANEL_NUMOF_ROWS_PER_PAGE) {
		panel_h += 1;
	}

	err = ssd16xx_write_uint8(dev, SSD16XX_CMD_ENTRY_MODE,
				  SSD16XX_DATA_ENTRY_XIYDY);
	if (err < 0) {
		return err;
	}

	err = ssd16xx_set_ram_param(dev, SSD16XX_PANEL_FIRST_PAGE,
				    panel_h - 1, last_gate,
				    SSD16XX_PANEL_FIRST_GATE);
	if (err < 0) {
		return err;
	}

	err = ssd16xx_set_ram_ptr(dev, SSD16XX_PANEL_FIRST_PAGE, last_gate);
	if (err < 0) {
		return err;
	}

	memset(clear_page, 0xff, sizeof(clear_page));
	for (int h = 0; h < panel_h; h++) {
		size_t x = config->width;

		while (x) {
			size_t l = MIN(x, sizeof(clear_page));

			x -= l;
			err = ssd16xx_write_cmd(dev, ram_cmd, clear_page, l);
			if (err < 0) {
				return err;
			}
		}
	}

	return 0;
}

static inline int ssd16xx_load_ws_from_otp_tssv(const struct device *dev)
{
	const struct ssd16xx_config *config = dev->config;

	/*
	 * Controller has an integrated temperature sensor or external
	 * temperature sensor is connected to the controller.
	 */
	LOG_INF("Select and load WS from OTP");
	return ssd16xx_write_uint8(dev, SSD16XX_CMD_TSENSOR_SELECTION,
				   config->tssv);
}

static inline int ssd16xx_load_ws_from_otp(const struct device *dev)
{
	int16_t t = (SSD16XX_DEFAULT_TR_VALUE * SSD16XX_TR_SCALE_FACTOR);
	uint8_t tmp[2];
	int err;

	LOG_INF("Load default WS (25 degrees Celsius) from OTP");

	err = ssd16xx_activate(dev, SSD16XX_CTRL2_ENABLE_CLK);
	if (err < 0) {
		return err;
	}

	/* Load temperature value */
	sys_put_be16(t, tmp);
	err = ssd16xx_write_cmd(dev, SSD16XX_CMD_TSENS_CTRL, tmp, 2);
	if (err < 0) {
		return err;
	}

	err = ssd16xx_activate(dev, SSD16XX_CTRL2_DISABLE_CLK);
	if (err < 0) {
		return err;
	}

	return 0;
}


static int ssd16xx_load_lut(const struct device *dev,
			    const struct ssd16xx_dt_array *lut)
{
	const struct ssd16xx_config *config = dev->config;

	if (lut && lut->len) {
		LOG_DBG("Using user-provided LUT");
		return ssd16xx_write_cmd(dev, SSD16XX_CMD_UPDATE_LUT,
					 lut->data, lut->len);
	} else {
		if (config->tssv) {
			return ssd16xx_load_ws_from_otp_tssv(dev);
		} else {
			return ssd16xx_load_ws_from_otp(dev);
		}
	}
}

static int ssd16xx_set_profile(const struct device *dev,
			       enum ssd16xx_profile_type type)
{
	const struct ssd16xx_config *config = dev->config;
	struct ssd16xx_data *data = dev->data;
	const struct ssd16xx_profile *p;
	const uint16_t last_gate = config->width - 1;
	uint8_t gdo[3];
	size_t gdo_len;
	int err = 0;

	if (type >= SSD16XX_NUM_PROFILES) {
		return -EINVAL;
	}

	p = config->profiles[type];

	/*
	 * The full profile is the only one that always exists. If it
	 * hasn't been specified, we use the defaults.
	 */
	if (!p && type != SSD16XX_PROFILE_FULL) {
		return -ENOENT;
	}

	if (type == data->profile) {
		return 0;
	}

	/*
	 * Perform a soft reset to make sure registers are reset. This
	 * will leave the RAM contents intact.
	 */
	err = ssd16xx_write_cmd(dev, SSD16XX_CMD_SW_RESET, NULL, 0);
	if (err < 0) {
		return err;
	}

	gdo_len = push_y_param(dev, gdo, last_gate);
	gdo[gdo_len++] = 0U;
	err = ssd16xx_write_cmd(dev, SSD16XX_CMD_GDO_CTRL, gdo, gdo_len);
	if (err < 0) {
		return err;
	}

	if (config->softstart.len) {
		err = ssd16xx_write_cmd(dev, SSD16XX_CMD_SOFTSTART,
					config->softstart.data,
					config->softstart.len);
		if (err < 0) {
			return err;
		}
	}

	err = ssd16xx_load_lut(dev, p ? &p->lut : NULL);
	if (err < 0) {
		return err;
	}

	if (p && p->override_dummy_line) {
		err = ssd16xx_write_uint8(dev, SSD16XX_CMD_DUMMY_LINE,
					  p->dummy_line);
		if (err < 0) {
			return err;
		}
	}

	if (p && p->override_gate_line_width) {
		err = ssd16xx_write_uint8(dev, SSD16XX_CMD_GATE_LINE_WIDTH,
					  p->override_gate_line_width);
		if (err < 0) {
			return err;
		}
	}

	if (p && p->gdv.len) {
		LOG_DBG("Setting GDV");
		err = ssd16xx_write_cmd(dev, SSD16XX_CMD_GDV_CTRL,
					p->gdv.data, p->gdv.len);
		if (err < 0) {
			return err;
		}
	}

	if (p && p->sdv.len) {
		LOG_DBG("Setting SDV");
		err = ssd16xx_write_cmd(dev, SSD16XX_CMD_SDV_CTRL,
					p->sdv.data, p->sdv.len);
		if (err < 0) {
			return err;
		}
	}

	if (p && p->override_vcom) {
		LOG_DBG("Setting VCOM");
		err = ssd16xx_write_cmd(dev, SSD16XX_CMD_VCOM_VOLTAGE,
					&p->vcom, 1);
		if (err < 0) {
			return err;
		}
	}

	if (p && p->override_bwf) {
		LOG_DBG("Setting BWF");
		err = ssd16xx_write_cmd(dev, SSD16XX_CMD_BWF_CTRL,
					&p->bwf, 1);
		if (err < 0) {
			return err;
		}
	}

	err = ssd16xx_write_uint8(dev, SSD16XX_CMD_ENTRY_MODE, data->scan_mode);
	if (err < 0) {
		return err;
	}

	data->profile = type;

	return 0;
}

static int ssd16xx_controller_init(const struct device *dev)
{
	const struct ssd16xx_config *config = dev->config;
	struct ssd16xx_data *data = dev->data;
	enum display_orientation orientation;
	int err;

	LOG_DBG("");

	data->blanking_on = false;
	data->profile = SSD16XX_PROFILE_INVALID;

	err = mipi_dbi_reset(config->mipi_dev, SSD16XX_RESET_DELAY);
	if (err < 0) {
		return err;
	}

	k_msleep(SSD16XX_RESET_DELAY);

	err = ssd16xx_set_profile(dev, SSD16XX_PROFILE_FULL);
	if (err < 0) {
		return err;
	}

	err = ssd16xx_clear_cntlr_mem(dev, SSD16XX_CMD_WRITE_RAM);
	if (err < 0) {
		return err;
	}

	err = ssd16xx_clear_cntlr_mem(dev, SSD16XX_CMD_WRITE_RED_RAM);
	if (err < 0) {
		return err;
	}

	if (config->rotation == 0U) {
		orientation = DISPLAY_ORIENTATION_NORMAL;
	} else if (config->rotation == 90U) {
		orientation = DISPLAY_ORIENTATION_ROTATED_90;
	} else if (config->rotation == 180U) {
		orientation = DISPLAY_ORIENTATION_ROTATED_180;
	} else {
		orientation = DISPLAY_ORIENTATION_ROTATED_270;
	}

	err = ssd16xx_set_orientation(dev, orientation);
	if (err < 0) {
		return err;
	}

	err = ssd16xx_update_display(dev);
	if (err < 0) {
		return err;
	}

	return 0;
}

static int ssd16xx_init(const struct device *dev)
{
	const struct ssd16xx_config *config = dev->config;
	struct ssd16xx_data *data = dev->data;
	int err;

	LOG_DBG("");

	if (!device_is_ready(config->mipi_dev)) {
		LOG_ERR("MIPI Device not ready");
		return -ENODEV;
	}

	data->read_supported =
		(config->dbi_config.config.operation & SPI_HALF_DUPLEX) != 0;

	if (!gpio_is_ready_dt(&config->busy_gpio)) {
		LOG_ERR("Busy GPIO device not ready");
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&config->busy_gpio, GPIO_INPUT);
	if (err < 0) {
		LOG_ERR("Failed to configure busy GPIO");
		return err;
	}

	if (config->width > config->quirks->max_width ||
	    config->height > config->quirks->max_height) {
		LOG_ERR("Display size out of range.");
		return -EINVAL;
	}

	return ssd16xx_controller_init(dev);
}

static const struct display_driver_api ssd16xx_driver_api = {
	.blanking_on = ssd16xx_blanking_on,
	.blanking_off = ssd16xx_blanking_off,
	.write = ssd16xx_write,
	.read = ssd16xx_read,
	.get_capabilities = ssd16xx_get_capabilities,
	.set_pixel_format = ssd16xx_set_pixel_format,
	.set_orientation = ssd16xx_set_orientation,
};

#if DT_HAS_COMPAT_STATUS_OKAY(solomon_ssd1608)
static struct ssd16xx_quirks quirks_solomon_ssd1608 = {
	.max_width = 320,
	.max_height = 240,
	.pp_width_bits = 16,
	.pp_height_bits = 16,
	.ctrl2_full = SSD16XX_GEN1_CTRL2_TO_PATTERN,
	.ctrl2_partial = SSD16XX_GEN1_CTRL2_TO_PATTERN,
};
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(solomon_ssd1673)
static struct ssd16xx_quirks quirks_solomon_ssd1673 = {
	.max_width = 250,
	.max_height = 150,
	.pp_width_bits = 8,
	.pp_height_bits = 8,
	.ctrl2_full = SSD16XX_GEN1_CTRL2_TO_PATTERN,
	.ctrl2_partial = SSD16XX_GEN1_CTRL2_TO_PATTERN,
};
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(solomon_ssd1675a)
static struct ssd16xx_quirks quirks_solomon_ssd1675a = {
	.max_width = 296,
	.max_height = 160,
	.pp_width_bits = 8,
	.pp_height_bits = 16,
	.ctrl2_full = SSD16XX_GEN1_CTRL2_TO_PATTERN,
	.ctrl2_partial = SSD16XX_GEN1_CTRL2_TO_PATTERN,
};
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(solomon_ssd1680)
static const struct ssd16xx_quirks quirks_solomon_ssd1680 = {
	.max_width = 296,
	.max_height = 176,
	.pp_width_bits = 8,
	.pp_height_bits = 16,
	.ctrl2_full = SSD16XX_GEN2_CTRL2_DISPLAY,
	.ctrl2_partial = SSD16XX_GEN2_CTRL2_DISPLAY | SSD16XX_GEN2_CTRL2_MODE2,
};
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(solomon_ssd1681)
static struct ssd16xx_quirks quirks_solomon_ssd1681 = {
	.max_width = 200,
	.max_height = 200,
	.pp_width_bits = 8,
	.pp_height_bits = 16,
	.ctrl2_full = SSD16XX_GEN2_CTRL2_DISPLAY,
	.ctrl2_partial = SSD16XX_GEN2_CTRL2_DISPLAY | SSD16XX_GEN2_CTRL2_MODE2,
};
#endif

#define SOFTSTART_ASSIGN(n)						\
		.softstart = {						\
			.data = softstart_##n,				\
			.len = sizeof(softstart_##n),			\
		},

#define SSD16XX_MAKE_ARRAY_OPT(n, p)					\
	static uint8_t data_ ## n ## _ ## p[] = DT_PROP_OR(n, p, {})

#define SSD16XX_ASSIGN_ARRAY(n, p)					\
	{								\
		.data = data_ ## n ## _ ## p,				\
		.len = sizeof(data_ ## n ## _ ## p),			\
	}

#define SSD16XX_PROFILE(n)						\
	SSD16XX_MAKE_ARRAY_OPT(n, lut);					\
	SSD16XX_MAKE_ARRAY_OPT(n, gdv);					\
	SSD16XX_MAKE_ARRAY_OPT(n, sdv);					\
									\
	static const struct ssd16xx_profile ssd16xx_profile_ ## n = {	\
		.lut = SSD16XX_ASSIGN_ARRAY(n, lut),			\
		.gdv = SSD16XX_ASSIGN_ARRAY(n, gdv),			\
		.sdv = SSD16XX_ASSIGN_ARRAY(n, sdv),			\
		.vcom = DT_PROP_OR(n, vcom, 0),				\
		.override_vcom = DT_NODE_HAS_PROP(n, vcom),		\
		.bwf = DT_PROP_OR(n, border_waveform, 0),		\
		.override_bwf = DT_NODE_HAS_PROP(n, border_waveform),	\
		.dummy_line = DT_PROP_OR(n, dummy_line, 0),		\
		.override_dummy_line = DT_NODE_HAS_PROP(n, dummy_line),	\
		.gate_line_width = DT_PROP_OR(n, gate_line_width, 0),	\
		.override_gate_line_width = DT_NODE_HAS_PROP(		\
			n, gate_line_width),				\
	};


#define _SSD16XX_PROFILE_PTR(n) &ssd16xx_profile_ ## n

#define SSD16XX_PROFILE_PTR(n)						\
	COND_CODE_1(DT_NODE_EXISTS(n),					\
		    (_SSD16XX_PROFILE_PTR(n)),				\
		    NULL)

#define SSD16XX_DEFINE(n, quirks_ptr)					\
	SSD16XX_MAKE_ARRAY_OPT(n, softstart);				\
									\
	DT_FOREACH_CHILD(n, SSD16XX_PROFILE);				\
									\
	static const struct ssd16xx_config ssd16xx_cfg_ ## n = {	\
		.mipi_dev = DEVICE_DT_GET(DT_PARENT(n)),                \
		.dbi_config = {                                         \
			.mode = MIPI_DBI_MODE_SPI_4WIRE,                \
			.config = MIPI_DBI_SPI_CONFIG_DT(n,             \
				SPI_OP_MODE_MASTER | SPI_WORD_SET(8) |  \
				SPI_HOLD_ON_CS | SPI_LOCK_ON, 0),       \
		},                                                      \
		.busy_gpio = GPIO_DT_SPEC_GET(n, busy_gpios),		\
		.quirks = quirks_ptr,					\
		.height = DT_PROP(n, height),				\
		.width = DT_PROP(n, width),				\
		.rotation = DT_PROP(n, rotation),			\
		.tssv = DT_PROP_OR(n, tssv, 0),				\
		.softstart = SSD16XX_ASSIGN_ARRAY(n, softstart),	\
		.profiles = {						\
			[SSD16XX_PROFILE_FULL] =			\
				SSD16XX_PROFILE_PTR(DT_CHILD(n, full)),	\
			[SSD16XX_PROFILE_PARTIAL] =			\
				SSD16XX_PROFILE_PTR(DT_CHILD(n, partial)),\
		},							\
	};								\
									\
	static struct ssd16xx_data ssd16xx_data_ ## n;			\
									\
	DEVICE_DT_DEFINE(n,						\
			 ssd16xx_init, NULL,				\
			 &ssd16xx_data_ ## n,				\
			 &ssd16xx_cfg_ ## n,				\
			 POST_KERNEL,					\
			 CONFIG_DISPLAY_INIT_PRIORITY,			\
			 &ssd16xx_driver_api)

DT_FOREACH_STATUS_OKAY_VARGS(solomon_ssd1608, SSD16XX_DEFINE,
			     &quirks_solomon_ssd1608);
DT_FOREACH_STATUS_OKAY_VARGS(solomon_ssd1673, SSD16XX_DEFINE,
			     &quirks_solomon_ssd1673);
DT_FOREACH_STATUS_OKAY_VARGS(solomon_ssd1675a, SSD16XX_DEFINE,
			     &quirks_solomon_ssd1675a);
DT_FOREACH_STATUS_OKAY_VARGS(solomon_ssd1680, SSD16XX_DEFINE,
			     &quirks_solomon_ssd1680);
DT_FOREACH_STATUS_OKAY_VARGS(solomon_ssd1681, SSD16XX_DEFINE,
			     &quirks_solomon_ssd1681);
