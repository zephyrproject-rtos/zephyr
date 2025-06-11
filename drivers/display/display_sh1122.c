/*
 * Copyright (c) 2025 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sh1122, CONFIG_DISPLAY_LOG_LEVEL);

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/mipi_dbi.h>
#include <zephyr/kernel.h>

#define SH1122_CONTROL_ALL_BYTES_CMD  0x0
#define SH1122_CONTROL_ALL_BYTES_DATA 0x40

#define SH1122_SET_PHASE_LENGTH    0xD9
#define SH1122_SET_OSC_FREQ        0xD5
#define SH1122_SET_VCOMH           0xDB
#define SH1122_SET_DCDC            0xAD
#define SH1122_SET_DISPLAY_OFFSET  0xD3
#define SH1122_DISPLAY_ON          0xAF
#define SH1122_DISPLAY_OFF         0xAE
#define SH1122_SET_MULTIPLEX_RATIO 0xA8
#define SH1122_SET_SEG_ORDER_10    0xA0
#define SH1122_SET_SEG_ORDER_01    0xA1
#define SH1122_SET_COM_ORDER_10    0xC0
#define SH1122_SET_COM_ORDER_01    0xC8
#define SH1122_SET_CONTRAST_CTRL   0x81
#define SH1122_SET_VSEGM           0xDC
#define SH1122_SET_DISPLAY_RAM     0xA4
#define SH1122_SET_DISPLAY_ALL_ON  0xA5
#define SH1122_SET_NORMAL_DISPLAY  0xA6
#define SH1122_SET_REVERSE_DISPLAY 0xA7
#define SH1122_SET_ROW_ADDR        0xB0

#define SH1122_SET_COLUMN_ADDR_HIGH(n)   (0x10 + ((n) >> 4))
#define SH1122_SET_COLUMN_ADDR_LOW(n)    (0x0 + ((n) & 0xf))
#define SH1122_SET_DISPLAY_START_LINE(n) (0x40 + ((n) & 0x3f))
#define SH1122_SET_VSL(n)                (0x30 + ((n) & 0xf))

#define SH1122_RESET_DELAY        10
#define SH1122_MAXIMUM_CMD_LENGTH 16
/* One line since we need to return to column every partial line */
#define SH1122_CONV_BUFFER_SIZE   128

typedef int (*sh1122_write_bus_cmd_fn)(const struct device *dev, const uint8_t cmd,
				       const uint8_t *data, size_t len);
typedef int (*sh1122_write_pixels_fn)(const struct device *dev, const uint8_t *buf,
				      const struct display_buffer_descriptor *desc);
typedef int (*sh1122_release_bus_fn)(const struct device *dev);

struct sh1122_config {
	struct i2c_dt_spec i2c;
	sh1122_write_bus_cmd_fn write_cmd;
	sh1122_write_pixels_fn write_pixels;
	sh1122_release_bus_fn release_bus;
	const struct device *mipi_dev;
	const struct mipi_dbi_config dbi_config;
	uint16_t height;
	uint16_t width;
	uint8_t oscillator_freq;
	uint8_t start_line;
	uint8_t display_offset;
	uint8_t multiplex_ratio;
	uint8_t dc_dc;
	uint8_t remap_value;
	uint8_t phase_length;
	uint8_t precharge_voltage;
	uint8_t vcomh_voltage;
	uint8_t low_voltage;
	bool color_inversion;
	bool inv_seg;
	bool inv_com;
	uint8_t *conversion_buf;
	size_t conversion_buf_size;
};

struct sh1122_data {
	uint8_t contrast;
	uint8_t scan_mode;
};

static inline int sh1122_write_bus_cmd_mipi(const struct device *dev, const uint8_t cmd,
					    const uint8_t *data, size_t len)
{
	const struct sh1122_config *config = dev->config;
	int err;

	/* Values given after the memory register must be sent with pin D/C set to 0. */
	/* Data is sent as a command following the mipi_cbi api */
	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config, cmd, NULL, 0);
	if (err) {
		return err;
	}
	for (size_t i = 0; i < len; i++) {
		err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config, data[i], NULL,
					     0);
		if (err) {
			return err;
		}
	}

	return 0;
}

static inline int sh1122_write_bus_cmd_i2c(const struct device *dev, const uint8_t cmd,
					   const uint8_t *data, size_t len)
{
	const struct sh1122_config *config = dev->config;
	static uint8_t buf[SH1122_MAXIMUM_CMD_LENGTH];

	if (len > SH1122_MAXIMUM_CMD_LENGTH - 1) {
		return -EINVAL;
	}

	buf[0] = cmd;
	memcpy(&(buf[1]), data, len);

	return i2c_burst_write_dt(&config->i2c, SH1122_CONTROL_ALL_BYTES_CMD, buf, len + 1);
}

