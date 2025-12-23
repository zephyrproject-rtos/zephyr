/*
 * Copyright 2019-24, NXP
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

/* Define the heap size. 512 bytes of padding are included for kernel heap structures */
K_HEAP_DEFINE(display_heap, CONFIG_MCUX_ELCDIF_FB_NUM * CONFIG_MCUX_ELCDIF_FB_SIZE + 512);

static const uint32_t supported_fmts =
	PIXEL_FORMAT_RGB_565 | PIXEL_FORMAT_XRGB_8888 | PIXEL_FORMAT_RGB_888;

struct mcux_elcdif_config {
	LCDIF_Type *base;
	void (*irq_config_func)(const struct device *dev);
	elcdif_rgb_mode_config_t rgb_mode;
	const struct pinctrl_dev_config *pincfg;
	const struct gpio_dt_spec backlight_gpio;
};

struct mcux_elcdif_data {
	/* Pointer to active framebuffer */
	const uint8_t *active_fb;
	/* Pointers to driver allocated framebuffers */
	uint8_t *fb[CONFIG_MCUX_ELCDIF_FB_NUM];
	enum display_pixel_format pixel_format;
	size_t pixel_bytes;
	size_t fb_bytes;
	elcdif_rgb_mode_config_t rgb_mode;
	struct k_sem sem;
	/* Tracks index of next active driver framebuffer */
	uint8_t next_idx;
#ifndef CONFIG_MCUX_ELCDIF_START_ON_INIT
	bool running;
#endif
};

static int mcux_elcdif_write(const struct device *dev, const uint16_t x, const uint16_t y,
			     const struct display_buffer_descriptor *desc, const void *buf)
{
	const struct mcux_elcdif_config *config = dev->config;
	struct mcux_elcdif_data *dev_data = dev->data;
	int h_idx;
	const uint8_t *src;
	uint8_t *dst;
	int ret = 0;
	bool full_fb = false;

	__ASSERT((dev_data->pixel_bytes * desc->pitch * desc->height) <= desc->buf_size,
		 "Input buffer too small");

	LOG_DBG("W=%d, H=%d, @%d,%d", desc->width, desc->height, x, y);

	if ((x == 0) && (y == 0) && (desc->width == config->rgb_mode.panelWidth) &&
	    (desc->height == config->rgb_mode.panelHeight) && (desc->pitch == desc->width)) {
		/* We can use the display buffer directly, no need to copy it */
		LOG_DBG("Setting FB from %p->%p", (void *)dev_data->active_fb, (void *)buf);
		dev_data->active_fb = buf;
		full_fb = true;
	} else {
		/* We must use partial framebuffer copy */
		if (CONFIG_MCUX_ELCDIF_FB_NUM == 0) {
			LOG_ERR("Partial display refresh requires driver framebuffers");
			return -ENOTSUP;
		} else if (dev_data->active_fb != dev_data->fb[dev_data->next_idx]) {
			/*
			 * We must copy the entire current framebuffer to new
			 * buffer, since we wil change the active buffer
			 * address
			 */
			src = dev_data->active_fb;
			dst = dev_data->fb[dev_data->next_idx];
			memcpy(dst, src, dev_data->fb_bytes);
		}
		/* Now, write the display update into active framebuffer */
		src = buf;
		dst = dev_data->fb[dev_data->next_idx];
		dst += dev_data->pixel_bytes * (y * config->rgb_mode.panelWidth + x);

		for (h_idx = 0; h_idx < desc->height; h_idx++) {
			memcpy(dst, src, dev_data->pixel_bytes * desc->width);
			src += dev_data->pixel_bytes * desc->pitch;
			dst += dev_data->pixel_bytes * config->rgb_mode.panelWidth;
		}

		LOG_DBG("Setting FB from %p->%p", (void *)dev_data->active_fb,
			(void *)dev_data->fb[dev_data->next_idx]);
		/* Set new active framebuffer */
		dev_data->active_fb = dev_data->fb[dev_data->next_idx];
	}

#ifdef CONFIG_HAS_MCUX_CACHE
	DCACHE_CleanByRange((uint32_t)dev_data->active_fb, dev_data->fb_bytes);
#endif

	/* Queue next framebuffer */
	ELCDIF_SetNextBufferAddr(config->base, (uint32_t)dev_data->active_fb);

#if CONFIG_MCUX_ELCDIF_FB_NUM != 0
	/* Update index of active framebuffer */
	dev_data->next_idx = (dev_data->next_idx + 1) % CONFIG_MCUX_ELCDIF_FB_NUM;
#endif

#ifndef CONFIG_MCUX_ELCDIF_START_ON_INIT
	if (unlikely(!dev_data->running)) {
		ELCDIF_RgbModeStart(config->base);
		dev_data->running = true;
	}
#endif

	/* Enable frame buffer completion interrupt */
	ELCDIF_EnableInterrupts(config->base, kELCDIF_CurFrameDoneInterruptEnable);
	/* Wait for frame send to complete */
	k_sem_take(&dev_data->sem, K_FOREVER);
	return ret;
}

