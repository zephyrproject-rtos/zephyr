/*
 * Copyright (c) 2018 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ssd1306, CONFIG_DISPLAY_LOG_LEVEL);

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>

/*
 * Commands
 */
#define SSD1306_SET_LOWER_COL_ADDRESS		0x00 /* No arguments, command is argument */
#define SSD1306_SET_LOWER_COL_ADDRESS_END	0x0f /* Command as argument end */
#define SSD1306_SET_HIGHER_COL_ADDRESS		0x10 /* No arguments, command is argument */
#define SSD1306_SET_HIGHER_COL_ADDRESS_END	0x1f /* Command as argument end */
#define SSD1306_SET_MEM_ADDRESSING_MODE		0x20 /* 1 byte args: A[1:0] Addressing mode */
#define SSD1306_SET_COLUMN_ADDRESS		0x21 /* 2 bytes args: column address, start end */
#define SSD1306_SET_PAGE_ADDRESS		0x22 /* 2 bytes args: page address, start end */
#define SSD1306_SET_PUMP_VOLTAGE_64		0x30 /* No arguments, command is argument */
#define SSD1306_SET_PUMP_VOLTAGE_74		0x31 /* No arguments, command is argument */
#define SSD1306_SET_PUMP_VOLTAGE_80		0x32 /* No arguments, command is argument */
#define SSD1306_SET_PUMP_VOLTAGE_90		0x33 /* No arguments, command is argument */
#define SSD1306_SET_START_LINE			0x40 /* No arguments, command is argument */
#define SSD1306_SET_START_LINE_END		0x7f /* Command as argument end */
#define SSD1306_SET_CONTRAST_CTRL		0x81 /* 1 byte args: Constrast */
#define SH1106_SET_DCDC_DISABLED		0x8a /* No arguments, command is argument */
#define SH1106_SET_DCDC_ENABLED			0x8b /* No arguments, command is argument */
#define SSD1306_SET_CHARGE_PUMP			0x8d /* 1 byte args: A[0]A[7] Volts A[2] Enable */
#define SSD1306_SET_SEGMENT_MAP_NORMAL		0xa0 /* No arguments */
#define SSD1306_SET_SEGMENT_MAP_REMAPED		0xa1 /* No arguments */
#define SSD1306_SET_ENTIRE_DISPLAY_OFF		0xa4 /* No arguments */
#define SSD1306_SET_ENTIRE_DISPLAY_ON		0xa5 /* No arguments */
#define SSD1306_SET_NORMAL_DISPLAY		0xa6 /* No arguments */
#define SSD1306_SET_REVERSE_DISPLAY		0xa7 /* No arguments */
#define SSD1306_SET_MULTIPLEX_RATIO		0xa8 /* 1 byte args: A[5:0] Multiplex Ratio */
#define SSD1306_SET_IREF_MODE			0xad /* 1 byte args: A[5:4] Iref configuration */
#define SH1106_SET_DCDC_MODE			0xad /* No arguments */
#define SSD1306_SET_DISPLAY_OFF			0xae /* No arguments */
#define SSD1306_SET_DISPLAY_ON			0xaf /* No arguments */
#define SSD1306_SET_PAGE_START_ADDRESS		0xb0 /* No arguments, command is argument */
#define SSD1306_SET_PAGE_START_ADDRESS_END	0xb7 /* Command as argument end */
#define SSD1306_SET_COM_OUTPUT_SCAN_NORMAL	0xc0 /* No arguments */
#define SSD1306_SET_COM_OUTPUT_SCAN_FLIPPED	0xc8 /* No arguments */
#define SSD1306_SET_DISPLAY_OFFSET		0xd3 /* 1 byte args: A[5:0] COM shift */
#define SSD1306_SET_CLOCK_DIV_RATIO		0xd5 /* 1 byte args: A[3:0] dratio A[7:4] oscfreq */
#define SSD1306_SET_CHARGE_PERIOD		0xd9 /* 1 byte args: A[3:0] Phase1 A[7:4] Phase2 */
#define SSD1306_SET_PADS_HW_CONFIG		0xda /* 1 byte args: A[5:4] COM configuration */
#define SSD1306_SET_VCOM_DESELECT_LEVEL		0xdb /* 1 byte args: A[5:4] Voltage */