static inline int sh1122_set_hardware_config(const struct device *dev)
{
	const struct sh1122_config *config = dev->config;
	int err;

	err = config->write_cmd(dev, SH1122_SET_DISPLAY_START_LINE(config->start_line), NULL, 0);
	if (err < 0) {
		return err;
	}
	err = config->write_cmd(dev, SH1122_SET_DISPLAY_OFFSET, &config->display_offset, 1);
	if (err < 0) {
		return err;
	}

	err = config->write_cmd(dev, SH1122_SET_DISPLAY_RAM, NULL, 0);
	if (err < 0) {
		return err;
	}
	err = config->write_cmd(dev, SH1122_SET_NORMAL_DISPLAY, NULL, 0);
	if (err < 0) {
		return err;
	}
	err = config->write_cmd(
		dev, config->inv_com ? SH1122_SET_COM_ORDER_01 : SH1122_SET_COM_ORDER_10, NULL, 0);
	if (err < 0) {
		return err;
	}
	err = config->write_cmd(
		dev, config->inv_seg ? SH1122_SET_SEG_ORDER_01 : SH1122_SET_SEG_ORDER_10, NULL, 0);
	if (err < 0) {
		return err;
	}
	err = config->write_cmd(dev, SH1122_SET_VSL(config->low_voltage), NULL, 0);
	if (err < 0) {
		return err;
	}
	err = config->write_cmd(dev, SH1122_SET_MULTIPLEX_RATIO, &config->multiplex_ratio, 1);
	if (err < 0) {
		return err;
	}

	err = config->write_cmd(dev, SH1122_SET_PHASE_LENGTH, &config->phase_length, 1);
	if (err < 0) {
		return err;
	}
	err = config->write_cmd(dev, SH1122_SET_OSC_FREQ, &config->oscillator_freq, 1);
	if (err < 0) {
		return err;
	}
	err = config->write_cmd(dev, SH1122_SET_VSEGM, &config->precharge_voltage, 1);
	if (err < 0) {
		return err;
	}
	err = config->write_cmd(dev, SH1122_SET_VCOMH, &config->vcomh_voltage, 1);
	if (err < 0) {
		return err;
	}
	return config->write_cmd(dev, SH1122_SET_DCDC, &config->dc_dc, 1);
}

static int sh1122_resume(const struct device *dev)
{
	const struct sh1122_config *config = dev->config;
	int err;

	err = config->write_cmd(dev, SH1122_DISPLAY_ON, NULL, 0);
	if (err < 0) {
		return err;
	}
	return config->release_bus(dev);
}

static int sh1122_suspend(const struct device *dev)
{
	const struct sh1122_config *config = dev->config;
	int err;

	err = config->write_cmd(dev, SH1122_DISPLAY_OFF, NULL, 0);
	if (err < 0) {
		return err;
	}
	return config->release_bus(dev);
}

#if DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(sinowealth_sh1122, mipi_dbi)
static int sh1122_write_pixels_mipi(const struct device *dev, const uint8_t *buf,
				    const struct display_buffer_descriptor *desc)
{
	const struct sh1122_config *config = dev->config;
	struct display_buffer_descriptor mipi_desc;

	mipi_desc.buf_size = desc->width / 2;
	mipi_desc.pitch = desc->pitch;
	mipi_desc.width = desc->width;
	mipi_desc.height = 1;

	/* This is the wrong format, but it doesn't matter to almost all mipi drivers */
	return mipi_dbi_write_display(config->mipi_dev, &config->dbi_config, config->conversion_buf,
				      &mipi_desc, PIXEL_FORMAT_L_8);
}

static int sh1122_release_bus_mipi(const struct device *dev)
{
	const struct sh1122_config *config = dev->config;

	return mipi_dbi_release(config->mipi_dev, &config->dbi_config);
}
#endif

#if DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(sinowealth_sh1122, i2c)
static int sh1122_write_pixels_i2c(const struct device *dev, const uint8_t *buf,
				   const struct display_buffer_descriptor *desc)
{
	const struct sh1122_config *config = dev->config;

	return i2c_burst_write_dt(&config->i2c, SH1122_CONTROL_ALL_BYTES_DATA,
				  config->conversion_buf, desc->width / 2);
}

static int sh1122_release_bus_i2c(const struct device *dev)
{
	return 0;
}
#endif

