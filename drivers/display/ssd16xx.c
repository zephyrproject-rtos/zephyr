/*
 * Copyright (c) 2018-2020 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT solomon_ssd16xxfb

#define LOG_LEVEL CONFIG_DISPLAY_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(ssd16xx);

#include <string.h>
#include <device.h>
#include <drivers/display.h>
#include <init.h>
#include <drivers/gpio.h>
#include <drivers/spi.h>
#include <sys/byteorder.h>

#include "ssd16xx_regs.h"
#include <display/cfb.h>

/**
 * SSD1673, SSD1608, SSD1681, ILI3897 compatible EPD controller driver.
 */

#define SSD16XX_SPI_FREQ DT_INST_PROP(0, spi_max_frequency)
#define SSD16XX_BUS_NAME DT_INST_BUS_LABEL(0)
#define SSD16XX_DC_PIN DT_INST_GPIO_PIN(0, dc_gpios)
#define SSD16XX_DC_FLAGS DT_INST_GPIO_FLAGS(0, dc_gpios)
#define SSD16XX_DC_CNTRL DT_INST_GPIO_LABEL(0, dc_gpios)
#define SSD16XX_CS_PIN DT_INST_SPI_DEV_CS_GPIOS_PIN(0)
#define SSD16XX_CS_FLAGS DT_INST_SPI_DEV_CS_GPIOS_FLAGS(0)
#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
#define SSD16XX_CS_CNTRL DT_INST_SPI_DEV_CS_GPIOS_LABEL(0)
#endif
#define SSD16XX_BUSY_PIN DT_INST_GPIO_PIN(0, busy_gpios)
#define SSD16XX_BUSY_CNTRL DT_INST_GPIO_LABEL(0, busy_gpios)
#define SSD16XX_BUSY_FLAGS DT_INST_GPIO_FLAGS(0, busy_gpios)
#define SSD16XX_RESET_PIN DT_INST_GPIO_PIN(0, reset_gpios)
#define SSD16XX_RESET_CNTRL DT_INST_GPIO_LABEL(0, reset_gpios)
#define SSD16XX_RESET_FLAGS DT_INST_GPIO_FLAGS(0, reset_gpios)

#define EPD_PANEL_WIDTH			DT_INST_PROP(0, width)
#define EPD_PANEL_HEIGHT		DT_INST_PROP(0, height)
#define EPD_PANEL_NUMOF_COLUMS		EPD_PANEL_WIDTH
#define EPD_PANEL_NUMOF_ROWS_PER_PAGE	8
#define EPD_PANEL_NUMOF_PAGES		(EPD_PANEL_HEIGHT / \
					 EPD_PANEL_NUMOF_ROWS_PER_PAGE)

#define SSD16XX_PANEL_FIRST_PAGE	0
#define SSD16XX_PANEL_LAST_PAGE		(EPD_PANEL_NUMOF_PAGES - 1)
#define SSD16XX_PANEL_FIRST_GATE	0
#define SSD16XX_PANEL_LAST_GATE		(EPD_PANEL_NUMOF_COLUMS - 1)

#define SSD16XX_PIXELS_PER_BYTE		8
#define SSD16XX_DEFAULT_TR_VALUE	25U
#define SSD16XX_TR_SCALE_FACTOR		256U

struct ssd16xx_data {
	const struct device *reset;
	const struct device *dc;
	const struct device *busy;
	const struct device *spi_dev;
	struct spi_config spi_config;
#if defined(SSD16XX_CS_CNTRL)
	struct spi_cs_control cs_ctrl;
#endif
	uint8_t scan_mode;
	uint8_t update_cmd;
};

#if DT_INST_NODE_HAS_PROP(0, lut_initial)
static uint8_t ssd16xx_lut_initial[] = DT_INST_PROP(0, lut_initial);
#endif
#if DT_INST_NODE_HAS_PROP(0, lut_default)
static uint8_t ssd16xx_lut_default[] = DT_INST_PROP(0, lut_default);
#endif
#if DT_INST_NODE_HAS_PROP(0, softstart)
static uint8_t ssd16xx_softstart[] = DT_INST_PROP(0, softstart);
#endif
static uint8_t ssd16xx_gdv[] = DT_INST_PROP(0, gdv);
static uint8_t ssd16xx_sdv[] = DT_INST_PROP(0, sdv);

