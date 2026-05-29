/*
 * Copyright (c) 2025 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(st730x, CONFIG_DISPLAY_LOG_LEVEL);

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mipi_dbi.h>
#include <zephyr/kernel.h>

/* Registers */
#define ST730X_SLEEP_IN			0x10
#define ST730X_SLEEP_OUT		0x11
#define ST730X_SET_NORMAL_DISPLAY	0x20
#define ST730X_SET_REVERSE_DISPLAY	0x21
#define ST730X_DISPLAY_OFF		0x28
#define ST730X_DISPLAY_ON		0x29
#define ST730X_SET_COLUMN_ADDR		0x2A
#define ST730X_SET_ROW_ADDR		0x2B
#define ST730X_WRITE			0x2C
#define ST730X_READ			0x2E
#define ST730X_TEARING_OUT		0x35
#define ST730X_TEARING_OUT_VBLANK	0x00
#define ST730X_TEARING_OUT_VHBLANK	0x01
#define ST730X_MADCTL			0x36
#define ST730X_HPM			0x38
#define ST730X_LPM			0x39
#define ST730X_DTFORM			0x3A
#define ST730X_DTFORM_4W_24B		0x10
#define ST730X_DTFORM_3W_24B		0x11
#define ST730X_GATESET			0xB0
#define ST730X_FIRSTGATE		0xB1
#define ST730X_FRAMERATE		0xB2
#define ST730X_HPM_GATE_WAVEFORM	0xB3
#define ST730X_LPM_GATE_WAVEFORM	0xB4
#define ST730X_SOURCE_EQ_EN		0xB7
#define ST730X_SOURCE_EQ_EN_ENABLE	0x13
#define ST730X_SOURCE_EQ_EN_DISABLE	0x03
#define ST730X_PNLSET			0xB8
#define ST730X_GAMAMS			0xB9
#define ST730X_CLEAR_RAM		0xBB
#define ST730X_CLEAR_RAM_ENABLE		0xCF
#define ST730X_CLEAR_RAM_DISABLE	0x4F
#define ST730X_GAMAMS_MONO		0x20
#define ST730X_GAMAMS_4GS		0x00
#define ST730X_GATE_VOLTAGE		0xC0
#define ST730X_VSH			0xC1
#define ST730X_VSL			0xC2
#define ST730X_VSHN			0xC4
#define ST730X_VSLN			0xC5
#define ST730X_VSHLSEL			0xC9
#define ST730X_AUTOPWRDOWN		0xD0
#define ST730X_AUTOPWRDOWN_ON		0xFF
#define ST730X_AUTOPWRDOWN_OFF		0x7F
#define ST730X_BOOSTER_EN		0xD1
#define ST730X_BOOSTER_EN_ENABLE	0x01
#define ST730X_BOOSTER_EN_DISABLE	0x00
#define ST730X_NVM_LOAD			0xD6
#define ST730X_OSC_SETTINGS		0xD8
#define ST730X_OSC_SETTINGS_BYTE2	0xE9

#define ST730X_HPM_GATE_WAVEFORM_LEN 10
#define ST730X_LPM_GATE_WAVEFORM_LEN 8

/* Pixels per byte and pixels per x and y address */
#define ST730X_PPB 8
#define ST730X_PPXA 12
#define ST730X_PPYA 2

#define ST730X_RESET_DELAY 100
#define ST730X_SLEEP_DELAY 100

#ifdef CONFIG_ST730X_POWERMODE_LOW
#define ST730X_POWER_MODE ST730X_LPM
#else
#define ST730X_POWER_MODE ST730X_HPM
#endif

/* ST730x controllers have an evil data format for b&w
 * where the pixels are ordered at each address as such:
 * p1  p3  p5  p7
 * p2  p4  p6  p8
 */

struct st730x_specific {
	uint8_t column_offset;
};