static int sh1122_write(const struct device *dev, const uint16_t x, const uint16_t y,
			const struct display_buffer_descriptor *desc, const void *buf)
{
	const struct sh1122_config *config = dev->config;
	int err;
	size_t buf_len;
	int pixel_count = desc->width * desc->height;
	int total = 0;
	uint8_t ybuf = y;

	if (desc->pitch != desc->width) {
		LOG_ERR("Pitch is not width");
		return -EINVAL;
	}

	/* Following the datasheet, two segment are split in one register */
	buf_len = MIN(desc->buf_size, desc->height * desc->width / 2);
	if (buf == NULL || buf_len == 0U) {
		LOG_ERR("Display buffer is not available");
		return -EINVAL;
	}

	if ((x & 1) != 0U) {
		LOG_ERR("Unsupported origin");
		return -EINVAL;
	}

	LOG_DBG("x %u, y %u, pitch %u, width %u, height %u, buf_len %u", x, y, desc->pitch,
		desc->width, desc->height, buf_len);

	while (pixel_count > total) {

		err = config->write_cmd(dev, SH1122_SET_COLUMN_ADDR_HIGH(x / 2), NULL, 0);
		if (err) {
			return err;
		}

		err = config->write_cmd(dev, SH1122_SET_COLUMN_ADDR_LOW(x / 2), NULL, 0);
		if (err) {
			return err;
		}

		err = config->write_cmd(dev, SH1122_SET_ROW_ADDR, &ybuf, 1);
		if (err) {
			return err;
		}

		/* Convert one line to pixelx+1 (3:0) and pixelx (7:4)
		 * SH1122 is 2 pixels per byte.
		 */
		for (int i = 0; desc->width > i; i += 2) {
			config->conversion_buf[i / 2] = (((uint8_t *)buf)[total + i + 1] >> 4) |
							((((uint8_t *)buf)[total + i] >> 4) << 4);
		}

		err = config->write_pixels(dev, config->conversion_buf, desc);
		if (err) {
			return err;
		}
		total += desc->width;
		ybuf++;
	}
	return config->release_bus(dev);
}

static int sh1122_set_contrast(const struct device *dev, const uint8_t contrast)
{
	const struct sh1122_config *config = dev->config;
	int err;

	err = config->write_cmd(dev, SH1122_SET_CONTRAST_CTRL, &contrast, 1);
	if (err < 0) {
		return err;
	}
	return config->release_bus(dev);
}

static void sh1122_get_capabilities(const struct device *dev, struct display_capabilities *caps)
{
	const struct sh1122_config *config = dev->config;

	memset(caps, 0, sizeof(struct display_capabilities));
	caps->x_resolution = config->width;
	caps->y_resolution = config->height;
	caps->supported_pixel_formats = PIXEL_FORMAT_L_8;
	caps->current_pixel_format = PIXEL_FORMAT_L_8;
	caps->screen_info = 0;
}

static int sh1122_set_pixel_format(const struct device *dev, const enum display_pixel_format pf)
{
	if (pf == PIXEL_FORMAT_L_8) {
		return 0;
	}
	LOG_ERR("Unsupported pixel format");
	return -ENOTSUP;
}

static int sh1122_init_device(const struct device *dev)
{
	const struct sh1122_config *config = dev->config;
	int err;

	/* Turn display off */
	err = sh1122_suspend(dev);
	if (err < 0) {
		return err;
	}

	err = sh1122_set_contrast(dev, CONFIG_SH1122_DEFAULT_CONTRAST);
	if (err < 0) {
		return err;
	}

	err = sh1122_set_hardware_config(dev);
	if (err < 0) {
		return err;
	}

	err = config->write_cmd(dev,
				config->color_inversion ? SH1122_SET_REVERSE_DISPLAY
							: SH1122_SET_NORMAL_DISPLAY,
				NULL, 0);
	if (err < 0) {
		return err;
	}

	err = sh1122_resume(dev);
	if (err < 0) {
		return err;
	}

	return config->release_bus(dev);
}

#if DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(sinowealth_sh1122, mipi_dbi)
static int sh1122_init(const struct device *dev)
{
	const struct sh1122_config *config = dev->config;
	int err;

	LOG_DBG("Initializing device");

	if (!device_is_ready(config->mipi_dev)) {
		LOG_ERR("MIPI Device not ready!");
		return -EINVAL;
	}

	err = mipi_dbi_reset(config->mipi_dev, SH1122_RESET_DELAY);
	if (err < 0) {
		LOG_ERR("Failed to reset device!");
		return err;
	}
	k_msleep(SH1122_RESET_DELAY);

	err = sh1122_init_device(dev);
	if (err < 0) {
		LOG_ERR("Failed to initialize device! %d", err);
		return err;
	}

	return 0;
}
#endif

#if DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(sinowealth_sh1122, i2c)
static int sh1122_init_i2c(const struct device *dev)
{
	const struct sh1122_config *config = dev->config;
	int err;

	LOG_DBG("Initializing device");

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C Device not ready!");
		return -EINVAL;
	}

	err = sh1122_init_device(dev);
	if (err < 0) {
		LOG_ERR("Failed to initialize device! %d", err);
		return err;
	}

	return 0;
}
#endif

