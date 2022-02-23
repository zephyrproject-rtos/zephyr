/*
 * Copyright 2019-22, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_elcdif

#include <zephyr/drivers/display.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/gpio.h>
#include <fsl_elcdif.h>

#ifdef CONFIG_HAS_MCUX_CACHE
#include <fsl_cache.h>
#endif

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(display_mcux_elcdif, CONFIG_DISPLAY_LOG_LEVEL);

/* Pixel formats */
#define MCUX_ELCDIF_PIXEL_FORMAT_RGB888 0U
#define MCUX_ELCDIF_PIXEL_FORMAT_BGR565 1U

K_HEAP_DEFINE(mcux_elcdif_pool,
	      CONFIG_MCUX_ELCDIF_POOL_BLOCK_MAX *
	      CONFIG_MCUX_ELCDIF_POOL_BLOCK_NUM);

struct mcux_elcdif_config {
	LCDIF_Type *base;
	void (*irq_config_func)(const struct device *dev);
	elcdif_rgb_mode_config_t rgb_mode;
	uint8_t pixel_format;
	const struct pinctrl_dev_config *pincfg;
	const struct gpio_dt_spec backlight_gpio;
};

struct mcux_mem_block {
	void *data;
};

struct mcux_elcdif_data {
	struct mcux_mem_block fb[CONFIG_MCUX_ELCDIF_POOL_BLOCK_NUM];
	struct k_sem sem;
	size_t pixel_bytes;
	size_t fb_bytes;
	uint8_t write_idx;
	enum display_pixel_format pixel_format;
};

static int mcux_elcdif_write(const struct device *dev, const uint16_t x,
			     const uint16_t y,
			     const struct display_buffer_descriptor *desc,
			     const void *buf)
{
	const struct mcux_elcdif_config *config = dev->config;
	struct mcux_elcdif_data *dev_data = dev->data;

	uint8_t write_idx = dev_data->write_idx;
	uint8_t read_idx = !write_idx;

	int h_idx;
	const uint8_t *src;
	uint8_t *dst;

	__ASSERT((dev_data->pixel_bytes * desc->pitch * desc->height) <=
		 desc->buf_size, "Input buffer too small");

	LOG_DBG("W=%d, H=%d, @%d,%d", desc->width, desc->height, x, y);

	k_sem_take(&dev_data->sem, K_FOREVER);

	memcpy(dev_data->fb[write_idx].data, dev_data->fb[read_idx].data,
	       dev_data->fb_bytes);

	src = buf;
	dst = dev_data->fb[dev_data->write_idx].data;
	dst += dev_data->pixel_bytes * (y * config->rgb_mode.panelWidth + x);

	for (h_idx = 0; h_idx < desc->height; h_idx++) {
		memcpy(dst, src, dev_data->pixel_bytes * desc->width);
		src += dev_data->pixel_bytes * desc->pitch;
		dst += dev_data->pixel_bytes * config->rgb_mode.panelWidth;
	}

#ifdef CONFIG_HAS_MCUX_CACHE
	DCACHE_CleanByRange((uint32_t) dev_data->fb[write_idx].data,
			    dev_data->fb_bytes);
#endif

	ELCDIF_SetNextBufferAddr(config->base,
				 (uint32_t) dev_data->fb[write_idx].data);

	dev_data->write_idx = read_idx;

	return 0;
}

static int mcux_elcdif_read(const struct device *dev, const uint16_t x,
			    const uint16_t y,
			    const struct display_buffer_descriptor *desc,
			    void *buf)
{
	LOG_ERR("Read not implemented");
	return -ENOTSUP;
}

static void *mcux_elcdif_get_framebuffer(const struct device *dev)
{
	LOG_ERR("Direct framebuffer access not implemented");
	return NULL;
}

static int mcux_elcdif_display_blanking_off(const struct device *dev)
{
	const struct mcux_elcdif_config *config = dev->config;

	return gpio_pin_set_dt(&config->backlight_gpio, 1);
}