struct st730x_config {
	const struct device *mipi_dev;
	const struct mipi_dbi_config dbi_config;
	const struct st730x_specific *specifics;
	uint16_t height;
	uint16_t width;
	uint16_t start_line;
	uint16_t start_column;
	uint8_t nvm_load[2];
	uint8_t gate_voltages[2];
	uint8_t vsh[4];
	uint8_t vsl[4];
	uint8_t vshn[4];
	uint8_t vsln[4];
	uint8_t osc_settings;
	uint8_t framerate;
	uint8_t multiplex_ratio;
	uint8_t source_voltage;
	uint8_t remap_value;
	uint8_t panel_settings;
	uint8_t hpm_gate_waveform[ST730X_HPM_GATE_WAVEFORM_LEN];
	uint8_t lpm_gate_waveform[ST730X_LPM_GATE_WAVEFORM_LEN];
	bool color_inversion;
	uint8_t *conversion_buf;
	size_t conversion_buf_size;
};

static int st730x_resume(const struct device *dev)
{
	const struct st730x_config *config = dev->config;
	int err;

	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config, ST730X_SLEEP_OUT,
				     NULL, 0);
	if (err < 0) {
		return err;
	}
	k_msleep(ST730X_SLEEP_DELAY);

	/* Also enable display */
	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config, ST730X_DISPLAY_ON,
				     NULL, 0);
	if (err < 0) {
		return err;
	}

	return mipi_dbi_release(config->mipi_dev, &config->dbi_config);
}

static int st730x_suspend(const struct device *dev)
{
	const struct st730x_config *config = dev->config;
	int err;

	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config, ST730X_SLEEP_IN,
				     NULL, 0);
	if (err < 0) {
		return err;
	}
	k_msleep(ST730X_SLEEP_DELAY);

	return mipi_dbi_release(config->mipi_dev, &config->dbi_config);
}

static inline int st730x_set_hardware_config(const struct device *dev)
{
	const struct st730x_config *config = dev->config;
	int err;
	uint8_t tmp[2];

	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config,
				     ST730X_NVM_LOAD, config->nvm_load, 2);
	if (err < 0) {
		return err;
	}

	tmp[0] = ST730X_BOOSTER_EN_ENABLE;
	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config, ST730X_BOOSTER_EN, tmp,
				     1);
	if (err < 0) {
		return err;
	}

	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config,
				     ST730X_GATE_VOLTAGE, config->gate_voltages, 2);
	if (err < 0) {
		return err;
	}

	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config,
				     ST730X_VSH, config->vsh, 4);
	if (err < 0) {
		return err;
	}

	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config,
				     ST730X_VSL, config->vsl, 4);
	if (err < 0) {
		return err;
	}

	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config,
				     ST730X_VSHN, config->vshn, 4);
	if (err < 0) {
		return err;
	}

	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config,
				     ST730X_VSLN, config->vsln, 4);
	if (err < 0) {
		return err;
	}

	tmp[0] = config->osc_settings;
	tmp[1] = ST730X_OSC_SETTINGS_BYTE2;
	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config,
				     ST730X_OSC_SETTINGS, tmp, 2);
	if (err < 0) {
		return err;
	}

	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config,
				     ST730X_FRAMERATE, &config->framerate, 1);
	if (err < 0) {
		return err;
	}

	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config,
				     ST730X_HPM_GATE_WAVEFORM, config->hpm_gate_waveform,
				     ST730X_HPM_GATE_WAVEFORM_LEN);
	if (err < 0) {
		return err;
	}

	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config,
				     ST730X_LPM_GATE_WAVEFORM, config->lpm_gate_waveform,
				     ST730X_LPM_GATE_WAVEFORM_LEN);
	if (err < 0) {
		return err;
	}

	tmp[0] = ST730X_SOURCE_EQ_EN_ENABLE;
	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config,
				     ST730X_SOURCE_EQ_EN, tmp, 1);
	if (err < 0) {
		return err;
	}

	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config,
				     ST730X_GATESET, &config->multiplex_ratio, 1);
	if (err < 0) {
		return err;
	}

	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config,
				     ST730X_VSHLSEL, &config->source_voltage, 1);
	if (err < 0) {
		return err;
	}

	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config,
				     ST730X_MADCTL, &config->remap_value, 1);
	if (err < 0) {
		return err;
	}

	tmp[0] = ST730X_DTFORM_3W_24B;
	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config,
				     ST730X_DTFORM, tmp, 1);
	if (err < 0) {
		return err;
	}

	tmp[0] = ST730X_GAMAMS_MONO;
	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config,
				     ST730X_GAMAMS, tmp, 1);
	if (err < 0) {
		return err;
	}

	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config,
				     ST730X_PNLSET, &config->panel_settings, 1);
	if (err < 0) {
		return err;
	}

	tmp[0] = ST730X_TEARING_OUT_VBLANK;
	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config,
				     ST730X_TEARING_OUT, tmp, 1);
	if (err < 0) {
		return err;
	}

	tmp[0] = ST730X_AUTOPWRDOWN_ON;
	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config,
				     ST730X_AUTOPWRDOWN, tmp, 1);
	if (err < 0) {
		return err;
	}

	tmp[0] = (config->start_line & 0x100) >> 8;
	tmp[1] = config->start_line & 0xFF;
	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config,
				     ST730X_FIRSTGATE, tmp, 2);
	if (err < 0) {
		return err;
	}

	return mipi_dbi_command_write(config->mipi_dev, &config->dbi_config,
				     ST730X_POWER_MODE, NULL, 0);
}