/*
 * Configuration Constants
 */
#define SSD1306_CLOCK_DIV_RATIO			0x0
#define SSD1306_CLOCK_FREQUENCY			0x8
#define SSD1306_PANEL_VCOM_DESEL_LEVEL		0x20
#define SSD1306_PANEL_PUMP_VOLTAGE		SSD1306_SET_PUMP_VOLTAGE_90
#define SSD1306_MEM_ADDRESSING_HORIZONTAL	0x00
#define SSD1306_MEM_ADDRESSING_VERTICAL		0x01
#define SSD1306_MEM_ADDRESSING_PAGE		0x02
#define SSD1306_PANEL_VCOM_DESEL_LEVEL_SSD1309	0x34
#define SSD1306_PADS_HW_SEQUENTIAL		0x02
#define SSD1306_PADS_HW_ALTERNATIVE		0x12
#define SSD1306_PADS_HW_COM_FLIP_SEQUENTIAL	0x22
#define SSD1306_PADS_HW_COM_FLIP_ALTERNATIVE	0x32
#define SSD1306_IREF_MODE_INTERNAL_30UA		0x30
#define SSD1306_IREF_MODE_INTERNAL_19UA		0x10
#define SSD1306_IREF_MODE_EXTERNAL		0x00
#define SSD1306_CHARGE_PUMP_DISABLED		0x10
#define SSD1306_CHARGE_PUMP_ENABLED		0x14

/*
 * Code Constants
 */
/* All following bytes will contain commands */
#define SSD1306_I2C_ALL_BYTES_CMD		0x00
/* All following bytes will contain data */
#define SSD1306_I2C_ALL_BYTES_DATA		0x40
/* The next byte will contain a command */
#define SSD1306_I2C_BYTE_CMD			0x80
/* The next byte will contain data */
#define SSD1306_I2C_BYTE_DATA			0xc0

#define SSD1306_RESET_DELAY			1
#define SSD1306_SUPPLY_DELAY			20

#ifndef SSD1306_ADDRESSING_MODE
#define SSD1306_ADDRESSING_MODE		(SSD1306_MEM_ADDRESSING_HORIZONTAL)
#endif

#define SSD1306_PPB_SHIFT	3

/*
 * Fields
 */
#define SSD1306_READ_STATUS_MASK		0xc0
#define SSD1306_READ_STATUS_BUSY		0x80
#define SSD1306_READ_STATUS_ON			0x40
#define SSD1306_SET_LOWER_COL_ADDRESS_MASK	0x0f
#define SSD1306_SET_HIGHER_COL_ADDRESS_MASK	0x0f
#define SSD1306_SET_START_LINE_MASK		0x3f
#define SSD1306_SET_PAGE_START_ADDRESS_MASK	0x07

union ssd1306_bus {
	struct i2c_dt_spec i2c;
	struct spi_dt_spec spi;
};

typedef bool (*ssd1306_bus_ready_fn)(const struct device *dev);
typedef int (*ssd1306_write_bus_fn)(const struct device *dev, uint8_t *buf, size_t len,
				    bool command);
typedef const char *(*ssd1306_bus_name_fn)(const struct device *dev);

struct ssd1306_config {
	union ssd1306_bus bus;
	struct gpio_dt_spec data_cmd;
	struct gpio_dt_spec reset;
	struct gpio_dt_spec supply;
	ssd1306_bus_ready_fn bus_ready;
	ssd1306_write_bus_fn write_bus;
	ssd1306_bus_name_fn bus_name;
	uint16_t height;
	uint16_t width;
	uint8_t segment_offset;
	uint8_t page_offset;
	uint8_t display_offset;
	uint8_t multiplex_ratio;
	uint8_t prechargep;
	bool segment_remap;
	bool com_invdir;
	bool com_sequential;
	bool color_inversion;
	bool ssd1309_compatible;
	bool sh1106_compatible;
	int ready_time_ms;
	bool use_internal_iref;
};

struct ssd1306_data {
	enum display_pixel_format pf;
	enum display_orientation orientation;
};

#if (DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(solomon_ssd1306, i2c) || \
	DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(solomon_ssd1309, i2c) || \
	DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(sinowealth_sh1106, i2c))