static DEVICE_API(display, sh1122_driver_api) = {
	.blanking_on = sh1122_suspend,
	.blanking_off = sh1122_resume,
	.write = sh1122_write,
	.set_contrast = sh1122_set_contrast,
	.get_capabilities = sh1122_get_capabilities,
	.set_pixel_format = sh1122_set_pixel_format,
};

#define SH1122_WORD_SIZE(inst)                                                                     \
	((DT_STRING_UPPER_TOKEN(inst, mipi_mode) == MIPI_DBI_MODE_SPI_4WIRE) ? SPI_WORD_SET(8)     \
									     : SPI_WORD_SET(9))

#define SH1122_DEFINE_I2C(node_id)                                                                 \
	static uint8_t conversion_buf##node_id[SH1122_CONV_BUFFER_SIZE];                           \
	static struct sh1122_data data##node_id;                                                   \
	static const struct sh1122_config config##node_id = {                                      \
		.i2c = I2C_DT_SPEC_GET(node_id),                                                   \
		.height = DT_PROP(node_id, height),                                                \
		.width = DT_PROP(node_id, width),                                                  \
		.oscillator_freq = DT_PROP(node_id, oscillator_freq),                              \
		.display_offset = DT_PROP(node_id, display_offset),                                \
		.start_line = DT_PROP(node_id, start_line),                                        \
		.multiplex_ratio = DT_PROP(node_id, multiplex_ratio),                              \
		.color_inversion = DT_PROP(node_id, inversion_on),                                 \
		.phase_length = DT_PROP(node_id, phase_length),                                    \
		.dc_dc = DT_PROP(node_id, dc_dc),                                                  \
		.precharge_voltage = DT_PROP(node_id, precharge_voltage),                          \
		.vcomh_voltage = DT_PROP(node_id, vcomh_voltage),                                  \
		.low_voltage = DT_PROP(node_id, low_voltage),                                      \
		.inv_seg = DT_PROP(node_id, inv_seg),                                              \
		.inv_com = DT_PROP(node_id, inv_com),                                              \
		.write_cmd = sh1122_write_bus_cmd_i2c,                                             \
		.write_pixels = sh1122_write_pixels_i2c,                                           \
		.release_bus = sh1122_release_bus_i2c,                                             \
		.conversion_buf = conversion_buf##node_id,                                         \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_DEFINE(node_id, sh1122_init_i2c, NULL, &data##node_id, &config##node_id,         \
			 POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY, &sh1122_driver_api);

#define SH1122_DEFINE_MIPI(node_id)                                                                \
	static uint8_t conversion_buf##node_id[SH1122_CONV_BUFFER_SIZE];                           \
	static struct sh1122_data data##node_id;                                                   \
	static const struct sh1122_config config##node_id = {                                      \
		.mipi_dev = DEVICE_DT_GET(DT_PARENT(node_id)),                                     \
		.dbi_config = MIPI_DBI_CONFIG_DT(                                                  \
			node_id, SH1122_WORD_SIZE(node_id) | SPI_OP_MODE_MASTER, 0),               \
		.height = DT_PROP(node_id, height),                                                \
		.width = DT_PROP(node_id, width),                                                  \
		.oscillator_freq = DT_PROP(node_id, oscillator_freq),                              \
		.display_offset = DT_PROP(node_id, display_offset),                                \
		.start_line = DT_PROP(node_id, start_line),                                        \
		.multiplex_ratio = DT_PROP(node_id, multiplex_ratio),                              \
		.color_inversion = DT_PROP(node_id, inversion_on),                                 \
		.phase_length = DT_PROP(node_id, phase_length),                                    \
		.dc_dc = DT_PROP(node_id, dc_dc),                                                  \
		.precharge_voltage = DT_PROP(node_id, precharge_voltage),                          \
		.vcomh_voltage = DT_PROP(node_id, vcomh_voltage),                                  \
		.low_voltage = DT_PROP(node_id, low_voltage),                                      \
		.inv_seg = DT_PROP(node_id, inv_seg),                                              \
		.inv_com = DT_PROP(node_id, inv_com),                                              \
		.write_cmd = sh1122_write_bus_cmd_mipi,                                            \
		.write_pixels = sh1122_write_pixels_mipi,                                          \
		.release_bus = sh1122_release_bus_mipi,                                            \
		.conversion_buf = conversion_buf##node_id,                                         \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_DEFINE(node_id, sh1122_init, NULL, &data##node_id, &config##node_id,             \
			 POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY, &sh1122_driver_api);

#define SH1122_DEFINE(node_id)                                                                     \
	COND_CODE_1(DT_ON_BUS(node_id, i2c), \
	(SH1122_DEFINE_I2C(node_id)), (SH1122_DEFINE_MIPI(node_id)))

DT_FOREACH_STATUS_OKAY(sinowealth_sh1122, SH1122_DEFINE)
