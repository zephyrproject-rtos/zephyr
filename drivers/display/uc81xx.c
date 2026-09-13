/*
 * Copyright (c) 2026 RASNA
 * Copyright (c) 2025 Cactus Engineering S.L
 * Copyright (c) 2022 Andreas Sandberg
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mipi_dbi.h>
#include <zephyr/sys/byteorder.h>

#include "uc81xx_regs.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uc81xx, CONFIG_DISPLAY_LOG_LEVEL);

/**
 * UC81XX compatible EPD controller driver.
 *
 * Currently only the black/white panels are supported (KW mode),
 * also first gate/source should be 0.
 *
 * The UC8253 can optionally use a four-level grayscale waveform.
 */

#define UC81XX_PIXELS_PER_BYTE		8U
#define UC8253_GRAY_LEVELS              4U

struct uc81xx_dt_array {
	uint8_t *data;
	uint8_t len;
};

enum uc81xx_profile_type {
	UC81XX_PROFILE_FULL = 0,
	UC81XX_PROFILE_PARTIAL,
	UC81XX_PROFILE_GRAY,
	UC81XX_NUM_PROFILES,
	UC81XX_PROFILE_INVALID = UC81XX_NUM_PROFILES,
};

enum uc81xx_phase {
	UC81XX_PHASE_NORMAL = 0,
	UC81XX_PHASE_SWAPPED,
};

struct uc81xx_profile {
	struct uc81xx_dt_array pwr;

	uint8_t cdi;
	bool override_cdi;
	uint8_t tcon;
	bool override_tcon;
	uint8_t  pll;
	bool override_pll;
	uint8_t  vdcs;
	bool override_vdcs;
	uint8_t ccset;
	bool override_ccset;
	uint8_t tsset;
	bool override_tsset;

	const struct uc81xx_dt_array lutc;
	const struct uc81xx_dt_array lutww;
	const struct uc81xx_dt_array lutkw;
	const struct uc81xx_dt_array lutwk;
	const struct uc81xx_dt_array lutkk;
	const struct uc81xx_dt_array lutbd;
};

struct uc81xx_quirks {
	uint16_t max_width;
	uint16_t max_height;

	bool auto_copy;
	bool pon_after_softstart;
	bool dtm_swap;

	int (*set_cdi)(const struct device *dev, bool border);
	int (*set_tres)(const struct device *dev);
	int (*set_ptl)(const struct device *dev, uint16_t x, uint16_t y,
		       uint16_t x_end_idx, uint16_t y_end_idx,
		       const struct display_buffer_descriptor *desc);
};

struct uc81xx_config {
	const struct uc81xx_quirks *quirks;

	const struct device *mipi_dev;
	const struct mipi_dbi_config dbi_config;
	struct gpio_dt_spec busy_gpio;

	uint16_t height;
	uint16_t width;
	bool grayscale;
	uint8_t gray_level_planes[UC8253_GRAY_LEVELS];
	size_t plane_size;
	uint8_t *plane_hi;
	uint8_t *plane_lo;

	struct uc81xx_dt_array softstart;

	const struct uc81xx_profile *profiles[UC81XX_NUM_PROFILES];
};

struct uc81xx_data {
	bool blanking_on;
	enum uc81xx_profile_type profile;
	enum uc81xx_phase phase;
};


static inline void uc81xx_busy_wait(const struct device *dev)
{
	const struct uc81xx_config *config = dev->config;
	int pin = gpio_pin_get_dt(&config->busy_gpio);

	while (pin > 0) {
		__ASSERT(pin >= 0, "Failed to get pin level");
		k_sleep(K_MSEC(UC81XX_BUSY_DELAY));
		pin = gpio_pin_get_dt(&config->busy_gpio);
	}
}

static inline int uc81xx_write_cmd(const struct device *dev, uint8_t cmd,
				   const uint8_t *data, size_t len)
{
	const struct uc81xx_config *config = dev->config;
	int err;

	uc81xx_busy_wait(dev);

	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config,
				     cmd, data, len);
	mipi_dbi_release(config->mipi_dev, &config->dbi_config);
	return err;
}

static inline int uc81xx_write_cmd_pattern(const struct device *dev,
					   uint8_t cmd,
					   uint8_t pattern, size_t len)
{
	const struct uc81xx_config *config = dev->config;
	struct display_buffer_descriptor mipi_desc = {0};
	int err;
	uint8_t data[64];

	uc81xx_busy_wait(dev);

	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config,
				     cmd, NULL, 0);
	if (err < 0) {
		mipi_dbi_release(config->mipi_dev, &config->dbi_config);
		return err;
	}

	/*
	 * MIPI display write API requires a display buffer descriptor.
	 * Create one that describes the buffer we are writing
	 */
	mipi_desc.height = 1;

	memset(data, pattern, sizeof(data));
	while (len) {
		mipi_desc.buf_size = mipi_desc.width = mipi_desc.pitch =
			MIN(len, sizeof(data));

		err = mipi_dbi_write_display(config->mipi_dev,
					     &config->dbi_config,
					     data, &mipi_desc,
					     PIXEL_FORMAT_MONO10);
		if (err < 0) {
			break;
		}

		len -= mipi_desc.buf_size;
	}

	mipi_dbi_release(config->mipi_dev, &config->dbi_config);
	return err;
}

static inline int uc81xx_write_cmd_uint8(const struct device *dev, uint8_t cmd,
					 uint8_t data)
{
	return uc81xx_write_cmd(dev, cmd, &data, 1);
}

static inline int uc81xx_write_array_opt(const struct device *dev, uint8_t cmd,
					 const struct uc81xx_dt_array *array)
{
	if (array->len && array->data) {
		return uc81xx_write_cmd(dev, cmd, array->data, array->len);
	} else {
		return 0;
	}
}

static int uc81xx_have_profile(const struct device *dev,
			       enum uc81xx_profile_type type)
{
	const struct uc81xx_config *config = dev->config;

	return type < UC81XX_NUM_PROFILES &&
		config->profiles[type];
}