/* Convert what the conversion buffer can hold to the st730x format */
static int st730x_convert(const struct device *dev, const uint8_t *buf, uint32_t offset,
			       const struct display_buffer_descriptor *desc)
{
	const struct st730x_config *config = dev->config;
	uint32_t vertical_offset = desc->width / ST730X_PPB;
	uint32_t i = 0;
	uint32_t ipos, ipos_zeroed;
	uint32_t max_lines = (config->conversion_buf_size / (desc->width / ST730X_PPB)) & ~0x1;

	if (max_lines < ST730X_PPYA) {
		LOG_ERR("Buffer too small, cannot convert");
		return -EINVAL;
	}

	for (; (offset + i) < desc->height && i < max_lines; i += ST730X_PPYA) {
		ipos = (offset + i) * vertical_offset;
		ipos_zeroed = i * vertical_offset / ST730X_PPYA;
		for (uint32_t j = 0; j < desc->width / ST730X_PPB ; j++) {
			config->conversion_buf[(ipos_zeroed + j) * 2 + 1] =
				(buf[ipos + j + vertical_offset] & 0x80) >> 7
				| (buf[ipos + j] & 0x80) >> 6
				| (buf[ipos + j + vertical_offset] & 0x40) >> 4
				| (buf[ipos + j] & 0x40) >> 3
				| (buf[ipos + j + vertical_offset] & 0x20) >> 1
				| (buf[ipos + j] & 0x20)
				| (buf[ipos + j + vertical_offset] & 0x10) << 2
				| (buf[ipos + j] & 0x10) << 3;
			config->conversion_buf[(ipos_zeroed + j) * 2] =
				(buf[ipos + j + vertical_offset] & 0x8) >> 3
				| (buf[ipos + j] & 0x8) >> 2
				| (buf[ipos + j + vertical_offset] & 0x4)
				| (buf[ipos + j] & 0x4) << 1
				| (buf[ipos + j + vertical_offset] & 0x2) << 3
				| (buf[ipos + j] & 0x2) << 4
				| (buf[ipos + j + vertical_offset] & 0x1) << 6
				| (buf[ipos + j] & 0x1) << 7;
		}
	}

	return i;
}

