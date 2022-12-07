/*
 * Copyright (c) 2019-22, NXP
 * Copyright (c) 2022, Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_elcdif

#include <zephyr/drivers/display.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <fsl_elcdif.h>

#ifdef CONFIG_HAS_MCUX_CACHE
#include <fsl_cache.h>
#endif

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>

LOG_MODULE_REGISTER(display_mcux_elcdif, CONFIG_DISPLAY_LOG_LEVEL);

#ifdef CONFIG_MCUX_ELCDIF_DOUBLE_FRAMEBUFFER
#define MCUX_ELCDIF_FB_NUM 2
#else
#define MCUX_ELCDIF_FB_NUM 1
#endif

struct mcux_elcdif_config {
	LCDIF_Type *base;
	void (*irq_config_func)(const struct device *dev);
	elcdif_rgb_mode_config_t rgb_mode;
	enum display_pixel_format pixel_format;
	size_t pixel_bytes;
	size_t fb_bytes;
	const struct pinctrl_dev_config *pincfg;
	const struct gpio_dt_spec backlight_gpio;
};

struct mcux_elcdif_data {
	uint8_t *fb_ptr;
	uint8_t *fb[MCUX_ELCDIF_FB_NUM];
	struct k_sem sem;
	uint8_t write_idx;
};

static int mcux_elcdif_write(const struct device *dev, const uint16_t x,
			     const uint16_t y,
			     const struct display_buffer_descriptor *desc,
			     const void *buf)
{
	const struct mcux_elcdif_config *config = dev->config;
	struct mcux_elcdif_data *dev_data = dev->data;

	uint8_t write_idx = (dev_data->write_idx + 1) % MCUX_ELCDIF_FB_NUM;

	int h_idx;
	const uint8_t *src;
	uint8_t *dst;

	__ASSERT((config->pixel_bytes * desc->pitch * desc->height) <=
		 desc->buf_size, "Input buffer too small");

	LOG_DBG("W=%d, H=%d, @%d,%d", desc->width, desc->height, x, y);

#ifdef CONFIG_MCUX_ELCDIF_DOUBLE_FRAMEBUFFER
	k_sem_take(&dev_data->sem, K_FOREVER);

	memcpy(dev_data->fb[write_idx], dev_data->fb[dev_data->write_idx],
	       config->fb_bytes);
#else
	/* wait for the next frame done */
	k_sem_reset(&dev_data->sem);
	k_sem_take(&dev_data->sem, K_FOREVER);
#endif

	src = buf;
	dst = dev_data->fb[write_idx];
	dst += config->pixel_bytes * (y * config->rgb_mode.panelWidth + x);

	for (h_idx = 0; h_idx < desc->height; h_idx++) {
		memcpy(dst, src, config->pixel_bytes * desc->width);
		src += config->pixel_bytes * desc->pitch;
		dst += config->pixel_bytes * config->rgb_mode.panelWidth;
	}

#ifdef CONFIG_HAS_MCUX_CACHE
	DCACHE_CleanByRange((uint32_t) dev_data->fb[write_idx],
			    config->fb_bytes);
#endif

#ifdef CONFIG_MCUX_ELCDIF_DOUBLE_FRAMEBUFFER
	ELCDIF_SetNextBufferAddr(config->base,
				 (uint32_t) dev_data->fb[write_idx]);
#endif

	dev_data->write_idx = write_idx;

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
#ifdef CONFIG_MCUX_ELCDIF_DOUBLE_FRAMEBUFFER
	LOG_ERR("Direct framebuffer access not available");
	return NULL;
#else
	struct mcux_elcdif_data *dev_data = dev->data;

	return dev_data->fb_ptr;
#endif
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
	const struct mcux_elcdif_config *config = dev->config;

	if (pixel_format == config->pixel_format) {
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

	memset(capabilities, 0, sizeof(struct display_capabilities));
	capabilities->x_resolution = config->rgb_mode.panelWidth;
	capabilities->y_resolution = config->rgb_mode.panelHeight;
	capabilities->supported_pixel_formats = config->pixel_format;
	capabilities->current_pixel_format = config->pixel_format;
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
	int err;

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
	if (config->pixel_format == PIXEL_FORMAT_BGR_565) {
		rgb_mode.pixelFormat = kELCDIF_PixelFormatRGB565;
	} else if (config->pixel_format == PIXEL_FORMAT_RGB_888) {
		rgb_mode.pixelFormat = kELCDIF_PixelFormatRGB888;
	}

	dev_data->fb[0] = dev_data->fb_ptr;
#ifdef CONFIG_MCUX_ELCDIF_DOUBLE_FRAMEBUFFER
	dev_data->fb[1] = dev_data->fb_ptr + config->fb_bytes;
#endif

	rgb_mode.bufferAddr = (uint32_t) dev_data->fb_ptr;

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

#define MCUX_ELCDIF_PIXEL_BYTES(id)						\
	COND_CODE_1(DT_INST_ENUM_IDX(id, pixel_format),	(2), (3))

#define MCUX_ELCDIF_DEVICE_INIT(id)						\
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
		.pixel_format = COND_CODE_1(DT_INST_ENUM_IDX(id, pixel_format),	\
			(PIXEL_FORMAT_BGR_565),					\
			(PIXEL_FORMAT_RGB_888)),				\
		.pixel_bytes = MCUX_ELCDIF_PIXEL_BYTES(id),			\
		.fb_bytes = DT_INST_PROP(id, width) * DT_INST_PROP(id, height)	\
			* MCUX_ELCDIF_PIXEL_BYTES(id),				\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(id),			\
		.backlight_gpio = GPIO_DT_SPEC_INST_GET(id, backlight_gpios),	\
	};									\
	static uint8_t __aligned(64) frame_buffer_##id[MCUX_ELCDIF_FB_NUM	\
			* DT_INST_PROP(id, width)				\
			* DT_INST_PROP(id, height)				\
			* MCUX_ELCDIF_PIXEL_BYTES(id)];				\
	static struct mcux_elcdif_data mcux_elcdif_data_##id = {		\
		.fb_ptr = frame_buffer_##id,					\
	};									\
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

DT_INST_FOREACH_STATUS_OKAY(MCUX_ELCDIF_DEVICE_INIT)