static int uc81xx_set_profile(const struct device *dev,
			      enum uc81xx_profile_type type)
{
	const struct uc81xx_config *config = dev->config;
	const struct uc81xx_profile *p;
	struct uc81xx_data *data = dev->data;
	uint8_t psr =
		UC81XX_PSR_KW_R |
		UC81XX_PSR_UD |
		UC81XX_PSR_SHL |
		UC81XX_PSR_SHD |
		UC81XX_PSR_RST;

	if (type >= UC81XX_NUM_PROFILES) {
		return -EINVAL;
	}

	/* No need to update the current profile, so do nothing */
	if (data->profile == type) {
		return 0;
	}

	p = config->profiles[type];
	data->profile = type;

	LOG_DBG("Initialize UC81XX controller with profile %d", type);

	if (p) {
		LOG_HEXDUMP_DBG(p->pwr.data, p->pwr.len, "PWR");
		if (uc81xx_write_array_opt(dev, UC81XX_CMD_PWR, &p->pwr)) {
			return -EIO;
		}

		if (uc81xx_write_array_opt(dev, UC81XX_CMD_BTST,
					   &config->softstart)) {
			return -EIO;
		}

		if (config->quirks->pon_after_softstart) {
			/* UC8151D requires PON command after BTST for proper
			 * power initialization
			 */
			LOG_DBG("Sending PON command after softstart");
			if (uc81xx_write_cmd(dev, UC81XX_CMD_PON, NULL, 0)) {
				return -EIO;
			}

			/* Wait for power stabilization and BUSY_N = HIGH */
			k_sleep(K_MSEC(UC81XX_PON_DELAY));
			uc81xx_busy_wait(dev);

			LOG_DBG("PON command completed");
		}

		/*
		 * Enable LUT overrides if a LUT has been provided by
		 * the user.
		 */
		if (p->lutc.len || p->lutww.len || p->lutkw.len || p->lutwk.len || p->lutkk.len ||
		    p->lutbd.len) {
			LOG_DBG("Using LUT from registers");
			psr |= UC81XX_PSR_REG;
		}
	}

	/* Panel settings, KW mode and soft reset */
	LOG_DBG("PSR: %#hhx", psr);
	if (uc81xx_write_cmd_uint8(dev, UC81XX_CMD_PSR, psr)) {
		return -EIO;
	}

	/* Set panel resolution */
	if (config->quirks->set_tres(dev)) {
		return -EIO;
	}

	/* Set CDI and enable border output */
	if (config->quirks->set_cdi(dev, true)) {
		return -EIO;
	}

	/*
	 * The rest of the configuration is optional and depends on
	 * having profile overrides specified in the device tree.
	 */
	if (!p) {
		return 0;
	}

	if (p->override_ccset && uc81xx_write_cmd_uint8(dev, UC81XX_CMD_CCSET, p->ccset)) {
		return -EIO;
	}

	if (p->override_tsset && uc81xx_write_cmd_uint8(dev, UC81XX_CMD_TSSET, p->tsset)) {
		return -EIO;
	}

	if (uc81xx_write_array_opt(dev, UC81XX_CMD_LUTC, &p->lutc)) {
		return -EIO;
	}

	if (uc81xx_write_array_opt(dev, UC81XX_CMD_LUTWW, &p->lutww)) {
		return -EIO;
	}

	if (uc81xx_write_array_opt(dev, UC81XX_CMD_LUTKW, &p->lutkw)) {
		return -EIO;
	}

	if (uc81xx_write_array_opt(dev, UC81XX_CMD_LUTWK, &p->lutwk)) {
		return -EIO;
	}

	if (uc81xx_write_array_opt(dev, UC81XX_CMD_LUTKK, &p->lutkk)) {
		return -EIO;
	}

	if (uc81xx_write_array_opt(dev, UC81XX_CMD_LUTBD, &p->lutbd)) {
		return -EIO;
	}

	if (p->override_pll) {
		LOG_DBG("PLL: %#hhx", p->pll);
		if (uc81xx_write_cmd_uint8(dev, UC81XX_CMD_PLL, p->pll)) {
			return -EIO;
		}
	}

	if (p->override_vdcs) {
		LOG_DBG("VDCS: %#hhx", p->vdcs);
		if (uc81xx_write_cmd_uint8(dev, UC81XX_CMD_VDCS, p->vdcs)) {
			return -EIO;
		}
	}

	if (p->override_tcon) {
		if (uc81xx_write_cmd_uint8(dev, UC81XX_CMD_TCON, p->tcon)) {
			return -EIO;
		}
	}

	return 0;
}

static int uc81xx_update_display(const struct device *dev)
{
	LOG_DBG("Trigger update sequence");

	/* Turn on: booster, controller, regulators, and sensor. */
	if (uc81xx_write_cmd(dev, UC81XX_CMD_PON, NULL, 0)) {
		return -EIO;
	}

	k_sleep(K_MSEC(UC81XX_PON_DELAY));

	if (uc81xx_write_cmd(dev, UC81XX_CMD_DRF, NULL, 0)) {
		return -EIO;
	}

	k_sleep(K_MSEC(UC81XX_BUSY_DELAY));

	/* Turn on: booster, controller, regulators, and sensor. */
	if (uc81xx_write_cmd(dev, UC81XX_CMD_POF, NULL, 0)) {
		return -EIO;
	}

	const struct uc81xx_config *config = dev->config;
	struct uc81xx_data *data = dev->data;

	if (config->quirks->dtm_swap) {
		/*
		 * UC8253 has an undocumented quirk that swaps DTM1 with DTM2
		 * on every refresh
		 */
		data->phase = (data->phase == UC81XX_PHASE_NORMAL) ? UC81XX_PHASE_SWAPPED
								   : UC81XX_PHASE_NORMAL;
	}

	return 0;
}

#if DT_HAS_COMPAT_STATUS_OKAY(ultrachip_uc8253)

