/*
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
 */

#define UC81XX_PIXELS_PER_BYTE		8U

struct uc81xx_dt_array {
	uint8_t *data;
	uint8_t len;
};

enum uc81xx_profile_type {
	UC81XX_PROFILE_FULL = 0,
	UC81XX_PROFILE_PARTIAL,
	UC81XX_NUM_PROFILES,
	UC81XX_PROFILE_INVALID = UC81XX_NUM_PROFILES,
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

	struct uc81xx_dt_array softstart;

	const struct uc81xx_profile *profiles[UC81XX_NUM_PROFILES];
};

struct uc81xx_data {
	bool blanking_on;
	enum uc81xx_profile_type profile;
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
	struct display_buffer_descriptor mipi_desc;
	int err;
	uint8_t data[64];

	uc81xx_busy_wait(dev);

	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config,
				     cmd, NULL, 0);
	if (err < 0) {
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
			goto out;
		}

		len -= mipi_desc.buf_size;
	}

out:
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

		/*
		 * Enable LUT overrides if a LUT has been provided by
		 * the user.
		 */
		if (p->lutc.len || p->lutww.len || p->lutkw.len ||
		    p->lutwk.len || p->lutbd.len) {
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

	return 0;
}

static int uc81xx_blanking_off(const struct device *dev)
{
	struct uc81xx_data *data = dev->data;

	if (data->blanking_on) {
		/* Update EPD panel in normal mode */
		if (uc81xx_update_display(dev)) {
			return -EIO;
		}
	}

	data->blanking_on = false;

	return 0;
}