static bool ssd1306_bus_ready_i2c(const struct device *dev)
{
	const struct ssd1306_config *config = dev->config;

	return i2c_is_ready_dt(&config->bus.i2c);
}

static int ssd1306_write_bus_i2c(const struct device *dev, uint8_t *buf, size_t len, bool command)
{
	const struct ssd1306_config *config = dev->config;

	return i2c_burst_write_dt(&config->bus.i2c,
				  command ? SSD1306_I2C_ALL_BYTES_CMD :
				  SSD1306_I2C_ALL_BYTES_DATA,
				  buf, len);
}

static const char *ssd1306_bus_name_i2c(const struct device *dev)
{
	const struct ssd1306_config *config = dev->config;

	return config->bus.i2c.bus->name;
}
#endif

#if (DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(solomon_ssd1306, spi) || \
	DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(solomon_ssd1309, spi) || \
	DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(sinowealth_sh1106, spi))
static bool ssd1306_bus_ready_spi(const struct device *dev)
{
	const struct ssd1306_config *config = dev->config;

	if (gpio_pin_configure_dt(&config->data_cmd, GPIO_OUTPUT_INACTIVE) < 0) {
		return false;
	}

	return spi_is_ready_dt(&config->bus.spi);
}

static int ssd1306_write_bus_spi(const struct device *dev, uint8_t *buf, size_t len, bool command)
{
	const struct ssd1306_config *config = dev->config;
	int ret;

	gpio_pin_set_dt(&config->data_cmd, command ? 0 : 1);
	struct spi_buf tx_buf = {
		.buf = buf,
		.len = len
	};

	struct spi_buf_set tx_bufs = {
		.buffers = &tx_buf,
		.count = 1
	};

	ret = spi_write_dt(&config->bus.spi, &tx_bufs);

	return ret;
}

static const char *ssd1306_bus_name_spi(const struct device *dev)
{
	const struct ssd1306_config *config = dev->config;

	return config->bus.spi.bus->name;
}
#endif

static inline bool ssd1306_bus_ready(const struct device *dev)
{
	const struct ssd1306_config *config = dev->config;

	return config->bus_ready(dev);
}

static inline int ssd1306_write_bus(const struct device *dev, uint8_t *buf, size_t len,
				    bool command)
{
	const struct ssd1306_config *config = dev->config;

	return config->write_bus(dev, buf, len, command);
}

static inline int ssd1306_set_panel_orientation(const struct device *dev, bool rotate_180_degrees)
{
	const struct ssd1306_config *config = dev->config;
	bool segment_remap = config->segment_remap;
	bool com_invdir = config->com_invdir;

	if (rotate_180_degrees) {
		com_invdir = !com_invdir;
		segment_remap = !segment_remap;
	}

	uint8_t cmd_buf[] = {(segment_remap ? SSD1306_SET_SEGMENT_MAP_REMAPED
					    : SSD1306_SET_SEGMENT_MAP_NORMAL),
			     (com_invdir ? SSD1306_SET_COM_OUTPUT_SCAN_FLIPPED
					 : SSD1306_SET_COM_OUTPUT_SCAN_NORMAL)};

	return ssd1306_write_bus(dev, cmd_buf, sizeof(cmd_buf), true);
}

static inline int ssd1306_set_timing_setting(const struct device *dev)
{
	const struct ssd1306_config *config = dev->config;
	uint8_t cmd_buf[] = {SSD1306_SET_CLOCK_DIV_RATIO,
			     (SSD1306_CLOCK_FREQUENCY << 4) | SSD1306_CLOCK_DIV_RATIO,
			     SSD1306_SET_CHARGE_PERIOD,
			     config->prechargep,
			     SSD1306_SET_VCOM_DESELECT_LEVEL,
			     config->ssd1309_compatible ? SSD1306_PANEL_VCOM_DESEL_LEVEL_SSD1309 :
				SSD1306_PANEL_VCOM_DESEL_LEVEL};

	return ssd1306_write_bus(dev, cmd_buf, sizeof(cmd_buf), true);
}

