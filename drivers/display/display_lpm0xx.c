/*
 * Copyright (c) 2020 Rohit Gujarathi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT   jdi_lpm0xx

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lpm0xx, CONFIG_DISPLAY_LOG_LEVEL);

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/byteorder.h>

/* Supports: LPM013M126C, LPM009M360A
 */

/* Note:
 * -> Display expects MSB first
 */

#define LPM0XX_PANEL_WIDTH   DT_INST_PROP(0, width)
#define LPM0XX_PANEL_HEIGHT  DT_INST_PROP(0, height)

#define LPM0XX_PIXELS_PER_BYTE  2U

/* Adding 2 for the line number and dummy byte
 * line_buf format for each row.
 * +----------------+-------------------+-------------------+
 * | dummy (8 bits) | line num (8 bits) | data (WIDTH bits) |
 * +----------------+-------------------+-------------------+
 * First byte is actually 6 bits and line number 10 bits, but there
 * are actually only 176 lines so only 8 usable line bits. We thus
 * shift 2 bits up the dummy byte.
 */

#define LPM0XX_BYTES_PER_LINE  ((LPM0XX_PANEL_WIDTH / LPM0XX_PIXELS_PER_BYTE) + 2)

#define LPM0XX_BIT_FMT_OFFSET  2
#define LPM0XX_BIT_4BIT_FMT    (0x4 << LPM0XX_BIT_FMT_OFFSET)

#define LPM0XX_BIT_MODE_OFFSET 5
#define LPM0XX_BIT_WRITECMD    (0x4 << LPM0XX_BIT_MODE_OFFSET)
#define LPM0XX_BIT_VCOM        (0x2 << LPM0XX_BIT_MODE_OFFSET)
#define LPM0XX_BIT_CLEAR       (0x1 << LPM0XX_BIT_MODE_OFFSET)

struct lpm0xx_data {
#if DT_INST_NODE_HAS_PROP(0, disp_en_gpios)
	const struct device *disp_dev;
#endif
#if DT_INST_NODE_HAS_PROP(0, extcomin_gpios)
	const struct device *extcomin_dev;
#endif
	const struct device *spi_dev;
	struct spi_cs_control cs_ctrl;
	struct spi_config spi_config;
};


#if DT_INST_NODE_HAS_PROP(0, extcomin_gpios)
/* Driver will handle VCOM toggling */
static void lpm0xx_vcom_toggle(void *a, void *b, void *c)
{
	struct lpm0xx_data *driver = (struct lpm0xx_data *)a;

	while (1) {
		gpio_pin_toggle(driver->extcomin_dev,
		DT_INST_GPIO_PIN(0, extcomin_gpios));
		k_msleep(1000 / (DT_INST_PROP(0, extcomin_frequency) / 2));
		gpio_pin_toggle(driver->extcomin_dev,
		DT_INST_GPIO_PIN(0, extcomin_gpios));
		k_msleep(1000 / (DT_INST_PROP(0, extcomin_frequency) / 2));
	}
}

K_THREAD_STACK_DEFINE(vcom_toggle_stack, 256);
struct k_thread vcom_toggle_thread;
#endif

static int lpm0xx_blanking_off(const struct device *dev)
{
#if DT_INST_NODE_HAS_PROP(0, disp_en_gpios)
	struct lpm0xx_data *driver = dev->data;

	return gpio_pin_set(driver->disp_dev,
			    DT_INST_GPIO_PIN(0, disp_en_gpios), 1);
#else
	LOG_WRN("Unsupported");
	return -ENOTSUP;
#endif
}

static int lpm0xx_blanking_on(const struct device *dev)
{
#if DT_INST_NODE_HAS_PROP(0, disp_en_gpios)
	struct lpm0xx_data *driver = dev->data;

	return gpio_pin_set(driver->disp_dev,
			    DT_INST_GPIO_PIN(0, disp_en_gpios), 0);
#else
	LOG_WRN("Unsupported");
	return -ENOTSUP;
#endif
}

static int lpm0xx_cmd(const struct device *dev, uint8_t *buf, uint8_t len)
{
	struct lpm0xx_data *driver = dev->data;
	struct spi_buf cmd_buf = { .buf = buf, .len = len };
	struct spi_buf_set buf_set = { .buffers = &cmd_buf, .count = 1 };

	return spi_write(driver->spi_dev, &driver->spi_config, &buf_set);
}