#define UC8253_GRAY_L8(level)  ((level) * UINT8_MAX / (UC8253_GRAY_LEVELS - 1U))
#define UC8253_GRAY_MID(level) ((UC8253_GRAY_L8(level) + UC8253_GRAY_L8((level) + 1U) + 1U) / 2U)

static uint8_t uc81xx_gray_code(const struct uc81xx_config *config, uint8_t luminance)
{
	uint8_t level;

	if (luminance < UC8253_GRAY_MID(0U)) {
		level = 0U;
	} else if (luminance < UC8253_GRAY_MID(1U)) {
		level = 1U;
	} else if (luminance < UC8253_GRAY_MID(2U)) {
		level = 2U;
	} else {
		level = 3U;
	}

	return config->gray_level_planes[level];
}

static void uc81xx_gray_put(uint8_t *plane_hi, uint8_t *plane_lo, size_t offset, uint8_t mask,
			    uint8_t code)
{
	if ((code & BIT(1)) != 0U) {
		plane_hi[offset] |= mask;
	} else {
		plane_hi[offset] &= (uint8_t)~mask;
	}

	if ((code & BIT(0)) != 0U) {
		plane_lo[offset] |= mask;
	} else {
		plane_lo[offset] &= (uint8_t)~mask;
	}
}

static void uc81xx_gray_scatter(const struct device *dev, uint16_t x, uint16_t y,
				const struct display_buffer_descriptor *desc, const uint8_t *buf)
{
	const struct uc81xx_config *config = dev->config;
	const size_t plane_stride = config->width / UC81XX_PIXELS_PER_BYTE;

	for (uint16_t row = 0U; row < desc->height; row++) {
		const uint8_t *src = buf + (size_t)row * desc->pitch;
		const size_t row_offset = (size_t)(y + row) * plane_stride;

		for (uint16_t column = 0U; column < desc->width; column++) {
			const uint16_t pixel_x = x + column;
			const size_t offset = row_offset + pixel_x / UC81XX_PIXELS_PER_BYTE;
			const uint8_t mask = BIT(7U - (pixel_x % UC81XX_PIXELS_PER_BYTE));

			uc81xx_gray_put(config->plane_hi, config->plane_lo, offset, mask,
					uc81xx_gray_code(config, src[column]));
		}
	}
}

static int uc81xx_gray_send_plane(const struct device *dev, uint8_t cmd, const uint8_t *plane)
{
	const struct uc81xx_config *config = dev->config;
	struct display_buffer_descriptor mipi_desc = {
		.buf_size = config->plane_size,
		.width = config->plane_size,
		.pitch = config->plane_size,
		.height = 1U,
	};
	int err;

	uc81xx_busy_wait(dev);

	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config, cmd, NULL, 0U);
	if (err < 0) {
		mipi_dbi_release(config->mipi_dev, &config->dbi_config);
		return err;
	}

	err = mipi_dbi_write_display(config->mipi_dev, &config->dbi_config, plane, &mipi_desc,
				     PIXEL_FORMAT_MONO10);
	mipi_dbi_release(config->mipi_dev, &config->dbi_config);

	return err;
}

static int uc81xx_gray_commit(const struct device *dev)
{
	const struct uc81xx_config *config = dev->config;
	const struct uc81xx_data *data = dev->data;
	const uint8_t plane_hi_cmd =
		data->phase == UC81XX_PHASE_NORMAL ? UC81XX_CMD_DTM1 : UC81XX_CMD_DTM2;
	const uint8_t plane_lo_cmd =
		data->phase == UC81XX_PHASE_NORMAL ? UC81XX_CMD_DTM2 : UC81XX_CMD_DTM1;

	if (uc81xx_set_profile(dev, UC81XX_PROFILE_GRAY)) {
		return -EIO;
	}

	if (uc81xx_gray_send_plane(dev, plane_hi_cmd, config->plane_hi) ||
	    uc81xx_gray_send_plane(dev, plane_lo_cmd, config->plane_lo)) {
		return -EIO;
	}

	return uc81xx_update_display(dev);
}

static int uc81xx_write_gray(const struct device *dev, uint16_t x, uint16_t y,
			     const struct display_buffer_descriptor *desc, const void *buf)
{
	const struct uc81xx_config *config = dev->config;
	const struct uc81xx_data *data = dev->data;
	size_t needed;

	if (buf == NULL || desc->width == 0U || desc->height == 0U || desc->pitch < desc->width) {
		LOG_ERR("Invalid grayscale buffer descriptor");
		return -EINVAL;
	}

	if ((uint32_t)x + desc->width > config->width ||
	    (uint32_t)y + desc->height > config->height) {
		LOG_ERR("Position out of bounds");
		return -EINVAL;
	}

	needed = (size_t)desc->pitch * (desc->height - 1U) + desc->width;
	if (desc->buf_size < needed) {
		LOG_ERR("Buffer size too small");
		return -EINVAL;
	}

	uc81xx_gray_scatter(dev, x, y, desc, buf);

	if (data->blanking_on || desc->frame_incomplete) {
		return 0;
	}

	return uc81xx_gray_commit(dev);
}

#endif /* DT_HAS_COMPAT_STATUS_OKAY(ultrachip_uc8253) */

static int uc81xx_blanking_off(const struct device *dev)
{
#if DT_HAS_COMPAT_STATUS_OKAY(ultrachip_uc8253)
	const struct uc81xx_config *config = dev->config;
#endif
	struct uc81xx_data *data = dev->data;
	int err;

	if (data->blanking_on) {
		/* Update EPD panel in the configured mode. */
#if DT_HAS_COMPAT_STATUS_OKAY(ultrachip_uc8253)
		err = config->grayscale ? uc81xx_gray_commit(dev) : uc81xx_update_display(dev);
#else
		err = uc81xx_update_display(dev);
#endif
		if (err) {
			return -EIO;
		}
	}

	data->blanking_on = false;

	return 0;
}