static inline int ssd1306_set_hardware_config(const struct device *dev)
{
	const struct ssd1306_config *config = dev->config;
	uint8_t cmd_buf[] = {
		SSD1306_SET_START_LINE,
		SSD1306_SET_DISPLAY_OFFSET,
		config->display_offset,
		SSD1306_SET_PADS_HW_CONFIG,
		(config->com_sequential ? SSD1306_PADS_HW_SEQUENTIAL
					: SSD1306_PADS_HW_ALTERNATIVE),
		SSD1306_SET_MULTIPLEX_RATIO,
		config->multiplex_ratio,
	};

	return ssd1306_write_bus(dev, cmd_buf, sizeof(cmd_buf), true);
}

static inline int ssd1306_set_charge_pump(const struct device *dev)
{
	const struct ssd1306_config *config = dev->config;
	uint8_t cmd_buf[] = {
		(config->sh1106_compatible ? SH1106_SET_DCDC_MODE : SSD1306_SET_CHARGE_PUMP),
		(config->sh1106_compatible ? SH1106_SET_DCDC_ENABLED
					   : SSD1306_CHARGE_PUMP_ENABLED),
		SSD1306_PANEL_PUMP_VOLTAGE,
	};

	return ssd1306_write_bus(dev, cmd_buf, sizeof(cmd_buf), true);
}

static inline int ssd1306_set_iref_mode(const struct device *dev)
{
	int ret = 0;
	const struct ssd1306_config *config = dev->config;
	uint8_t cmd_buf[] = {
		SSD1306_SET_IREF_MODE,
		SSD1306_IREF_MODE_INTERNAL_30UA,
	};

	if (config->use_internal_iref) {
		ret = ssd1306_write_bus(dev, cmd_buf, sizeof(cmd_buf), true);
	}

	return ret;
}

static int ssd1306_resume(const struct device *dev)
{
	const struct ssd1306_config *config = dev->config;
	uint8_t cmd_buf[] = {
		SSD1306_SET_DISPLAY_ON,
	};

	/* Turn on supply if pin connected */
	if (config->supply.port) {
		gpio_pin_set_dt(&config->supply, 1);
		k_sleep(K_MSEC(SSD1306_SUPPLY_DELAY));
	}

	return ssd1306_write_bus(dev, cmd_buf, sizeof(cmd_buf), true);
}

static int ssd1306_suspend(const struct device *dev)
{
	const struct ssd1306_config *config = dev->config;
	uint8_t cmd_buf[] = {
		SSD1306_SET_DISPLAY_OFF,
	};

	/* Turn off supply if pin connected */
	if (config->supply.port) {
		gpio_pin_set_dt(&config->supply, 0);
		k_sleep(K_MSEC(SSD1306_SUPPLY_DELAY));
	}

	return ssd1306_write_bus(dev, cmd_buf, sizeof(cmd_buf), true);
}

static int ssd1306_write_default(const struct device *dev, const uint16_t x, const uint16_t y,
				 const struct display_buffer_descriptor *desc, uint8_t *buf,
				 const size_t buf_len)
{
	const struct ssd1306_config *config = dev->config;
	const uint8_t x_offset = x + config->segment_offset;
	uint8_t cmd_buf[] = {
		SSD1306_SET_MEM_ADDRESSING_MODE,
		SSD1306_ADDRESSING_MODE,
		SSD1306_SET_COLUMN_ADDRESS,
		x_offset,
		x_offset + desc->width - 1,
		SSD1306_SET_PAGE_ADDRESS,
		y >> SSD1306_PPB_SHIFT,
		(((y + desc->height) >> SSD1306_PPB_SHIFT) - 1)
	};
	int ret;

	ret = ssd1306_write_bus(dev, cmd_buf, sizeof(cmd_buf), true);
	if (ret < 0) {
		LOG_ERR("Failed to write command: %d", ret);
		return ret;
	}

	return ssd1306_write_bus(dev, buf, buf_len, false);
}