static int st730x_write(const struct device *dev, const uint16_t x, const uint16_t y,
			 const struct display_buffer_descriptor *desc, const void *buf)
{
	const struct st730x_config *config = dev->config;
	int err;
	size_t buf_len;
	uint32_t processed = 0;
	int i;
	struct display_buffer_descriptor mipi_desc = *desc;
	uint8_t x_start = config->specifics->column_offset + (config->start_column + x)
			  / ST730X_PPXA;
	uint8_t x_end = config->specifics->column_offset
			+ (config->start_column + x + desc->width) / ST730X_PPXA - 1;
	uint8_t x_position[] = {x_start, x_end};
	uint8_t y_position[] = {y / ST730X_PPYA, (y + desc->height) / ST730X_PPYA - 1};

	if (desc->pitch != desc->width) {
		LOG_ERR("Pitch is not width");
		return -EINVAL;
	}

	buf_len = MIN(desc->buf_size, desc->height * desc->width / ST730X_PPB);
	if (buf == NULL || buf_len == 0U) {
		LOG_ERR("Display buffer is not available");
		return -EINVAL;
	}

	if (x % ST730X_PPXA || desc->width % ST730X_PPXA) {
		LOG_ERR("X coordinate and size must be aligned by 12 pixels");
		return -EINVAL;
	}

	if (y % ST730X_PPYA || desc->height % ST730X_PPYA) {
		LOG_ERR("Y coordinate and size must be aligned by 2 pixels");
		return -EINVAL;
	}

	LOG_DBG("x %u, y %u, pitch %u, width %u, height %u, buf_len %u", x, y, desc->pitch,
		desc->width, desc->height, buf_len);

	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config, ST730X_SET_COLUMN_ADDR,
				     x_position, 2);
	if (err < 0) {
		return err;
	}

	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config, ST730X_SET_ROW_ADDR,
				     y_position, 2);
	if (err < 0) {
		return err;
	}

	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config, ST730X_WRITE, NULL, 0);
	if (err < 0) {
		return err;
	}

	while (desc->height > processed) {
		i = st730x_convert(dev, buf, processed, desc);

		if (i < 0) {
			return i;
		}

		mipi_desc.buf_size = i * desc->width / ST730X_PPB;
		mipi_desc.width = desc->width;
		mipi_desc.height = i;

		err = mipi_dbi_write_display(config->mipi_dev, &config->dbi_config,
						config->conversion_buf, &mipi_desc,
						PIXEL_FORMAT_MONO01);
		if (err < 0) {
			return err;
		}
		processed += i;
	}

	return mipi_dbi_release(config->mipi_dev, &config->dbi_config);
}

static void st730x_get_capabilities(const struct device *dev, struct display_capabilities *caps)
{
	const struct st730x_config *config = dev->config;

	memset(caps, 0, sizeof(struct display_capabilities));
	caps->x_resolution = config->width;
	caps->y_resolution = config->height;
	caps->supported_pixel_formats = PIXEL_FORMAT_MONO01;
	caps->current_pixel_format = PIXEL_FORMAT_MONO01;
	caps->screen_info = 0;
}

static int st730x_set_pixel_format(const struct device *dev, const enum display_pixel_format pf)
{
	if (pf == PIXEL_FORMAT_MONO01) {
		return 0;
	}
	LOG_ERR("Unsupported pixel format");

	return -ENOTSUP;
}

static int st730x_init_device(const struct device *dev)
{
	const struct st730x_config *config = dev->config;
	int err;

	err = st730x_suspend(dev);
	if (err < 0) {
		return err;
	}

	err = st730x_set_hardware_config(dev);
	if (err < 0) {
		return err;
	}

	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config,
				     config->color_inversion ? ST730X_SET_REVERSE_DISPLAY
							     : ST730X_SET_NORMAL_DISPLAY,
				     NULL, 0);
	if (err < 0) {
		return err;
	}

	err = st730x_resume(dev);
	if (err < 0) {
		return err;
	}

	return mipi_dbi_release(config->mipi_dev, &config->dbi_config);
}

static int st730x_init(const struct device *dev)
{
	const struct st730x_config *config = dev->config;
	int err;

	LOG_DBG("Initializing device");

	if (!device_is_ready(config->mipi_dev)) {
		LOG_ERR("MIPI Device not ready!");
		return -ENODEV;
	}

	err = mipi_dbi_reset(config->mipi_dev, ST730X_RESET_DELAY);
	if (err < 0) {
		LOG_ERR("Failed to reset device!");
		return err;
	}

	err = st730x_init_device(dev);
	if (err < 0) {
		LOG_ERR("Failed to initialize device! %d", err);
		return err;
	}

	return 0;
}