static inline int ssd16xx_write_cmd(struct ssd16xx_data *driver,
				    uint8_t cmd, uint8_t *data, size_t len)
{
	int err;
	struct spi_buf buf = {.buf = &cmd, .len = sizeof(cmd)};
	struct spi_buf_set buf_set = {.buffers = &buf, .count = 1};

	gpio_pin_set(driver->dc, SSD16XX_DC_PIN, 1);
	err = spi_write(driver->spi_dev, &driver->spi_config, &buf_set);
	if (err < 0) {
		return err;
	}

	if (data != NULL) {
		buf.buf = data;
		buf.len = len;
		gpio_pin_set(driver->dc, SSD16XX_DC_PIN, 0);
		err = spi_write(driver->spi_dev, &driver->spi_config, &buf_set);
		if (err < 0) {
			return err;
		}
	}

	return 0;
}

static inline void ssd16xx_busy_wait(struct ssd16xx_data *driver)
{
	int pin = gpio_pin_get(driver->busy, SSD16XX_BUSY_PIN);

	while (pin > 0) {
		__ASSERT(pin >= 0, "Failed to get pin level");
		k_msleep(SSD16XX_BUSY_DELAY);
		pin = gpio_pin_get(driver->busy, SSD16XX_BUSY_PIN);
	}
}

static inline size_t push_x_param(uint8_t *data, uint16_t x)
{
#if DT_INST_PROP(0, pp_width_bits) == 8
	data[0] = (uint8_t)x;
	return 1;
#elif DT_INST_PROP(0, pp_width_bits) == 16
	sys_put_le16(sys_cpu_to_le16(x), data);
	return 2;
#else
#error Unsupported pp_width_bits value for solomon,ssd16xxfb DTS instance 0
#endif
}

static inline size_t push_y_param(uint8_t *data, uint16_t y)
{
#if DT_INST_PROP(0, pp_height_bits) == 8
	data[0] = (uint8_t)y;
	return 1;
#elif DT_INST_PROP(0, pp_height_bits) == 16
	sys_put_le16(sys_cpu_to_le16(y), data);
	return 2;
#else
#error Unsupported pp_height_bits value for solomon,ssd16xxfb DTS instance 0
#endif
}


static inline int ssd16xx_set_ram_param(struct ssd16xx_data *driver,
					uint16_t sx, uint16_t ex, uint16_t sy, uint16_t ey)
{
	int err;
	uint8_t tmp[4];
	size_t len;

	len  = push_x_param(tmp, sx);
	len += push_x_param(tmp + len, ex);
	err = ssd16xx_write_cmd(driver, SSD16XX_CMD_RAM_XPOS_CTRL, tmp, len);
	if (err < 0) {
		return err;
	}

	len  = push_y_param(tmp, sy);
	len += push_y_param(tmp + len, ey);
	err = ssd16xx_write_cmd(driver, SSD16XX_CMD_RAM_YPOS_CTRL, tmp,	len);
	if (err < 0) {
		return err;
	}

	return 0;
}

static inline int ssd16xx_set_ram_ptr(struct ssd16xx_data *driver,
				      uint16_t x, uint16_t y)
{
	int err;
	uint8_t tmp[2];
	size_t len;

	len = push_x_param(tmp, x);
	err = ssd16xx_write_cmd(driver, SSD16XX_CMD_RAM_XPOS_CNTR, tmp, len);
	if (err < 0) {
		return err;
	}

	len = push_y_param(tmp, y);
	return ssd16xx_write_cmd(driver, SSD16XX_CMD_RAM_YPOS_CNTR, tmp, len);
}

static void ssd16xx_set_orientation_internall(struct ssd16xx_data *driver)

{
#if DT_INST_PROP(0, orientation_flipped) == 1
	driver->scan_mode = SSD16XX_DATA_ENTRY_XIYDY;
#else
	driver->scan_mode = SSD16XX_DATA_ENTRY_XDYIY;
#endif
}

static int ssd16xx_blanking_off(const struct device *dev)
{
	return -ENOTSUP;
}

static int ssd16xx_blanking_on(const struct device *dev)
{
	return -ENOTSUP;
}

static int ssd16xx_update_display(const struct device *dev)
{
	struct ssd16xx_data *driver = dev->data;
	int err;

	err = ssd16xx_write_cmd(driver, SSD16XX_CMD_UPDATE_CTRL2,
				&driver->update_cmd, 1);
	if (err < 0) {
		return err;
	}

	return ssd16xx_write_cmd(driver, SSD16XX_CMD_MASTER_ACTIVATION,
				 NULL, 0);
}