static int ssd1306_write_sh1106(const struct device *dev, const uint16_t x, const uint16_t y,
				const struct display_buffer_descriptor *desc, uint8_t *buf,
				const size_t buf_len)
{
	const struct ssd1306_config *config = dev->config;
	const uint8_t x_offset = x + config->segment_offset;
	const uint8_t y_offset = y >> SSD1306_PPB_SHIFT;
	const uint8_t height = desc->height >> SSD1306_PPB_SHIFT;
	uint8_t cmd_buf[] = {
		SSD1306_SET_LOWER_COL_ADDRESS |
			(x_offset & SSD1306_SET_LOWER_COL_ADDRESS_MASK),
		SSD1306_SET_HIGHER_COL_ADDRESS |
			((x_offset >> 4) & SSD1306_SET_LOWER_COL_ADDRESS_MASK),
		SSD1306_SET_PAGE_START_ADDRESS | y_offset
	};
	int ret = 0;

	for (uint8_t y_o = 0; y_o < height; y_o++) {
		cmd_buf[sizeof(cmd_buf) - 1] = SSD1306_SET_PAGE_START_ADDRESS | (y_o + y_offset);
		LOG_HEXDUMP_DBG(cmd_buf, sizeof(cmd_buf), "cmd_buf");

		ret = ssd1306_write_bus(dev, cmd_buf, sizeof(cmd_buf), true);
		if (ret < 0) {
			LOG_ERR("Failed to write position");
			return ret;
		}

		ret = ssd1306_write_bus(dev, &(buf[y_o * desc->width]), desc->width, false);
		if (ret < 0) {
			LOG_ERR("Failed to write pixel data");
			return ret;
		}
	}

	return ret;
}

static int ssd1306_write(const struct device *dev, const uint16_t x, const uint16_t y,
			 const struct display_buffer_descriptor *desc, const void *buf)
{
	const struct ssd1306_config *config = dev->config;
	const size_t buf_len = (desc->height * desc->width) >> SSD1306_PPB_SHIFT;

	if (desc->pitch != desc->width) {
		LOG_ERR("Pitch is not width");
		return -EINVAL;
	}

	if (buf == NULL) {
		LOG_ERR("Display buffer is invalid");
		return -ENODATA;
	}

	if (buf_len > desc->buf_size) {
		LOG_ERR("Display buffer is too small");
		return -ENODATA;
	}

	if ((y & 0x7) != 0U) {
		LOG_ERR("Unsupported origin");
		return -EINVAL;
	}

	if ((desc->height & 0x7) != 0U) {
		LOG_ERR("Unsupported height");
		return -EINVAL;
	}

	if (desc->buf_size == 0U) {
		return 0;
	}

	LOG_DBG("x %u, y %u, pitch %u, width %u, height %u, buf_len %u", x, y, desc->pitch,
		desc->width, desc->height, buf_len);

	if (config->sh1106_compatible) {
		return ssd1306_write_sh1106(dev, x, y, desc, (uint8_t *)buf, buf_len);
	}

	return ssd1306_write_default(dev, x, y, desc, (uint8_t *)buf, buf_len);
}

static int ssd1306_set_contrast(const struct device *dev, const uint8_t contrast)
{
	uint8_t cmd_buf[] = {
		SSD1306_SET_CONTRAST_CTRL,
		contrast,
	};

	return ssd1306_write_bus(dev, cmd_buf, sizeof(cmd_buf), true);
}

static void ssd1306_get_capabilities(const struct device *dev,
				     struct display_capabilities *caps)
{
	const struct ssd1306_config *config = dev->config;
	struct ssd1306_data *data = dev->data;

	caps->x_resolution = config->width;
	caps->y_resolution = config->height;
	caps->supported_pixel_formats = PIXEL_FORMAT_MONO10 | PIXEL_FORMAT_MONO01;
	caps->current_pixel_format = data->pf;
	caps->screen_info = SCREEN_INFO_MONO_VTILED;
	caps->current_orientation = data->orientation;
}

static int ssd1306_set_orientation(const struct device *dev,
				   const enum display_orientation orientation)
{
	struct ssd1306_data *data = dev->data;
	int ret;

	if (orientation == data->orientation) {
		return 0;
	}

	if (orientation == DISPLAY_ORIENTATION_NORMAL) {
		ret = ssd1306_set_panel_orientation(dev, false);
	} else if (orientation == DISPLAY_ORIENTATION_ROTATED_180) {
		ret = ssd1306_set_panel_orientation(dev, true);
	} else {
		LOG_WRN("Unsupported orientation");
		return -ENOTSUP;
	}

	if (ret) {
		return ret;
	}

	data->orientation = orientation;

	return 0;
}