static DEVICE_API(display, st730x_driver_api) = {
	.blanking_on = st730x_suspend,
	.blanking_off = st730x_resume,
	.write = st730x_write,
	.get_capabilities = st730x_get_capabilities,
	.set_pixel_format = st730x_set_pixel_format,
};

#if DT_HAS_COMPAT_STATUS_OKAY(sitronix_st7305)
static const struct st730x_specific st7305_specifics = {
	.column_offset = 19,
};
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(sitronix_st7306)
static const struct st730x_specific st7306_specifics = {
	.column_offset = 0,
};
#endif

#define ST730X_WORD_SIZE(inst)                                                                     \
	((DT_STRING_UPPER_TOKEN(inst, mipi_mode) == MIPI_DBI_MODE_SPI_4WIRE) ? SPI_WORD_SET(8)     \
									     : SPI_WORD_SET(9))

#define ST730X_CONV_BUFFER_SIZE(node_id)                                                           \
	ROUND_UP(DT_PROP(node_id, width) * CONFIG_ST730X_CONV_BUFFER_LINE_CNT,                     \
		     DT_PROP(node_id, width))

#define ST730X_DEFINE_MIPI(node_id, specifics_ptr)                                                 \
	static uint8_t conversion_buf##node_id[ST730X_CONV_BUFFER_SIZE(node_id)];                  \
	static const struct st730x_config config##node_id = {                                      \
		.mipi_dev = DEVICE_DT_GET(DT_PARENT(node_id)),                                     \
		.dbi_config = MIPI_DBI_CONFIG_DT(                                                  \
			node_id, ST730X_WORD_SIZE(node_id) | SPI_OP_MODE_MASTER, 0),               \
		.height = DT_PROP(node_id, height),                                                \
		.width = DT_PROP(node_id, width),                                                  \
		.start_line = DT_PROP(node_id, start_line),                                        \
		.start_column = DT_PROP(node_id, start_column),                                    \
		.nvm_load = DT_PROP(node_id, nvm_load),                                            \
		.gate_voltages = DT_PROP(node_id, gate_voltages),                                  \
		.vsh = DT_PROP(node_id, vsh),                                                      \
		.vsl = DT_PROP(node_id, vsl),                                                      \
		.vshn = DT_PROP(node_id, vshn),                                                    \
		.vsln = DT_PROP(node_id, vsln),                                                    \
		.osc_settings = DT_PROP(node_id, osc_settings),                                    \
		.framerate = DT_PROP(node_id, framerate),                                          \
		.multiplex_ratio = DT_PROP(node_id, multiplex_ratio),                              \
		.source_voltage = DT_PROP(node_id, source_voltage),                                \
		.remap_value = DT_PROP(node_id, remap_value),                                      \
		.panel_settings = DT_PROP(node_id, panel_settings),                                \
		.hpm_gate_waveform = DT_PROP(node_id, hpm_gate_waveform),                          \
		.lpm_gate_waveform = DT_PROP(node_id, lpm_gate_waveform),                          \
		.color_inversion = DT_PROP(node_id, inversion_on),                                 \
		.specifics = specifics_ptr,                                                        \
		.conversion_buf = conversion_buf##node_id,                                         \
		.conversion_buf_size = sizeof(conversion_buf##node_id),                            \
	};                                                                                         \
	DEVICE_DT_DEFINE(node_id, st730x_init, NULL, NULL, &config##node_id,                       \
			 POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY, &st730x_driver_api);

DT_FOREACH_STATUS_OKAY_VARGS(sitronix_st7305, ST730X_DEFINE_MIPI, &st7305_specifics)
DT_FOREACH_STATUS_OKAY_VARGS(sitronix_st7306, ST730X_DEFINE_MIPI, &st7306_specifics)