static int uc81xx_blanking_on(const struct device *dev)
{
	const struct uc81xx_config *config = dev->config;
	struct uc81xx_data *data = dev->data;

	if (!data->blanking_on) {
		if (uc81xx_set_profile(dev, config->grayscale ? UC81XX_PROFILE_GRAY
							      : UC81XX_PROFILE_FULL)) {
			return -EIO;
		}
	}

	data->blanking_on = true;

	return 0;
}

static int uc81xx_write(const struct device *dev, const uint16_t x, const uint16_t y,
			const struct display_buffer_descriptor *desc,
			const void *buf)
{
	const struct uc81xx_config *config = dev->config;
	struct uc81xx_data *data = dev->data;
	uint16_t x_end_idx;
	uint16_t y_end_idx;
	size_t buf_len;

	LOG_DBG("x %u, y %u, height %u, width %u, pitch %u",
		x, y, desc->height, desc->width, desc->pitch);

#if DT_HAS_COMPAT_STATUS_OKAY(ultrachip_uc8253)
	if (config->grayscale) {
		return uc81xx_write_gray(dev, x, y, desc, buf);
	}
#endif

	x_end_idx = x + desc->width - 1U;
	y_end_idx = y + desc->height - 1U;
	buf_len = desc->width * desc->height / UC81XX_PIXELS_PER_BYTE;

	__ASSERT(desc->width == desc->pitch, "Non-contiguous display buffers are not supported");
	__ASSERT(buf != NULL, "Buffer is not available");
	__ASSERT(desc->buf_size >= buf_len, "Buffer size too small");
	__ASSERT(!(desc->width % UC81XX_PIXELS_PER_BYTE), "Width must be aligned to %u pixels",
		 UC81XX_PIXELS_PER_BYTE);
	__ASSERT(!(x % UC81XX_PIXELS_PER_BYTE), "X must be aligned to %u pixels",
		 UC81XX_PIXELS_PER_BYTE);

	if ((y_end_idx > (config->height - 1)) ||
	    (x_end_idx > (config->width - 1))) {
		LOG_ERR("Position out of bounds");
		return -EINVAL;
	}

	if (!data->blanking_on) {
		/* Blanking isn't on, so this is a partial
		 * refresh. Request the partial profile if it
		 * exists. If a partial profile hasn't been provided,
		 * we continue to use the full refresh profile. Note
		 * that the controller still only scans a partial
		 * window.
		 *
		 * This operation becomes a no-op if the profile is
		 * already active
		 */
		if (uc81xx_have_profile(dev, UC81XX_PROFILE_PARTIAL) &&
		    uc81xx_set_profile(dev, UC81XX_PROFILE_PARTIAL)) {
			return -EIO;
		}
	}

	if (uc81xx_write_cmd(dev, UC81XX_CMD_PTIN, NULL, 0)) {
		return -EIO;
	}

	if (config->quirks->set_ptl(dev, x, y, x_end_idx, y_end_idx, desc)) {
		return -EIO;
	}

	if (uc81xx_write_cmd(dev,
			     data->phase == UC81XX_PHASE_NORMAL ? UC81XX_CMD_DTM2 : UC81XX_CMD_DTM1,
			     (const uint8_t *)buf, buf_len)) {
		return -EIO;
	}

	/* Update the display */
	if (data->blanking_on == false) {
		/* Disable border output */
		if (config->quirks->set_cdi(dev, false)) {
			return -EIO;
		}

		if (uc81xx_update_display(dev)) {
			return -EIO;
		}

		/* Enable border output */
		if (config->quirks->set_cdi(dev, true)) {
			return -EIO;
		}
	}

	if (!config->quirks->auto_copy) {
		/* Some controllers don't copy the new data to the old
		 * data buffer on refresh. Do that manually here if
		 * needed.
		 */
		if (uc81xx_write_cmd(dev,
				     data->phase == UC81XX_PHASE_NORMAL ? UC81XX_CMD_DTM1
									: UC81XX_CMD_DTM2,
				     (const uint8_t *)buf, buf_len)) {
			return -EIO;
		}
	}

	if (uc81xx_write_cmd(dev, UC81XX_CMD_PTOUT, NULL, 0)) {
		return -EIO;
	}

	return 0;
}

static void uc81xx_get_capabilities(const struct device *dev,
				    struct display_capabilities *caps)
{
	const struct uc81xx_config *config = dev->config;

	memset(caps, 0, sizeof(struct display_capabilities));
	caps->x_resolution = config->width;
	caps->y_resolution = config->height;

	if (config->grayscale) {
		caps->supported_pixel_formats = PIXEL_FORMAT_L_8;
		caps->current_pixel_format = PIXEL_FORMAT_L_8;
		caps->screen_info = SCREEN_INFO_EPD;
	} else {
		caps->supported_pixel_formats = PIXEL_FORMAT_MONO01;
		caps->current_pixel_format = PIXEL_FORMAT_MONO01;
		caps->screen_info = SCREEN_INFO_MONO_MSB_FIRST | SCREEN_INFO_EPD;
	}
}

static int uc81xx_set_pixel_format(const struct device *dev,
				   const enum display_pixel_format pf)
{
	const struct uc81xx_config *config = dev->config;
	const enum display_pixel_format expected =
		config->grayscale ? PIXEL_FORMAT_L_8 : PIXEL_FORMAT_MONO01;

	if (pf == expected) {
		return 0;
	}

	LOG_ERR("not supported");
	return -ENOTSUP;
}

static int uc81xx_clear_and_write_buffer(const struct device *dev,
					 uint8_t pattern, bool update)
{
	const struct uc81xx_config *config = dev->config;
	const int size = config->width * config->height
		/ UC81XX_PIXELS_PER_BYTE;

	if (uc81xx_write_cmd_pattern(dev, UC81XX_CMD_DTM1, pattern, size)) {
		return -EIO;
	}

	if (uc81xx_write_cmd_pattern(dev, UC81XX_CMD_DTM2, pattern, size)) {
		return -EIO;
	}

	if (update == true) {
		if (uc81xx_update_display(dev)) {
			return -EIO;
		}
	}

	return 0;
}