static int ssd16xx_write(const struct device *dev, const uint16_t x,
			 const uint16_t y,
			 const struct display_buffer_descriptor *desc,
			 const void *buf)
{
	struct ssd16xx_data *driver = dev->data;
	int err;
	size_t buf_len;
	uint16_t x_start;
	uint16_t x_end;
	uint16_t y_start;
	uint16_t y_end;
	uint16_t panel_h = EPD_PANEL_HEIGHT -
			EPD_PANEL_HEIGHT % EPD_PANEL_NUMOF_ROWS_PER_PAGE;

	if (desc->pitch < desc->width) {
		LOG_ERR("Pitch is smaller than width");
		return -EINVAL;
	}

	buf_len = MIN(desc->buf_size, desc->height * desc->width / 8);
	if (buf == NULL || buf_len == 0U) {
		LOG_ERR("Display buffer is not available");
		return -EINVAL;
	}

	if (desc->pitch > desc->width) {
		LOG_ERR("Unsupported mode");
		return -ENOTSUP;
	}

	if ((y + desc->height) > panel_h) {
		LOG_ERR("Buffer out of bounds (height)");
		return -EINVAL;
	}

	if ((x + desc->width) > EPD_PANEL_WIDTH) {
		LOG_ERR("Buffer out of bounds (width)");
		return -EINVAL;
	}

	if ((desc->height % EPD_PANEL_NUMOF_ROWS_PER_PAGE) != 0U) {
		LOG_ERR("Buffer height not multiple of %d",
				EPD_PANEL_NUMOF_ROWS_PER_PAGE);
		return -EINVAL;
	}

	if ((y % EPD_PANEL_NUMOF_ROWS_PER_PAGE) != 0U) {
		LOG_ERR("Y coordinate not multiple of %d",
				EPD_PANEL_NUMOF_ROWS_PER_PAGE);
		return -EINVAL;
	}

	switch (driver->scan_mode) {
	case SSD16XX_DATA_ENTRY_XIYDY:
		x_start = y / SSD16XX_PIXELS_PER_BYTE;
		x_end = (y + desc->height - 1) / SSD16XX_PIXELS_PER_BYTE;
		y_start = (x + desc->width - 1);
		y_end = x;
		break;

	case SSD16XX_DATA_ENTRY_XDYIY:
		x_start = (panel_h - 1 - y) / SSD16XX_PIXELS_PER_BYTE;
		x_end = (panel_h - 1 - (y + desc->height - 1)) /
			SSD16XX_PIXELS_PER_BYTE;
		y_start = x;
		y_end = (x + desc->width - 1);
		break;
	default:
		return -EINVAL;
	}

	ssd16xx_busy_wait(driver);

	err = ssd16xx_write_cmd(driver, SSD16XX_CMD_ENTRY_MODE,
				&driver->scan_mode, sizeof(driver->scan_mode));
	if (err < 0) {
		return err;
	}

	err = ssd16xx_set_ram_param(driver, x_start, x_end, y_start, y_end);
	if (err < 0) {
		return err;
	}

	err = ssd16xx_set_ram_ptr(driver, x_start, y_start);
	if (err < 0) {
		return err;
	}

	err = ssd16xx_write_cmd(driver, SSD16XX_CMD_WRITE_RAM, (uint8_t *)buf,
				buf_len);
	if (err < 0) {
		return err;
	}

	return ssd16xx_update_display(dev);
}

static int ssd16xx_read(const struct device *dev, const uint16_t x,
			const uint16_t y,
			const struct display_buffer_descriptor *desc,
			void *buf)
{
	LOG_ERR("not supported");
	return -ENOTSUP;
}

static void *ssd16xx_get_framebuffer(const struct device *dev)
{
	LOG_ERR("not supported");
	return NULL;
}

static int ssd16xx_set_brightness(const struct device *dev,
				  const uint8_t brightness)
{
	LOG_WRN("not supported");
	return -ENOTSUP;
}

static int ssd16xx_set_contrast(const struct device *dev, uint8_t contrast)
{
	LOG_WRN("not supported");
	return -ENOTSUP;
}