static int mcux_elcdif_display_blanking_off(const struct device *dev)
{
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(backlight_gpios)
	const struct mcux_elcdif_config *config = dev->config;
	if (config->backlight_gpio.port) {
		return gpio_pin_set_dt(&config->backlight_gpio, 1);
	}
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(backlight_gpios) */

	return -ENOSYS;
}

static int mcux_elcdif_display_blanking_on(const struct device *dev)
{
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(backlight_gpios)
	const struct mcux_elcdif_config *config = dev->config;
	if (config->backlight_gpio.port) {
		return gpio_pin_set_dt(&config->backlight_gpio, 0);
	}
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(backlight_gpios) */

	return -ENOSYS;
}

static int mcux_elcdif_set_pixel_format(const struct device *dev,
					const enum display_pixel_format pixel_format)
{
	struct mcux_elcdif_data *dev_data = dev->data;
	const struct mcux_elcdif_config *config = dev->config;

	if (!(pixel_format & supported_fmts)) {
		LOG_ERR("Unsupported pixel format");
		return -ENOTSUP;
	}

	dev_data->pixel_format = pixel_format;
	dev_data->pixel_bytes = DISPLAY_BITS_PER_PIXEL(pixel_format) / BITS_PER_BYTE;
	dev_data->fb_bytes =
		config->rgb_mode.panelWidth * config->rgb_mode.panelHeight * dev_data->pixel_bytes;

	for (int i = 0; i < CONFIG_MCUX_ELCDIF_FB_NUM; i++) {
		k_heap_free(&display_heap, dev_data->fb[i]);
		dev_data->fb[i] =
			k_heap_aligned_alloc(&display_heap, 64, dev_data->fb_bytes, K_FOREVER);
		if (dev_data->fb[i] == NULL) {
			LOG_ERR("Could not allocate memory for framebuffers");
			return -ENOMEM;
		}
		memset(dev_data->fb[i], 0, dev_data->fb_bytes);
	}

	dev_data->rgb_mode = config->rgb_mode;
	if (pixel_format == PIXEL_FORMAT_RGB_565) {
		dev_data->rgb_mode.pixelFormat = kELCDIF_PixelFormatRGB565;
	} else if (pixel_format == PIXEL_FORMAT_RGB_888) {
		dev_data->rgb_mode.pixelFormat = kELCDIF_PixelFormatRGB888;
	} else if (pixel_format == PIXEL_FORMAT_XRGB_8888) {
		dev_data->rgb_mode.pixelFormat = kELCDIF_PixelFormatXRGB8888;
	}

	ELCDIF_RgbModeSetPixelFormat(config->base, dev_data->rgb_mode.pixelFormat);

	return 0;
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
	capabilities->supported_pixel_formats = supported_fmts;
	capabilities->current_pixel_format = ((struct mcux_elcdif_data *)dev->data)->pixel_format;
	capabilities->current_orientation = DISPLAY_ORIENTATION_NORMAL;
}

static void mcux_elcdif_isr(const struct device *dev)
{
	const struct mcux_elcdif_config *config = dev->config;
	struct mcux_elcdif_data *dev_data = dev->data;
	uint32_t status;

	status = ELCDIF_GetInterruptStatus(config->base);
	ELCDIF_ClearInterruptStatus(config->base, status);
	if (config->base->CUR_BUF == ((uint32_t)dev_data->active_fb)) {
		/* Disable frame completion interrupt, post to
		 * sem to notify that frame send is complete.
		 */
		ELCDIF_DisableInterrupts(config->base, kELCDIF_CurFrameDoneInterruptEnable);
		k_sem_give(&dev_data->sem);
	}
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

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(backlight_gpios)
	if (config->backlight_gpio.port) {
		err = gpio_pin_configure_dt(&config->backlight_gpio, GPIO_OUTPUT_ACTIVE);
		if (err) {
			return err;
		}
	}
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(backlight_gpios) */

	k_sem_init(&dev_data->sem, 0, 1);

	config->irq_config_func(dev);

	/* Set default pixel format obtained from device tree */
	mcux_elcdif_set_pixel_format(dev, dev_data->pixel_format);

#if CONFIG_MCUX_ELCDIF_FB_NUM != 0
	dev_data->active_fb = dev_data->fb[0];
	dev_data->rgb_mode.bufferAddr = (uint32_t)dev_data->active_fb;
#endif

	ELCDIF_RgbModeInit(config->base, &dev_data->rgb_mode);
#ifdef CONFIG_MCUX_ELCDIF_START_ON_INIT
	ELCDIF_RgbModeStart(config->base);
#endif

	return 0;
}