static int ssd1306_set_pixel_format(const struct device *dev,
				    const enum display_pixel_format pf)
{
	struct ssd1306_data *data = dev->data;
	uint8_t cmd;
	int ret;

	if (pf == data->pf) {
		return 0;
	}

	if (pf == PIXEL_FORMAT_MONO10) {
		cmd = SSD1306_SET_REVERSE_DISPLAY;
	} else if (pf == PIXEL_FORMAT_MONO01) {
		cmd = SSD1306_SET_NORMAL_DISPLAY;
	} else {
		LOG_WRN("Unsupported pixel format");
		return -ENOTSUP;
	}

	ret = ssd1306_write_bus(dev, &cmd, 1, true);
	if (ret < 0) {
		LOG_ERR("Failed to set pixel format");
		return ret;
	}

	data->pf = pf;

	return 0;
}

static int ssd1306_init_device(const struct device *dev)
{
	const struct ssd1306_config *config = dev->config;
	struct ssd1306_data *data = dev->data;
	int ret;

	uint8_t cmd_buf[] = {
		SSD1306_SET_ENTIRE_DISPLAY_OFF,
		(config->color_inversion ? SSD1306_SET_REVERSE_DISPLAY
					 : SSD1306_SET_NORMAL_DISPLAY),
	};

	data->pf = config->color_inversion ? PIXEL_FORMAT_MONO10 : PIXEL_FORMAT_MONO01;
	/* Turn on supply if pin connected */
	if (config->supply.port) {
		gpio_pin_set_dt(&config->supply, 1);
		k_sleep(K_MSEC(SSD1306_SUPPLY_DELAY));
	}

	/* Reset if pin connected */
	if (config->reset.port) {
		gpio_pin_set_dt(&config->reset, 1);
		k_sleep(K_MSEC(SSD1306_RESET_DELAY));
		gpio_pin_set_dt(&config->reset, 0);
		k_sleep(K_MSEC(SSD1306_RESET_DELAY));
	}

	/* Turn display off */
	ret = ssd1306_suspend(dev);
	if (ret < 0) {
		LOG_ERR("Failed to suspend: %d", ret);
		return ret;
	}

	ret = ssd1306_set_timing_setting(dev);
	if (ret < 0) {
		LOG_ERR("Failed to set timings: %d", ret);
		return ret;
	}

	ret = ssd1306_set_hardware_config(dev);
	if (ret < 0) {
		LOG_ERR("Failed to set hardware configuration: %d", ret);
		return ret;
	}

	ret = ssd1306_set_panel_orientation(dev, false);
	if (ret < 0) {
		LOG_ERR("Failed to set panel orientation: %d", ret);
		return ret;
	}
	data->orientation = DISPLAY_ORIENTATION_NORMAL;

	if (!config->ssd1309_compatible) {
		ret = ssd1306_set_charge_pump(dev);
		if (ret < 0) {
			LOG_ERR("Failed to apply charge pump settings: %d", ret);
			return ret;
		}

		ret = ssd1306_set_iref_mode(dev);
		if (ret < 0) {
			LOG_ERR("Failed to set reference settings: %d", ret);
			return ret;
		}
	}

	ret = ssd1306_write_bus(dev, cmd_buf, sizeof(cmd_buf), true);
	if (ret < 0) {
		LOG_ERR("Failed to set inversion: %d", ret);
		return ret;
	}

	ret = ssd1306_set_contrast(dev, CONFIG_SSD1306_DEFAULT_CONTRAST);
	if (ret < 0) {
		LOG_ERR("Failed to set default contrast: %d", ret);
		return ret;
	}

	return ssd1306_resume(dev);
}