static int mcux_elcdif_display_blanking_on(const struct device *dev)
{
	const struct mcux_elcdif_config *config = dev->config;

	return gpio_pin_set_dt(&config->backlight_gpio, 0);
}

static int mcux_elcdif_set_brightness(const struct device *dev,
				      const uint8_t brightness)
{
	LOG_WRN("Set brightness not implemented");
	return -ENOTSUP;
}

static int mcux_elcdif_set_contrast(const struct device *dev,
				    const uint8_t contrast)
{
	LOG_ERR("Set contrast not implemented");
	return -ENOTSUP;
}

static int mcux_elcdif_set_pixel_format(const struct device *dev,
					const enum display_pixel_format
					pixel_format)
{
	struct mcux_elcdif_data *dev_data = dev->data;

	if (pixel_format == dev_data->pixel_format) {
		return 0;
	}
	LOG_ERR("Pixel format change not implemented");
	return -ENOTSUP;
}

static int mcux_elcdif_set_orientation(const struct device *dev,
		const enum display_orientation orientation)
{
	if (orientation == DISPLAY_ORIENTATION_NORMAL) {
		return 0;
	}
	LOG_ERR("Changing display orientation not implemented");
	return -ENOTSUP;
}

static void mcux_elcdif_get_capabilities(const struct device *dev,
		struct display_capabilities *capabilities)
{
	const struct mcux_elcdif_config *config = dev->config;
	struct mcux_elcdif_data *dev_data = dev->data;

	memset(capabilities, 0, sizeof(struct display_capabilities));
	capabilities->x_resolution = config->rgb_mode.panelWidth;
	capabilities->y_resolution = config->rgb_mode.panelHeight;
	capabilities->supported_pixel_formats = dev_data->pixel_format;
	capabilities->current_pixel_format = dev_data->pixel_format;
	capabilities->current_orientation = DISPLAY_ORIENTATION_NORMAL;
}

static void mcux_elcdif_isr(const struct device *dev)
{
	const struct mcux_elcdif_config *config = dev->config;
	struct mcux_elcdif_data *dev_data = dev->data;
	uint32_t status;

	status = ELCDIF_GetInterruptStatus(config->base);
	ELCDIF_ClearInterruptStatus(config->base, status);

	k_sem_give(&dev_data->sem);
}

static int mcux_elcdif_init(const struct device *dev)
{
	const struct mcux_elcdif_config *config = dev->config;
	struct mcux_elcdif_data *dev_data = dev->data;
	int i, err;

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		return err;
	}

	err = gpio_pin_configure_dt(&config->backlight_gpio, GPIO_OUTPUT_ACTIVE);
	if (err) {
		return err;
	}

	elcdif_rgb_mode_config_t rgb_mode = config->rgb_mode;

	/* Shift the polarity bits to the appropriate location in the register */
	rgb_mode.polarityFlags = rgb_mode.polarityFlags << LCDIF_VDCTRL0_ENABLE_POL_SHIFT;

	/* Set the Pixel format */
	if (config->pixel_format == MCUX_ELCDIF_PIXEL_FORMAT_BGR565) {
		rgb_mode.pixelFormat = kELCDIF_PixelFormatRGB565;
		dev_data->pixel_format = PIXEL_FORMAT_BGR_565;
		dev_data->pixel_bytes = 2;
	} else if (config->pixel_format == MCUX_ELCDIF_PIXEL_FORMAT_RGB888) {
		rgb_mode.pixelFormat = kELCDIF_PixelFormatRGB888;
		dev_data->pixel_format = PIXEL_FORMAT_RGB_888;
		dev_data->pixel_bytes = 3;
	}

	dev_data->fb_bytes = dev_data->pixel_bytes *
			 rgb_mode.panelWidth * rgb_mode.panelHeight;
	dev_data->write_idx = 1U;

	for (i = 0; i < ARRAY_SIZE(dev_data->fb); i++) {
		dev_data->fb[i].data = k_heap_alloc(&mcux_elcdif_pool,
						dev_data->fb_bytes, K_NO_WAIT);
		if (dev_data->fb[i].data == NULL) {
			LOG_ERR("Could not allocate frame buffer %d", i);
			return -ENOMEM;
		}
		memset(dev_data->fb[i].data, 0, dev_data->fb_bytes);
	}
	rgb_mode.bufferAddr = (uint32_t) dev_data->fb[0].data;

	k_sem_init(&dev_data->sem, 1, 1);

	config->irq_config_func(dev);

	ELCDIF_RgbModeInit(config->base, &rgb_mode);
	ELCDIF_EnableInterrupts(config->base,
				kELCDIF_CurFrameDoneInterruptEnable);
	ELCDIF_RgbModeStart(config->base);

	return 0;
}