static int lpm0xx_clear(const struct device *dev)
{
	struct lpm0xx_data *driver = dev->data;
	uint8_t clear_cmd[2] = { LPM0XX_BIT_CLEAR, 0 };
	int err;

	err = lpm0xx_cmd(dev, clear_cmd, sizeof(clear_cmd));
	spi_release(driver->spi_dev, &driver->spi_config);

	return err;
}

static int lpm0xx_update_display(const struct device *dev,
				uint16_t start_line,
				uint16_t num_lines,
				const uint8_t *data)
{
	struct lpm0xx_data *driver = dev->data;
	uint8_t write_cmd[2] = { LPM0XX_BIT_WRITECMD | LPM0XX_BIT_4BIT_FMT, 0 };
	uint8_t ln = start_line;
	struct spi_buf line_buf[2] = {
		{
			.len = sizeof(write_cmd),
			.buf = write_cmd,
		},
		{
			.len = LPM0XX_BYTES_PER_LINE - 2,
		},
	};
	struct spi_buf_set line_set = {
		.buffers = line_buf,
		.count = ARRAY_SIZE(line_buf),
	};
	int err = 0;

	LOG_DBG("Lines %d to %d", start_line, start_line + num_lines - 1);

	/* Send each line to the screen including
	 * the line number and dummy bits
	 */
	for (; ln <= start_line + num_lines - 1; ln++) {
		write_cmd[1] = ln;
		line_buf[1].buf = (uint8_t *)data;
		err |= spi_write(driver->spi_dev, &driver->spi_config,
				 &line_set);
		data += LPM0XX_PANEL_WIDTH / LPM0XX_PIXELS_PER_BYTE;
	}

	/* Send another trailing 16 bits for the last line
	 * These can be any bits, it does not matter
	 * just reusing the write_cmd buffer
	 */
	err |= lpm0xx_cmd(dev, write_cmd, sizeof(write_cmd));

	spi_release(driver->spi_dev, &driver->spi_config);

	return err;
}

/* Buffer width should be equal to display width */
static int lpm0xx_write(const struct device *dev, const uint16_t x,
		       const uint16_t y,
		       const struct display_buffer_descriptor *desc,
		       const void *buf)
{
	LOG_DBG("X: %d, Y: %d, W: %d, H: %d", x, y, desc->width, desc->height);

	if (buf == NULL) {
		LOG_WRN("Display buffer is not available");
		return -EINVAL;
	}

	if (desc->width != LPM0XX_PANEL_WIDTH) {
		LOG_ERR("Width not a multiple of %d", LPM0XX_PANEL_WIDTH);
		return -EINVAL;
	}

	if (desc->pitch != desc->width) {
		LOG_ERR("Unsupported mode");
		return -ENOTSUP;
	}

	if ((y + desc->height) > LPM0XX_PANEL_HEIGHT) {
		LOG_ERR("Buffer out of bounds (height)");
		return -EINVAL;
	}

	if (x != 0) {
		LOG_ERR("X-coordinate has to be 0");
		return -EINVAL;
	}

	/* Adding 1 since line numbering on the display starts with 1 */
	return lpm0xx_update_display(dev, y + 1, desc->height, buf);
}

static int lpm0xx_read(const struct device *dev, const uint16_t x,
		      const uint16_t y,
		      const struct display_buffer_descriptor *desc,
		      void *buf)
{
	LOG_ERR("not supported");
	return -ENOTSUP;
}

static void *lpm0xx_get_framebuffer(const struct device *dev)
{
	LOG_ERR("not supported");
	return NULL;
}

static int lpm0xx_set_brightness(const struct device *dev,
				const uint8_t brightness)
{
	LOG_WRN("not supported");
	return -ENOTSUP;
}

static int lpm0xx_set_contrast(const struct device *dev, uint8_t contrast)
{
	LOG_WRN("not supported");
	return -ENOTSUP;
}

static void lpm0xx_get_capabilities(const struct device *dev,
				   struct display_capabilities *caps)
{
	memset(caps, 0, sizeof(struct display_capabilities));
	caps->x_resolution = LPM0XX_PANEL_WIDTH;
	caps->y_resolution = LPM0XX_PANEL_HEIGHT;
	caps->supported_pixel_formats = PIXEL_FORMAT_RGBx_111;
	caps->current_pixel_format = PIXEL_FORMAT_RGBx_111;
	caps->screen_info = SCREEN_INFO_X_ALIGNMENT_WIDTH;
}

static int lpm0xx_set_orientation(const struct device *dev, const enum display_orientation orientation)
{
	LOG_ERR("Unsupported");
	return -ENOTSUP;
}