static int ssd1306_init(const struct device *dev)
{
	const struct ssd1306_config *config = dev->config;
	int ret;

	k_sleep(K_TIMEOUT_ABS_MS(config->ready_time_ms));

	if (!ssd1306_bus_ready(dev)) {
		LOG_ERR("Bus device %s not ready!", config->bus_name(dev));
		return -EINVAL;
	}

	if (config->supply.port) {
		ret = gpio_pin_configure_dt(&config->supply,
					    GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			return ret;
		}
		if (!gpio_is_ready_dt(&config->supply)) {
			LOG_ERR("Supply GPIO device not ready");
			return -ENODEV;
		}
	}

	if (config->reset.port) {
		ret = gpio_pin_configure_dt(&config->reset,
					    GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			return ret;
		}
		if (!gpio_is_ready_dt(&config->reset)) {
			LOG_ERR("Reset GPIO device not ready");
			return -ENODEV;
		}
	}

	if (ssd1306_init_device(dev)) {
		LOG_ERR("Failed to initialize device!");
		return -EIO;
	}

	return 0;
}

static DEVICE_API(display, ssd1306_driver_api) = {
	.blanking_on = ssd1306_suspend,
	.blanking_off = ssd1306_resume,
	.write = ssd1306_write,
	.set_contrast = ssd1306_set_contrast,
	.get_capabilities = ssd1306_get_capabilities,
	.set_pixel_format = ssd1306_set_pixel_format,
	.set_orientation = ssd1306_set_orientation,
};

#define SSD1306_CONFIG_SPI(node_id)                                                                \
	.bus = {.spi = SPI_DT_SPEC_GET(                                                            \
			node_id, SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8))},        \
	.bus_ready = ssd1306_bus_ready_spi,                                                        \
	.write_bus = ssd1306_write_bus_spi,                                                        \
	.bus_name = ssd1306_bus_name_spi,                                                          \
	.data_cmd = GPIO_DT_SPEC_GET(node_id, data_cmd_gpios),

#define SSD1306_CONFIG_I2C(node_id)                                                                \
	.bus = {.i2c = I2C_DT_SPEC_GET(node_id)},                                                  \
	.bus_ready = ssd1306_bus_ready_i2c,                                                        \
	.write_bus = ssd1306_write_bus_i2c,                                                        \
	.bus_name = ssd1306_bus_name_i2c,                                                          \
	.data_cmd = {0},

#define SSD1306_DEFINE(node_id)                                                                    \
	static struct ssd1306_data data##node_id;                                                  \
	static const struct ssd1306_config config##node_id = {                                     \
		.reset = GPIO_DT_SPEC_GET_OR(node_id, reset_gpios, {0}),                           \
		.supply = GPIO_DT_SPEC_GET_OR(node_id, supply_gpios, {0}),                         \
		.height = DT_PROP(node_id, height),                                                \
		.width = DT_PROP(node_id, width),                                                  \
		.segment_offset = DT_PROP(node_id, segment_offset),                                \
		.page_offset = DT_PROP(node_id, page_offset),                                      \
		.display_offset = DT_PROP(node_id, display_offset),                                \
		.multiplex_ratio = DT_PROP(node_id, multiplex_ratio),                              \
		.segment_remap = DT_PROP(node_id, segment_remap),                                  \
		.com_invdir = DT_PROP(node_id, com_invdir),                                        \
		.com_sequential = DT_PROP(node_id, com_sequential),                                \
		.prechargep = DT_PROP(node_id, prechargep),                                        \
		.color_inversion = DT_PROP(node_id, inversion_on),                                 \
		.ssd1309_compatible = DT_NODE_HAS_COMPAT(node_id, solomon_ssd1309),                \
		.sh1106_compatible = DT_NODE_HAS_COMPAT(node_id, sinowealth_sh1106),               \
		.ready_time_ms = DT_PROP(node_id, ready_time_ms),                                  \
		.use_internal_iref = DT_PROP(node_id, use_internal_iref),                          \
		COND_CODE_1(DT_ON_BUS(node_id, spi), (SSD1306_CONFIG_SPI(node_id)),                \
			    (SSD1306_CONFIG_I2C(node_id)))                                         \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_DEFINE(node_id, ssd1306_init, NULL, &data##node_id, &config##node_id,            \
			 POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY, &ssd1306_driver_api);

DT_FOREACH_STATUS_OKAY(solomon_ssd1306, SSD1306_DEFINE)
DT_FOREACH_STATUS_OKAY(solomon_ssd1309, SSD1306_DEFINE)
DT_FOREACH_STATUS_OKAY(sinowealth_sh1106, SSD1306_DEFINE)