static void ssd16xx_get_capabilities(const struct device *dev,
				     struct display_capabilities *caps)
{
	memset(caps, 0, sizeof(struct display_capabilities));
	caps->x_resolution = EPD_PANEL_WIDTH;
	caps->y_resolution = EPD_PANEL_HEIGHT -
			     EPD_PANEL_HEIGHT % EPD_PANEL_NUMOF_ROWS_PER_PAGE;
	caps->supported_pixel_formats = PIXEL_FORMAT_MONO10;
	caps->current_pixel_format = PIXEL_FORMAT_MONO10;
	caps->screen_info = SCREEN_INFO_MONO_VTILED |
			    SCREEN_INFO_MONO_MSB_FIRST |
			    SCREEN_INFO_EPD |
			    SCREEN_INFO_DOUBLE_BUFFER;
}

static int ssd16xx_set_orientation(const struct device *dev,
				   const enum display_orientation
				   orientation)
{
	LOG_ERR("Unsupported");
	return -ENOTSUP;
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

static int ssd16xx_clear_cntlr_mem(const struct device *dev, uint8_t ram_cmd,
				   bool update)
{
	struct ssd16xx_data *driver = dev->data;
	uint8_t clear_page[EPD_PANEL_WIDTH];
	uint16_t panel_h = EPD_PANEL_HEIGHT /
			EPD_PANEL_NUMOF_ROWS_PER_PAGE;
	uint8_t scan_mode = SSD16XX_DATA_ENTRY_XIYDY;

	/*
	 * Clear unusable memory area when the resolution of the panel is not
	 * multiple of an octet.
	 */
	if (EPD_PANEL_HEIGHT % EPD_PANEL_NUMOF_ROWS_PER_PAGE) {
		panel_h += 1;
	}

	if (ssd16xx_write_cmd(driver, SSD16XX_CMD_ENTRY_MODE, &scan_mode, 1)) {
		return -EIO;
	}

	if (ssd16xx_set_ram_param(driver, SSD16XX_PANEL_FIRST_PAGE,
				  panel_h - 1,
				  SSD16XX_PANEL_LAST_GATE,
				  SSD16XX_PANEL_FIRST_GATE)) {
		return -EIO;
	}

	if (ssd16xx_set_ram_ptr(driver, SSD16XX_PANEL_FIRST_PAGE,
				SSD16XX_PANEL_LAST_GATE)) {
		return -EIO;
	}


	memset(clear_page, 0xff, sizeof(clear_page));
	for (int i = 0; i < panel_h; i++) {
		if (ssd16xx_write_cmd(driver, ram_cmd, clear_page,
				      sizeof(clear_page))) {
			return -EIO;
		}
	}

	if (update) {
		return ssd16xx_update_display(dev);
	}

	return 0;
}

static inline int ssd16xx_load_ws_from_otp(const struct device *dev)
{
	struct ssd16xx_data *driver = dev->data;
	uint8_t tmp[2];

#if DT_INST_NODE_HAS_PROP(0, tssv)
	/*
	 * Controller has an integrated temperature sensor or external
	 * temperature sensor is connected to the controller.
	 */
	LOG_INF("Select and load WS from OTP");

	tmp[0] = DT_INST_PROP(0, tssv);
	if (ssd16xx_write_cmd(driver,
			      SSD16XX_CMD_TSENSOR_SELECTION,
			      tmp, 1)) {
		return -EIO;
	}

	driver->update_cmd |= SSD16XX_CTRL2_LOAD_LUT |
			      SSD16XX_CTRL2_LOAD_TEMPERATURE;

	return 0;
#else
	int16_t t = (SSD16XX_DEFAULT_TR_VALUE * SSD16XX_TR_SCALE_FACTOR);

	LOG_INF("Load default WS (25 degrees Celsius) from OTP");

	tmp[0] = SSD16XX_CTRL2_ENABLE_CLK;
	if (ssd16xx_write_cmd(driver, SSD16XX_CMD_UPDATE_CTRL2,
			      tmp, 1)) {
		return -EIO;
	}

	if (ssd16xx_write_cmd(driver, SSD16XX_CMD_MASTER_ACTIVATION,
			      NULL, 0)) {
		return -EIO;
	}

	ssd16xx_busy_wait(driver);

	/* Load temperature value */
	sys_put_be16(t, tmp);
	if (ssd16xx_write_cmd(driver, SSD16XX_CMD_TSENS_CTRL,
			      tmp, 2)) {
		return -EIO;
	}

	tmp[0] = SSD16XX_CTRL2_DISABLE_CLK;
	if (ssd16xx_write_cmd(driver, SSD16XX_CMD_UPDATE_CTRL2,
			      tmp, 1)) {
		return -EIO;
	}

	if (ssd16xx_write_cmd(driver, SSD16XX_CMD_MASTER_ACTIVATION,
			      NULL, 0)) {
		return -EIO;
	}

	ssd16xx_busy_wait(driver);

	driver->update_cmd |= SSD16XX_CTRL2_LOAD_LUT;

	return 0;
#endif
}

static int ssd16xx_load_ws_initial(const struct device *dev)
{
#if DT_INST_NODE_HAS_PROP(0, lut_initial)
	struct ssd16xx_data *driver = dev->data;

	if (ssd16xx_write_cmd(driver, SSD16XX_CMD_UPDATE_LUT,
			      ssd16xx_lut_initial,
			      sizeof(ssd16xx_lut_initial))) {
		return -EIO;
	}

	ssd16xx_busy_wait(driver);
#else
	ssd16xx_load_ws_from_otp(dev);
#endif

	return 0;
}

static int ssd16xx_load_ws_default(const struct device *dev)
{
#if DT_INST_NODE_HAS_PROP(0, lut_default)
	struct ssd16xx_data *driver = dev->data;

	if (ssd16xx_write_cmd(driver, SSD16XX_CMD_UPDATE_LUT,
			      ssd16xx_lut_default,
			      sizeof(ssd16xx_lut_default))) {
		return -EIO;
	}

	ssd16xx_busy_wait(driver);
#endif

	return 0;
}

static int ssd16xx_controller_init(const struct device *dev)
{
	int err;
	uint8_t tmp[3];
	size_t len;
	struct ssd16xx_data *driver = dev->data;

	LOG_DBG("");

	gpio_pin_set(driver->reset, SSD16XX_RESET_PIN, 1);
	k_msleep(SSD16XX_RESET_DELAY);
	gpio_pin_set(driver->reset, SSD16XX_RESET_PIN, 0);
	k_msleep(SSD16XX_RESET_DELAY);
	ssd16xx_busy_wait(driver);

	err = ssd16xx_write_cmd(driver, SSD16XX_CMD_SW_RESET, NULL, 0);
	if (err < 0) {
		return err;
	}
	ssd16xx_busy_wait(driver);

	len = push_y_param(tmp, SSD16XX_PANEL_LAST_GATE);
	tmp[len++] = 0U;
	err = ssd16xx_write_cmd(driver, SSD16XX_CMD_GDO_CTRL, tmp, len);
	if (err < 0) {
		return err;
	}

#if DT_INST_NODE_HAS_PROP(0, softstart)
	err = ssd16xx_write_cmd(driver, SSD16XX_CMD_SOFTSTART,
				ssd16xx_softstart, sizeof(ssd16xx_softstart));
	if (err < 0) {
		return err;
	}
#endif

	err = ssd16xx_write_cmd(driver, SSD16XX_CMD_GDV_CTRL, ssd16xx_gdv,
				sizeof(ssd16xx_gdv));
	if (err < 0) {
		return err;
	}

	err = ssd16xx_write_cmd(driver, SSD16XX_CMD_SDV_CTRL, ssd16xx_sdv,
				sizeof(ssd16xx_sdv));
	if (err < 0) {
		return err;
	}

	tmp[0] = DT_INST_PROP(0, vcom);
	err = ssd16xx_write_cmd(driver, SSD16XX_CMD_VCOM_VOLTAGE, tmp, 1);
	if (err < 0) {
		return err;
	}

	tmp[0] = SSD16XX_VAL_DUMMY_LINE;
	err = ssd16xx_write_cmd(driver, SSD16XX_CMD_DUMMY_LINE, tmp, 1);
	if (err < 0) {
		return err;
	}

	tmp[0] = SSD16XX_VAL_GATE_LWIDTH;
	err = ssd16xx_write_cmd(driver, SSD16XX_CMD_GATE_LINE_WIDTH, tmp, 1);
	if (err < 0) {
		return err;
	}

	tmp[0] = DT_INST_PROP(0, border_waveform);
	err = ssd16xx_write_cmd(driver, SSD16XX_CMD_BWF_CTRL, tmp, 1);
	if (err < 0) {
		return err;
	}

	ssd16xx_set_orientation_internall(driver);
	driver->update_cmd = (SSD16XX_CTRL2_ENABLE_CLK |
			      SSD16XX_CTRL2_ENABLE_ANALOG |
			      SSD16XX_CTRL2_TO_PATTERN |
			      SSD16XX_CTRL2_DISABLE_ANALOG |
			      SSD16XX_CTRL2_DISABLE_CLK);

	if (ssd16xx_load_ws_initial(dev)) {
		return -EIO;
	}

	err = ssd16xx_clear_cntlr_mem(dev, SSD16XX_CMD_WRITE_RAM, true);
	if (err < 0) {
		return err;
	}

	ssd16xx_busy_wait(driver);

	err = ssd16xx_clear_cntlr_mem(dev, SSD16XX_CMD_WRITE_RED_RAM,
					     false);
	if (err < 0) {
		return err;
	}

	ssd16xx_busy_wait(driver);

	if (ssd16xx_load_ws_default(dev)) {
		return -EIO;
	}

	return ssd16xx_clear_cntlr_mem(dev, SSD16XX_CMD_WRITE_RAM, true);
}

static int ssd16xx_init(const struct device *dev)
{
	struct ssd16xx_data *driver = dev->data;

	LOG_DBG("");

	driver->spi_dev = device_get_binding(SSD16XX_BUS_NAME);
	if (driver->spi_dev == NULL) {
		LOG_ERR("Could not get SPI device for SSD16XX");
		return -EIO;
	}

	driver->spi_config.frequency = SSD16XX_SPI_FREQ;
	driver->spi_config.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8);
	driver->spi_config.slave = DT_INST_REG_ADDR(0);
	driver->spi_config.cs = NULL;

	driver->reset = device_get_binding(SSD16XX_RESET_CNTRL);
	if (driver->reset == NULL) {
		LOG_ERR("Could not get GPIO port for SSD16XX reset");
		return -EIO;
	}

	gpio_pin_configure(driver->reset, SSD16XX_RESET_PIN,
			   GPIO_OUTPUT_INACTIVE | SSD16XX_RESET_FLAGS);

	driver->dc = device_get_binding(SSD16XX_DC_CNTRL);
	if (driver->dc == NULL) {
		LOG_ERR("Could not get GPIO port for SSD16XX DC signal");
		return -EIO;
	}

	gpio_pin_configure(driver->dc, SSD16XX_DC_PIN,
			   GPIO_OUTPUT_INACTIVE | SSD16XX_DC_FLAGS);

	driver->busy = device_get_binding(SSD16XX_BUSY_CNTRL);
	if (driver->busy == NULL) {
		LOG_ERR("Could not get GPIO port for SSD16XX busy signal");
		return -EIO;
	}

	gpio_pin_configure(driver->busy, SSD16XX_BUSY_PIN,
			   GPIO_INPUT | SSD16XX_BUSY_FLAGS);

#if defined(SSD16XX_CS_CNTRL)
	driver->cs_ctrl.gpio_dev = device_get_binding(SSD16XX_CS_CNTRL);
	if (!driver->cs_ctrl.gpio_dev) {
		LOG_ERR("Unable to get SPI GPIO CS device");
		return -EIO;
	}

	driver->cs_ctrl.gpio_pin = SSD16XX_CS_PIN;
	driver->cs_ctrl.gpio_dt_flags = SSD16XX_CS_FLAGS;
	driver->cs_ctrl.delay = 0U;
	driver->spi_config.cs = &driver->cs_ctrl;
#endif

	return ssd16xx_controller_init(dev);
}

static struct ssd16xx_data ssd16xx_driver;

static struct display_driver_api ssd16xx_driver_api = {
	.blanking_on = ssd16xx_blanking_on,
	.blanking_off = ssd16xx_blanking_off,
	.write = ssd16xx_write,
	.read = ssd16xx_read,
	.get_framebuffer = ssd16xx_get_framebuffer,
	.set_brightness = ssd16xx_set_brightness,
	.set_contrast = ssd16xx_set_contrast,
	.get_capabilities = ssd16xx_get_capabilities,
	.set_pixel_format = ssd16xx_set_pixel_format,
	.set_orientation = ssd16xx_set_orientation,
};


DEVICE_AND_API_INIT(ssd16xx, DT_INST_LABEL(0), ssd16xx_init,
		    &ssd16xx_driver, NULL,
		    POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY,
		    &ssd16xx_driver_api);