static int uc81xx_controller_init(const struct device *dev)
{
	const struct uc81xx_config *config = dev->config;
	struct uc81xx_data *data = dev->data;

	if (mipi_dbi_reset(config->mipi_dev, UC81XX_RESET_DELAY) < 0) {
		return -EIO;
	}

	k_sleep(K_MSEC(UC81XX_RESET_DELAY));
	uc81xx_busy_wait(dev);

	data->blanking_on = true;
	data->profile = UC81XX_PROFILE_INVALID;
	data->phase = UC81XX_PHASE_NORMAL;

	if (uc81xx_set_profile(dev,
			       config->grayscale ? UC81XX_PROFILE_GRAY : UC81XX_PROFILE_FULL)) {
		return -EIO;
	}

#if DT_HAS_COMPAT_STATUS_OKAY(ultrachip_uc8253)
	if (config->grayscale) {
		const uint8_t white = config->gray_level_planes[UC8253_GRAY_LEVELS - 1U];

		memset(config->plane_hi, (white & BIT(1)) != 0U ? 0xff : 0x00, config->plane_size);
		memset(config->plane_lo, (white & BIT(0)) != 0U ? 0xff : 0x00, config->plane_size);

		if (uc81xx_gray_send_plane(dev, UC81XX_CMD_DTM1, config->plane_hi) ||
		    uc81xx_gray_send_plane(dev, UC81XX_CMD_DTM2, config->plane_lo)) {
			return -EIO;
		}

		return 0;
	}
#endif

	if (uc81xx_clear_and_write_buffer(dev, 0xff, false)) {
		return -EIO;
	}

	return 0;
}