static DEVICE_API(display, mcux_elcdif_api) = {
	.blanking_on = mcux_elcdif_display_blanking_on,
	.blanking_off = mcux_elcdif_display_blanking_off,
	.write = mcux_elcdif_write,
	.get_capabilities = mcux_elcdif_get_capabilities,
	.set_pixel_format = mcux_elcdif_set_pixel_format,
	.set_orientation = mcux_elcdif_set_orientation,
};

#define MCUX_ELCDIF_DEVICE_INIT(id)                                                                \
	PINCTRL_DT_INST_DEFINE(id);                                                                \
	static void mcux_elcdif_config_func_##id(const struct device *dev);                        \
	static const struct mcux_elcdif_config mcux_elcdif_config_##id = {                         \
		.base = (LCDIF_Type *)DT_INST_REG_ADDR(id),                                        \
		.irq_config_func = mcux_elcdif_config_func_##id,                                   \
		.rgb_mode =                                                                        \
			{                                                                          \
				.panelWidth = DT_INST_PROP(id, width),                             \
				.panelHeight = DT_INST_PROP(id, height),                           \
				.hsw = DT_PROP(DT_INST_CHILD(id, display_timings), hsync_len),     \
				.hfp = DT_PROP(DT_INST_CHILD(id, display_timings), hfront_porch),  \
				.hbp = DT_PROP(DT_INST_CHILD(id, display_timings), hback_porch),   \
				.vsw = DT_PROP(DT_INST_CHILD(id, display_timings), vsync_len),     \
				.vfp = DT_PROP(DT_INST_CHILD(id, display_timings), vfront_porch),  \
				.vbp = DT_PROP(DT_INST_CHILD(id, display_timings), vback_porch),   \
				.polarityFlags =                                                   \
					(DT_PROP(DT_INST_CHILD(id, display_timings), hsync_active) \
						 ? kELCDIF_HsyncActiveHigh                         \
						 : kELCDIF_HsyncActiveLow) |                       \
					(DT_PROP(DT_INST_CHILD(id, display_timings), vsync_active) \
						 ? kELCDIF_VsyncActiveHigh                         \
						 : kELCDIF_VsyncActiveLow) |                       \
					(DT_PROP(DT_INST_CHILD(id, display_timings), de_active)    \
						 ? kELCDIF_DataEnableActiveHigh                    \
						 : kELCDIF_DataEnableActiveLow) |                  \
					(DT_PROP(DT_INST_CHILD(id, display_timings),               \
						 pixelclk_active)                                  \
						 ? kELCDIF_DriveDataOnRisingClkEdge                \
						 : kELCDIF_DriveDataOnFallingClkEdge),             \
				.dataBus = LCDIF_CTRL_LCD_DATABUS_WIDTH(                           \
					DT_INST_ENUM_IDX(id, data_bus_width)),                     \
			},                                                                         \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(id),                                      \
		.backlight_gpio = GPIO_DT_SPEC_INST_GET_OR(id, backlight_gpios, {0}),              \
	};                                                                                         \
	static struct mcux_elcdif_data mcux_elcdif_data_##id = {                                   \
		.next_idx = 0,                                                                     \
		.pixel_format = DT_INST_PROP(id, pixel_format),                                    \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(id, &mcux_elcdif_init, NULL, &mcux_elcdif_data_##id,                 \
			      &mcux_elcdif_config_##id, POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY, \
			      &mcux_elcdif_api);                                                   \
	static void mcux_elcdif_config_func_##id(const struct device *dev)                         \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(id), DT_INST_IRQ(id, priority), mcux_elcdif_isr,          \
			    DEVICE_DT_INST_GET(id), 0);                                            \
		irq_enable(DT_INST_IRQN(id));                                                      \
	}

DT_INST_FOREACH_STATUS_OKAY(MCUX_ELCDIF_DEVICE_INIT)