static int lpm0xx_set_pixel_format(const struct device *dev, const enum display_pixel_format pf)
{
	if (pf == PIXEL_FORMAT_RGBx_111) {
		return 0;
	}

	LOG_ERR("not supported");
	return -ENOTSUP;
}

static int lpm0xx_init(const struct device *dev)
{
	struct lpm0xx_data *driver = dev->data;

	driver->spi_dev = device_get_binding(DT_INST_BUS_LABEL(0));
	if (driver->spi_dev == NULL) {
		LOG_ERR("Could not get SPI device for LPM0XX");
		return -EIO;
	}

	driver->spi_config.frequency = DT_INST_PROP(0, spi_max_frequency);
	driver->spi_config.operation = SPI_OP_MODE_MASTER |
				       SPI_WORD_SET(8) |
				       SPI_TRANSFER_MSB |
				       SPI_CS_ACTIVE_HIGH |
				       SPI_HOLD_ON_CS |
				       SPI_LOCK_ON;
	driver->spi_config.slave = 0;

#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
	driver->cs_ctrl.gpio_dev = device_get_binding(
		DT_INST_SPI_DEV_CS_GPIOS_LABEL(0));
	if (driver->cs_ctrl.gpio_dev == NULL) {
		LOG_ERR("Could not get SPI device for LPM0XX");
		return -EIO;
	}
	driver->cs_ctrl.gpio_pin = DT_INST_SPI_DEV_CS_GPIOS_PIN(0);
	driver->cs_ctrl.gpio_dt_flags = DT_INST_SPI_DEV_CS_GPIOS_FLAGS(0);
	driver->cs_ctrl.delay = 0U;
	driver->spi_config.cs = &driver->cs_ctrl;
#endif

#if DT_INST_NODE_HAS_PROP(0, disp_en_gpios)
	driver->disp_dev = device_get_binding(
		DT_INST_GPIO_LABEL(0, disp_en_gpios));
	if (driver->disp_dev == NULL) {
		LOG_ERR("Could not get DISP pin port for LPM0XX");
		return -EIO;
	}
	LOG_INF("Configuring DISP pin to OUTPUT_HIGH");
	gpio_pin_configure(driver->disp_dev,
			   DT_INST_GPIO_PIN(0, disp_en_gpios),
			   GPIO_OUTPUT_HIGH);
#endif

#if DT_INST_NODE_HAS_PROP(0, extcomin_gpios)
	driver->extcomin_dev = device_get_binding(
		DT_INST_GPIO_LABEL(0, extcomin_gpios));
	if (driver->extcomin_dev == NULL) {
		LOG_ERR("Could not get EXTCOMIN pin port for LPM0XX");
		return -EIO;
	}
	LOG_INF("Configuring EXTCOMIN pin");
	gpio_pin_configure(driver->extcomin_dev,
			   DT_INST_GPIO_PIN(0, extcomin_gpios),
			   GPIO_OUTPUT_LOW);

	/* Start thread for toggling VCOM */
	k_tid_t vcom_toggle_tid = k_thread_create(&vcom_toggle_thread,
						  vcom_toggle_stack,
						  K_THREAD_STACK_SIZEOF(vcom_toggle_stack),
						  lpm0xx_vcom_toggle,
						  driver, NULL, NULL,
						  3, 0, K_NO_WAIT);
	k_thread_name_set(vcom_toggle_tid, "lpm0xx_vcom");
#endif  /* DT_INST_NODE_HAS_PROP(0, extcomin_gpios) */

	/* Clear display else it shows random data */
	return lpm0xx_clear(dev);
}

static struct lpm0xx_data lpm0xx_driver;

static struct display_driver_api lpm0xx_driver_api = {
	.blanking_on = lpm0xx_blanking_on,
	.blanking_off = lpm0xx_blanking_off,
	.write = lpm0xx_write,
	.read = lpm0xx_read,
	.get_framebuffer = lpm0xx_get_framebuffer,
	.set_brightness = lpm0xx_set_brightness,
	.set_contrast = lpm0xx_set_contrast,
	.get_capabilities = lpm0xx_get_capabilities,
	.set_pixel_format = lpm0xx_set_pixel_format,
	.set_orientation = lpm0xx_set_orientation,
};

DEVICE_DT_INST_DEFINE(0, lpm0xx_init, NULL, &lpm0xx_driver, NULL, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY, &lpm0xx_driver_api);