static const struct display_driver_api mcux_elcdif_api = {
	.blanking_on = mcux_elcdif_display_blanking_on,
	.blanking_off = mcux_elcdif_display_blanking_off,
	.write = mcux_elcdif_write,
	.read = mcux_elcdif_read,
	.get_framebuffer = mcux_elcdif_get_framebuffer,
	.set_brightness = mcux_elcdif_set_brightness,
	.set_contrast = mcux_elcdif_set_contrast,
	.get_capabilities = mcux_elcdif_get_capabilities,
	.set_pixel_format = mcux_elcdif_set_pixel_format,
	.set_orientation = mcux_elcdif_set_orientation,
};

#define MCUX_ELDCIF_DEVICE(id)							\
	PINCTRL_DT_INST_DEFINE(id);						\
	static void mcux_elcdif_config_func_##id(const struct device *dev);	\
	static const struct mcux_elcdif_config mcux_elcdif_config_##id = {	\
		.base = (LCDIF_Type *) DT_INST_REG_ADDR(id),			\
		.irq_config_func = mcux_elcdif_config_func_##id,		\
		.rgb_mode = {							\
			.panelWidth = DT_INST_PROP(id, width),			\
			.panelHeight = DT_INST_PROP(id, height),		\
			.hsw = DT_INST_PROP(id, hsync),				\
			.hfp = DT_INST_PROP(id, hfp),				\
			.hbp = DT_INST_PROP(id, hbp),				\
			.vsw = DT_INST_PROP(id, vsync),				\
			.vfp = DT_INST_PROP(id, vfp),				\
			.vbp = DT_INST_PROP(id, vbp),				\
			.polarityFlags = DT_INST_PROP(id, polarity),		\
			.dataBus = LCDIF_CTRL_LCD_DATABUS_WIDTH(		\
					DT_INST_ENUM_IDX(id, data_buswidth)),	\
		},								\
		.pixel_format = DT_INST_ENUM_IDX(id, pixel_format),		\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(id),			\
		.backlight_gpio = GPIO_DT_SPEC_INST_GET(id, backlight_gpios),	\
	};									\
	static struct mcux_elcdif_data mcux_elcdif_data_##id;			\
	DEVICE_DT_INST_DEFINE(id,						\
			    &mcux_elcdif_init,					\
			    NULL,						\
			    &mcux_elcdif_data_##id,				\
			    &mcux_elcdif_config_##id,				\
			    POST_KERNEL,					\
			    CONFIG_DISPLAY_INIT_PRIORITY,			\
			    &mcux_elcdif_api);					\
	static void mcux_elcdif_config_func_##id(const struct device *dev)	\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(id),					\
			    DT_INST_IRQ(id, priority),				\
			    mcux_elcdif_isr,					\
			    DEVICE_DT_INST_GET(id),				\
			    0);							\
		irq_enable(DT_INST_IRQN(id));					\
	}

DT_INST_FOREACH_STATUS_OKAY(MCUX_ELDCIF_DEVICE)