static int uc81xx_init(const struct device *dev)
{
	const struct uc81xx_config *config = dev->config;

	LOG_DBG("");

	if (!device_is_ready(config->mipi_dev)) {
		LOG_ERR("MIPI device not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&config->busy_gpio)) {
		LOG_ERR("Busy GPIO device not ready");
		return -ENODEV;
	}

	if (config->grayscale && !uc81xx_have_profile(dev, UC81XX_PROFILE_GRAY)) {
		LOG_ERR("Grayscale mode requires a gray refresh profile");
		return -EINVAL;
	}

	gpio_pin_configure_dt(&config->busy_gpio, GPIO_INPUT);

	if (config->width > config->quirks->max_width ||
	    config->height > config->quirks->max_height) {
		LOG_ERR("Display size out of range.");
		return -EINVAL;
	}

	return uc81xx_controller_init(dev);
}

#if DT_HAS_COMPAT_STATUS_OKAY(ultrachip_uc8175)
static int uc81xx_set_tres_8(const struct device *dev)
{
	const struct uc81xx_config *config = dev->config;
	const struct uc81xx_tres8 tres = {
		.hres = config->width,
		.vres = config->height,
	};

	LOG_HEXDUMP_DBG(&tres, sizeof(tres), "TRES");

	return uc81xx_write_cmd(dev, UC81XX_CMD_TRES, (const void *)&tres, sizeof(tres));
}

static inline int uc81xx_set_ptl_8(const struct device *dev, uint16_t x, uint16_t y,
				   uint16_t x_end_idx, uint16_t y_end_idx,
				   const struct display_buffer_descriptor *desc)
{
	const struct uc81xx_ptl8 ptl = {
		.hrst = x,
		.hred = x_end_idx,
		.vrst = y,
		.vred = y_end_idx,
		.flags = UC81XX_PTL_FLAG_PT_SCAN,
	};

	/* Setup Partial Window and enable Partial Mode */
	LOG_HEXDUMP_DBG(&ptl, sizeof(ptl), "PTL");

	return uc81xx_write_cmd(dev, UC81XX_CMD_PTL, (const void *)&ptl, sizeof(ptl));
}
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(ultrachip_uc8176) || DT_HAS_COMPAT_STATUS_OKAY(ultrachip_uc8179)
static int uc81xx_set_tres_16(const struct device *dev)
{
	const struct uc81xx_config *config = dev->config;
	const struct uc81xx_tres16 tres = {
		.hres = sys_cpu_to_be16(config->width),
		.vres = sys_cpu_to_be16(config->height),
	};

	LOG_HEXDUMP_DBG(&tres, sizeof(tres), "TRES");

	return uc81xx_write_cmd(dev, UC81XX_CMD_TRES, (const void *)&tres, sizeof(tres));
}

static inline int uc81xx_set_ptl_16(const struct device *dev, uint16_t x, uint16_t y,
				    uint16_t x_end_idx, uint16_t y_end_idx,
				    const struct display_buffer_descriptor *desc)
{
	const struct uc81xx_ptl16 ptl = {
		.hrst = sys_cpu_to_be16(x),
		.hred = sys_cpu_to_be16(x_end_idx),
		.vrst = sys_cpu_to_be16(y),
		.vred = sys_cpu_to_be16(y_end_idx),
		.flags = UC81XX_PTL_FLAG_PT_SCAN,
	};

	/* Setup Partial Window and enable Partial Mode */
	LOG_HEXDUMP_DBG(&ptl, sizeof(ptl), "PTL");

	return uc81xx_write_cmd(dev, UC81XX_CMD_PTL, (const void *)&ptl, sizeof(ptl));
}
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(ultrachip_uc8176)
static int uc8176_set_cdi(const struct device *dev, bool border)
{
	const struct uc81xx_config *config = dev->config;
	const struct uc81xx_data *data = dev->data;
	const struct uc81xx_profile *p = config->profiles[data->profile];

	uint8_t interval = UC8176_CDI_DEFAULT_INTERVAL;

	if (p && p->override_cdi) {
		interval = p->cdi & UC8176_CDI_CDI_MASK;
	}

	/* Border uses LUTKW */
	uint8_t cdi = UC8176_CDI_VBD1 | UC8176_CDI_DDX0 | interval;

	if (!border) {
		/* Floating border */
		cdi |= UC8176_CDI_VBD1 | UC8176_CDI_VBD0;
	}

	LOG_DBG("CDI: %#hhx", cdi);
	return uc81xx_write_cmd_uint8(dev, UC81XX_CMD_CDI, cdi);
}
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(ultrachip_uc8175)
static int uc8175_set_cdi(const struct device *dev, bool border)
{
	const struct uc81xx_config *config = dev->config;
	const struct uc81xx_data *data = dev->data;
	const struct uc81xx_profile *p = config->profiles[data->profile];

	uint8_t interval = UC8175_CDI_DEFAULT_INTERVAL;

	if (p && p->override_cdi) {
		interval = p->cdi & UC8175_CDI_CDI_MASK;
	}

	/* Border uses LUTW */
	uint8_t cdi = UC8176_CDI_VBD1 | UC8176_CDI_DDX0 | interval;

	if (!border) {
		/* Floating border */
		cdi &= GENMASK(5, 0);
	}

	LOG_DBG("CDI: %#hhx", cdi);
	return uc81xx_write_cmd_uint8(dev, UC81XX_CMD_CDI, cdi);
}
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(ultrachip_uc8175)
static const struct uc81xx_quirks uc8175_quirks = {
	.max_width = 80,
	.max_height = 160,

	.auto_copy = false,
	.pon_after_softstart = false,
	.dtm_swap = false,

	.set_cdi = uc8175_set_cdi,
	.set_tres = uc81xx_set_tres_8,
	.set_ptl = uc81xx_set_ptl_8,
};
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(ultrachip_uc8176)
static const struct uc81xx_quirks uc8176_quirks = {
	.max_width = 400,
	.max_height = 300,

	.auto_copy = false,
	.pon_after_softstart = false,
	.dtm_swap = false,

	.set_cdi = uc8176_set_cdi,
	.set_tres = uc81xx_set_tres_16,
	.set_ptl = uc81xx_set_ptl_16,
};
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(ultrachip_uc8151d) || DT_HAS_COMPAT_STATUS_OKAY(ultrachip_uc8253)
static int uc81xx_set_tres_h8v16(const struct device *dev)
{
	const struct uc81xx_config *config = dev->config;
	/* Pass pixel coordinates directly; hardware interprets as byte+bit encoding
	 * See UC8151D datasheet page 22 (TRES command, R61h)
	 */
	const struct uc8151d_tres tres = {
		.hres = config->width,
		.vres = sys_cpu_to_be16(config->height),
	};

	LOG_HEXDUMP_DBG(&tres, sizeof(tres), "TRES");

	return uc81xx_write_cmd(dev, UC81XX_CMD_TRES,
			       (const void *)&tres, sizeof(tres));
}

static int uc81xx_set_ptl_h8v16(const struct device *dev, uint16_t x, uint16_t y,
				uint16_t x_end_idx, uint16_t y_end_idx,
				const struct display_buffer_descriptor *desc)
{
	/* Pass pixel coordinates directly; hardware interprets as byte+bit encoding
	 * See UC8151D datasheet page 26 (Partial Window command, R90h)
	 */
	const struct uc8151d_ptl ptl = {
		.hrst = x & BIT_MASK(8),
		.hred = x_end_idx & BIT_MASK(8),
		.vrst = sys_cpu_to_be16(y & BIT_MASK(9)),
		.vred = sys_cpu_to_be16(y_end_idx & BIT_MASK(9)),
		.pt_scan = UC81XX_PTL_FLAG_PT_SCAN,
	};

	/* Setup Partial Window and enable Partial Mode */
	LOG_HEXDUMP_DBG(&ptl, sizeof(ptl), "PTL");

	return uc81xx_write_cmd(dev, UC81XX_CMD_PTL,
			       (const void *)&ptl, sizeof(ptl));
}

static int uc8151d_set_cdi(const struct device *dev, bool border)
{
	const struct uc81xx_config *config = dev->config;
	const struct uc81xx_data *data = dev->data;
	const struct uc81xx_profile *p = config->profiles[data->profile];
	uint8_t cdi = UC8151D_CDI_DEFAULT;  /* Start with 0xD7 */

	if (!p || !p->override_cdi) {
		/* Use default CDI value if no profile override */
		cdi = UC8151D_CDI_DEFAULT;
	} else {
		/* Keep VBD and DDX bits from default, use profile CDI interval */
		cdi = (UC8151D_CDI_DEFAULT & (UC8151D_CDI_VBD_MASK | UC8151D_CDI_DDX_MASK)) |
		      (p->cdi & UC8151D_CDI_MASK);
	}

	if (border) {
		/* Set VBD to LUTKW for border data */
		cdi = (cdi & ~UC8151D_CDI_VBD_MASK) | UC8151D_CDI_VBD_LUTKW;
	}

	LOG_DBG("CDI: %#hhx", cdi);
	return uc81xx_write_cmd_uint8(dev, UC81XX_CMD_CDI, cdi);
}
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(ultrachip_uc8151d)
static const struct uc81xx_quirks uc8151d_quirks = {
	.max_width = 160,      /* Actual max from datasheet */
	.max_height = 296,     /* Actual max from datasheet */

	.auto_copy = false,    /* Manual copy required */
	.pon_after_softstart = true,
	.dtm_swap = false,

	.set_cdi = uc8151d_set_cdi,
	.set_tres = uc81xx_set_tres_h8v16,
	.set_ptl = uc81xx_set_ptl_h8v16,
};
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(ultrachip_uc8253)
static const struct uc81xx_quirks uc8253_quirks = {
	.max_width = 240,
	.max_height = 480,

	.auto_copy = false,
	.pon_after_softstart = false,
	.dtm_swap = true,

	.set_cdi = uc8151d_set_cdi,
	.set_tres = uc81xx_set_tres_h8v16,
	.set_ptl = uc81xx_set_ptl_h8v16,
};
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(ultrachip_uc8179)
static int uc8179_set_cdi(const struct device *dev, bool border)
{
	const struct uc81xx_config *config = dev->config;
	const struct uc81xx_data *data = dev->data;
	const struct uc81xx_profile *p = config->profiles[data->profile];

	uint8_t interval = UC8179_CDI_DEFAULT_INTERVAL;

	if (p && p->override_cdi) {
		interval = p->cdi & UC8179_CDI_CDI_MASK;
	}

	/* Border uses LUTKW, force NEW->OLD auto-copy and NEW/OLD KW operation */
	uint8_t cdi[UC8179_CDI_REG_LENGTH] = {
		UC8179_CDI_BDV1 | UC8179_CDI_N2OCP | UC8179_CDI_DDX0,
		interval,
	};

	if (!border) {
		cdi[UC8179_CDI_BDZ_DDX_IDX] |= UC8179_CDI_BDZ;
	}

	LOG_HEXDUMP_DBG(cdi, sizeof(cdi), "CDI");
	return uc81xx_write_cmd(dev, UC81XX_CMD_CDI, cdi, sizeof(cdi));
}

static const struct uc81xx_quirks uc8179_quirks = {
	.max_width = 800,
	.max_height = 600,

	.auto_copy = true,
	.pon_after_softstart = false,
	.dtm_swap = false,

	.set_cdi = uc8179_set_cdi,
	.set_tres = uc81xx_set_tres_16,
	.set_ptl = uc81xx_set_ptl_16,
};
#endif

static DEVICE_API(display, uc81xx_driver_api) = {
	.blanking_on = uc81xx_blanking_on,
	.blanking_off = uc81xx_blanking_off,
	.write = uc81xx_write,
	.get_capabilities = uc81xx_get_capabilities,
	.set_pixel_format = uc81xx_set_pixel_format,
};

#define UC81XX_MAKE_ARRAY_OPT(n, p)					\
	static uint8_t data_ ## n ## _ ## p[] = DT_PROP_OR(n, p, {})

#define UC81XX_MAKE_ARRAY(n, p)						\
	static uint8_t data_ ## n ## _ ## p[] = DT_PROP(n, p)

#define UC81XX_ASSIGN_ARRAY(n, p)					\
	{								\
		.data = data_ ## n ## _ ## p,				\
		.len = sizeof(data_ ## n ## _ ## p),			\
	}

#define UC81XX_PROFILE(n)                                                                          \
	UC81XX_MAKE_ARRAY_OPT(n, pwr);                                                             \
	UC81XX_MAKE_ARRAY_OPT(n, lutc);                                                            \
	UC81XX_MAKE_ARRAY_OPT(n, lutww);                                                           \
	UC81XX_MAKE_ARRAY_OPT(n, lutkw);                                                           \
	UC81XX_MAKE_ARRAY_OPT(n, lutwk);                                                           \
	UC81XX_MAKE_ARRAY_OPT(n, lutkk);                                                           \
	UC81XX_MAKE_ARRAY_OPT(n, lutbd);                                                           \
                                                                                                   \
	static const struct uc81xx_profile uc81xx_profile_##n = {                                  \
		.pwr = UC81XX_ASSIGN_ARRAY(n, pwr),                                                \
		.cdi = DT_PROP_OR(n, cdi, 0),                                                      \
		.override_cdi = DT_NODE_HAS_PROP(n, cdi),                                          \
		.tcon = DT_PROP_OR(n, tcon, 0),                                                    \
		.override_tcon = DT_NODE_HAS_PROP(n, tcon),                                        \
		.pll = DT_PROP_OR(n, pll, 0),                                                      \
		.override_pll = DT_NODE_HAS_PROP(n, pll),                                          \
		.vdcs = DT_PROP_OR(n, vdcs, 0),                                                    \
		.override_vdcs = DT_NODE_HAS_PROP(n, vdcs),                                        \
		.ccset = DT_PROP_OR(n, ccset, 0),                                                  \
		.override_ccset = DT_NODE_HAS_PROP(n, ccset),                                      \
		.tsset = DT_PROP_OR(n, tsset, 0),                                                  \
		.override_tsset = DT_NODE_HAS_PROP(n, tsset),                                      \
                                                                                                   \
		.lutc = UC81XX_ASSIGN_ARRAY(n, lutc),                                              \
		.lutww = UC81XX_ASSIGN_ARRAY(n, lutww),                                            \
		.lutkw = UC81XX_ASSIGN_ARRAY(n, lutkw),                                            \
		.lutwk = UC81XX_ASSIGN_ARRAY(n, lutwk),                                            \
		.lutkk = UC81XX_ASSIGN_ARRAY(n, lutkk),                                            \
		.lutbd = UC81XX_ASSIGN_ARRAY(n, lutbd),                                            \
	};

#define _UC81XX_PROFILE_PTR(n) &uc81xx_profile_ ## n

#define UC81XX_PROFILE_PTR(n)						\
	COND_CODE_1(DT_NODE_EXISTS(n),					\
		    (_UC81XX_PROFILE_PTR(n)),				\
		    NULL)

#define UC81XX_NODE_IS_UC8253(n) DT_NODE_HAS_COMPAT(n, ultrachip_uc8253)

#define UC81XX_GRAYSCALE(n) COND_CODE_1(UC81XX_NODE_IS_UC8253(n), (DT_PROP(n, grayscale)), (0))

#define UC81XX_GRAY_LEVEL_PLANE(n, i)                                                              \
	COND_CODE_1(UC81XX_NODE_IS_UC8253(n),				\
		    (DT_PROP_BY_IDX(n, gray_level_planes, i)), (i))

#define UC81XX_GRAY_PLANE_SIZE(n) (DT_PROP(n, width) * DT_PROP(n, height) / UC81XX_PIXELS_PER_BYTE)

#define UC81XX_GRAY_PLANE_SUM(n)                                                                   \
	(UC81XX_GRAY_LEVEL_PLANE(n, 0) + UC81XX_GRAY_LEVEL_PLANE(n, 1) +                           \
	 UC81XX_GRAY_LEVEL_PLANE(n, 2) + UC81XX_GRAY_LEVEL_PLANE(n, 3))

#define UC81XX_GRAY_PLANE_SUMSQ(n)                                                                 \
	(UC81XX_GRAY_LEVEL_PLANE(n, 0) * UC81XX_GRAY_LEVEL_PLANE(n, 0) +                           \
	 UC81XX_GRAY_LEVEL_PLANE(n, 1) * UC81XX_GRAY_LEVEL_PLANE(n, 1) +                           \
	 UC81XX_GRAY_LEVEL_PLANE(n, 2) * UC81XX_GRAY_LEVEL_PLANE(n, 2) +                           \
	 UC81XX_GRAY_LEVEL_PLANE(n, 3) * UC81XX_GRAY_LEVEL_PLANE(n, 3))

#define UC81XX_GRAY_MAPPING_VALIDATE(n)                                                            \
	COND_CODE_1(UC81XX_NODE_IS_UC8253(n),				\
		    (BUILD_ASSERT(DT_PROP_LEN(n, gray_level_planes) == UC8253_GRAY_LEVELS, \
				  "gray-level-planes must contain four values");	\
		     BUILD_ASSERT(UC81XX_GRAY_PLANE_SUM(n) == 6 &&		\
				  UC81XX_GRAY_PLANE_SUMSQ(n) == 14,		\
				  "gray-level-planes must list 0..3 exactly once");), ())

#define UC81XX_GRAY_WIDTH_VALIDATE(n)                                                              \
	COND_CODE_1(UC81XX_GRAYSCALE(n),				\
		    (BUILD_ASSERT(DT_PROP(n, width) % UC81XX_PIXELS_PER_BYTE == 0, \
				  "UC8253 grayscale width must be a multiple of 8");), ())

#define UC81XX_GRAY_BUFFERS(n)                                                                     \
	COND_CODE_1(UC81XX_GRAYSCALE(n),				\
		    (static uint8_t uc81xx_plane_hi_ ## n[UC81XX_GRAY_PLANE_SIZE(n)]; \
		     static uint8_t uc81xx_plane_lo_ ## n[UC81XX_GRAY_PLANE_SIZE(n)];), ())

#define UC81XX_GRAY_PLANE_PTR(n, plane)                                                            \
	COND_CODE_1(UC81XX_GRAYSCALE(n), (uc81xx_plane_ ## plane ## _ ## n), (NULL))

#define UC81XX_DEFINE(n, quirks_ptr)                                                               \
	UC81XX_MAKE_ARRAY_OPT(n, softstart);                                                       \
                                                                                                   \
	DT_FOREACH_CHILD(n, UC81XX_PROFILE);                                                       \
                                                                                                   \
	UC81XX_GRAY_MAPPING_VALIDATE(n);                                                           \
	UC81XX_GRAY_WIDTH_VALIDATE(n);                                                             \
	UC81XX_GRAY_BUFFERS(n);                                                                    \
                                                                                                   \
	static const struct uc81xx_config uc81xx_cfg_##n = {                                       \
		.quirks = quirks_ptr,                                                              \
		.mipi_dev = DEVICE_DT_GET(DT_PARENT(n)),                                           \
		.dbi_config =                                                                      \
			{                                                                          \
				.mode = MIPI_DBI_MODE_SPI_4WIRE,                                   \
				.config = MIPI_DBI_SPI_CONFIG_DT(                                  \
					n, SPI_OP_MODE_CONTROLLER | SPI_LOCK_ON | SPI_WORD_SET(8), \
					0),                                                        \
			},                                                                         \
		.busy_gpio = GPIO_DT_SPEC_GET(n, busy_gpios),                                      \
                                                                                                   \
		.height = DT_PROP(n, height),                                                      \
		.width = DT_PROP(n, width),                                                        \
		.grayscale = UC81XX_GRAYSCALE(n),                                                  \
		.gray_level_planes =                                                               \
			{                                                                          \
				UC81XX_GRAY_LEVEL_PLANE(n, 0),                                     \
				UC81XX_GRAY_LEVEL_PLANE(n, 1),                                     \
				UC81XX_GRAY_LEVEL_PLANE(n, 2),                                     \
				UC81XX_GRAY_LEVEL_PLANE(n, 3),                                     \
			},                                                                         \
		.plane_size = UC81XX_GRAY_PLANE_SIZE(n),                                           \
		.plane_hi = UC81XX_GRAY_PLANE_PTR(n, hi),                                          \
		.plane_lo = UC81XX_GRAY_PLANE_PTR(n, lo),                                          \
                                                                                                   \
		.softstart = UC81XX_ASSIGN_ARRAY(n, softstart),                                    \
                                                                                                   \
		.profiles =                                                                        \
			{                                                                          \
				[UC81XX_PROFILE_FULL] = UC81XX_PROFILE_PTR(DT_CHILD(n, full)),     \
				[UC81XX_PROFILE_PARTIAL] =                                         \
					UC81XX_PROFILE_PTR(DT_CHILD(n, partial)),                  \
				[UC81XX_PROFILE_GRAY] = UC81XX_PROFILE_PTR(DT_CHILD(n, gray)),     \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	static struct uc81xx_data uc81xx_data_##n = {};                                            \
                                                                                                   \
	DEVICE_DT_DEFINE(n, uc81xx_init, NULL, &uc81xx_data_##n, &uc81xx_cfg_##n, POST_KERNEL,     \
			 CONFIG_DISPLAY_INIT_PRIORITY, &uc81xx_driver_api);

DT_FOREACH_STATUS_OKAY_VARGS(ultrachip_uc8175, UC81XX_DEFINE,
			     &uc8175_quirks);

DT_FOREACH_STATUS_OKAY_VARGS(ultrachip_uc8176, UC81XX_DEFINE,
			     &uc8176_quirks);

DT_FOREACH_STATUS_OKAY_VARGS(ultrachip_uc8151d, UC81XX_DEFINE,
			     &uc8151d_quirks);

DT_FOREACH_STATUS_OKAY_VARGS(ultrachip_uc8253, UC81XX_DEFINE,
			     &uc8253_quirks);

DT_FOREACH_STATUS_OKAY_VARGS(ultrachip_uc8179, UC81XX_DEFINE,
			     &uc8179_quirks);