static int uc81xx_blanking_on(const struct device *dev)
{
	struct uc81xx_data *data = dev->data;

	if (!data->blanking_on) {
		if (uc81xx_set_profile(dev, UC81XX_PROFILE_FULL)) {
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

	uint16_t x_end_idx = x + desc->width - 1;
	uint16_t y_end_idx = y + desc->height - 1;
	size_t buf_len;
	const uint8_t back_buffer = data->blanking_on ?
		UC81XX_CMD_DTM1 : UC81XX_CMD_DTM2;

	LOG_DBG("x %u, y %u, height %u, width %u, pitch %u",
		x, y, desc->height, desc->width, desc->pitch);

	buf_len = MIN(desc->buf_size,
		      desc->height * desc->width / UC81XX_PIXELS_PER_BYTE);
	__ASSERT(desc->width <= desc->pitch, "Pitch is smaller than width");
	__ASSERT(buf != NULL, "Buffer is not available");
	__ASSERT(buf_len != 0U, "Buffer of length zero");
	__ASSERT(!(desc->width % UC81XX_PIXELS_PER_BYTE),
		 "Buffer width not multiple of %d", UC81XX_PIXELS_PER_BYTE);

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

	if (uc81xx_write_cmd(dev, UC81XX_CMD_DTM2, (uint8_t *)buf, buf_len)) {
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

		if (config->quirks->set_ptl(dev, x, y, x_end_idx, y_end_idx, desc)) {
			return -EIO;
		}

		if (uc81xx_write_cmd(dev, back_buffer,
				     (uint8_t *)buf, buf_len)) {
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
	caps->supported_pixel_formats = PIXEL_FORMAT_MONO10;
	caps->current_pixel_format = PIXEL_FORMAT_MONO10;
	caps->screen_info = SCREEN_INFO_MONO_MSB_FIRST | SCREEN_INFO_EPD;
}

static int uc81xx_set_pixel_format(const struct device *dev,
				   const enum display_pixel_format pf)
{
	if (pf == PIXEL_FORMAT_MONO10) {
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

	mipi_dbi_reset(config->mipi_dev, UC81XX_RESET_DELAY);
	k_sleep(K_MSEC(UC81XX_RESET_DELAY));
	uc81xx_busy_wait(dev);

	data->blanking_on = true;
	data->profile = UC81XX_PROFILE_INVALID;

	if (uc81xx_set_profile(dev, UC81XX_PROFILE_FULL)) {
		return -EIO;
	}

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
	LOG_HEXDUMP_DBG(&ptl, sizeof(ptl), "ptl");

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
	LOG_HEXDUMP_DBG(&ptl, sizeof(ptl), "ptl");

	return uc81xx_write_cmd(dev, UC81XX_CMD_PTL, (const void *)&ptl, sizeof(ptl));
}
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(ultrachip_uc8175) || DT_HAS_COMPAT_STATUS_OKAY(ultrachip_uc8176)
static int uc8176_set_cdi(const struct device *dev, bool border)
{
	const struct uc81xx_config *config = dev->config;
	const struct uc81xx_data *data = dev->data;
	const struct uc81xx_profile *p = config->profiles[data->profile];
	uint8_t cdi = UC8176_CDI_VBD1 | UC8176_CDI_DDX0 |
		(p ? (p->cdi & UC8176_CDI_CDI_MASK) : 0);

	if (!p || !p->override_cdi) {
		return 0;
	}

	if (!border) {
		/* Floating border */
		cdi |= UC8176_CDI_VBD1 | UC8176_CDI_VBD0;
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

	.set_cdi = uc8176_set_cdi,
	.set_tres = uc81xx_set_tres_8,
	.set_ptl = uc81xx_set_ptl_8,
};
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(ultrachip_uc8176)
static const struct uc81xx_quirks uc8176_quirks = {
	.max_width = 400,
	.max_height = 300,

	.auto_copy = false,

	.set_cdi = uc8176_set_cdi,
	.set_tres = uc81xx_set_tres_16,
	.set_ptl = uc81xx_set_ptl_16,
};
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(ultrachip_uc8179)
static int uc8179_set_cdi(const struct device *dev, bool border)
{
	const struct uc81xx_config *config = dev->config;
	const struct uc81xx_data *data = dev->data;
	const struct uc81xx_profile *p = config->profiles[data->profile];
	uint8_t cdi[UC8179_CDI_REG_LENGTH] = {
		UC8179_CDI_BDV1 | UC8179_CDI_N2OCP | UC8179_CDI_DDX0,
		p ? p->cdi : 0,
	};

	if (!p || !p->override_cdi) {
		return 0;
	}

	cdi[UC8179_CDI_BDZ_DDX_IDX] |= border ? 0 : UC8179_CDI_BDZ;

	LOG_HEXDUMP_DBG(cdi, sizeof(cdi), "CDI");
	return uc81xx_write_cmd(dev, UC81XX_CMD_CDI, cdi, sizeof(cdi));
}

static const struct uc81xx_quirks uc8179_quirks = {
	.max_width = 800,
	.max_height = 600,

	.auto_copy = true,

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

#define UC81XX_PROFILE(n)						\
	UC81XX_MAKE_ARRAY_OPT(n, pwr);					\
	UC81XX_MAKE_ARRAY_OPT(n, lutc);					\
	UC81XX_MAKE_ARRAY_OPT(n, lutww);				\
	UC81XX_MAKE_ARRAY_OPT(n, lutkw);				\
	UC81XX_MAKE_ARRAY_OPT(n, lutwk);				\
	UC81XX_MAKE_ARRAY_OPT(n, lutkk);				\
	UC81XX_MAKE_ARRAY_OPT(n, lutbd);				\
									\
	static const struct uc81xx_profile uc81xx_profile_ ## n = {	\
		.pwr = UC81XX_ASSIGN_ARRAY(n, pwr),			\
		.cdi = DT_PROP_OR(n, cdi, 0),				\
		.override_cdi = DT_NODE_HAS_PROP(n, cdi),		\
		.tcon = DT_PROP_OR(n, tcon, 0),				\
		.override_tcon = DT_NODE_HAS_PROP(n, tcon),		\
		.pll = DT_PROP_OR(n, pll, 0),				\
		.override_pll = DT_NODE_HAS_PROP(n, pll),		\
		.vdcs = DT_PROP_OR(n, vdcs, 0),				\
		.override_vdcs = DT_NODE_HAS_PROP(n, vdcs),		\
									\
		.lutc = UC81XX_ASSIGN_ARRAY(n, lutc),			\
		.lutww = UC81XX_ASSIGN_ARRAY(n, lutww),			\
		.lutkw = UC81XX_ASSIGN_ARRAY(n, lutkw),			\
		.lutwk = UC81XX_ASSIGN_ARRAY(n, lutwk),			\
		.lutkk = UC81XX_ASSIGN_ARRAY(n, lutkk),			\
		.lutbd = UC81XX_ASSIGN_ARRAY(n, lutbd),			\
	};

#define _UC81XX_PROFILE_PTR(n) &uc81xx_profile_ ## n

#define UC81XX_PROFILE_PTR(n)						\
	COND_CODE_1(DT_NODE_EXISTS(n),					\
		    (_UC81XX_PROFILE_PTR(n)),				\
		    NULL)

#define UC81XX_DEFINE(n, quirks_ptr)					\
	UC81XX_MAKE_ARRAY_OPT(n, softstart);				\
									\
	DT_FOREACH_CHILD(n, UC81XX_PROFILE);				\
									\
	static const struct uc81xx_config uc81xx_cfg_ ## n = {		\
		.quirks = quirks_ptr,					\
		.mipi_dev = DEVICE_DT_GET(DT_PARENT(n)),                \
		.dbi_config = {                                         \
			.mode = MIPI_DBI_MODE_SPI_4WIRE,                \
			.config = MIPI_DBI_SPI_CONFIG_DT(n,             \
					SPI_OP_MODE_MASTER |            \
					SPI_LOCK_ON | SPI_WORD_SET(8),  \
					0),                             \
		},                                                      \
		.busy_gpio = GPIO_DT_SPEC_GET(n, busy_gpios),		\
									\
		.height = DT_PROP(n, height),				\
		.width = DT_PROP(n, width),				\
									\
		.softstart = UC81XX_ASSIGN_ARRAY(n, softstart),		\
									\
		.profiles = {						\
			[UC81XX_PROFILE_FULL] =				\
				UC81XX_PROFILE_PTR(DT_CHILD(n, full)),	\
			[UC81XX_PROFILE_PARTIAL] =			\
				UC81XX_PROFILE_PTR(DT_CHILD(n, partial)), \
		},							\
	};								\
									\
	static struct uc81xx_data uc81xx_data_##n = {};			\
									\
	DEVICE_DT_DEFINE(n, uc81xx_init, NULL,				\
			 &uc81xx_data_ ## n,				\
			 &uc81xx_cfg_ ## n,				\
			 POST_KERNEL,					\
			 CONFIG_DISPLAY_INIT_PRIORITY,			\
			 &uc81xx_driver_api);

DT_FOREACH_STATUS_OKAY_VARGS(ultrachip_uc8175, UC81XX_DEFINE,
			     &uc8175_quirks);

DT_FOREACH_STATUS_OKAY_VARGS(ultrachip_uc8176, UC81XX_DEFINE,
			     &uc8176_quirks);

DT_FOREACH_STATUS_OKAY_VARGS(ultrachip_uc8179, UC81XX_DEFINE,
			     &uc8179_quirks);
