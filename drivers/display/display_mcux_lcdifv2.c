/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_lcdifv2

#include <zephyr/drivers/display.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <fsl_lcdifv2.h>

#ifdef CONFIG_HAS_MCUX_CACHE
#include <fsl_cache.h>
#endif

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>

LOG_MODULE_REGISTER(display_mcux_lcdifv2, CONFIG_DISPLAY_LOG_LEVEL);

/* Define the heap size. 512 bytes of padding are included for kernel heap structures */
K_HEAP_DEFINE(display_heap, CONFIG_MCUX_LCDIFV2_FB_NUM * CONFIG_MCUX_LCDIFV2_FB_SIZE + 512);

/* Layer index for display output */
#define LCDIFV2_LAYER_INDEX 0U

static const uint32_t mcux_lcdifv2_supported_fmts =
	PIXEL_FORMAT_RGB_565 | PIXEL_FORMAT_ARGB_8888 | PIXEL_FORMAT_RGB_888;

struct mcux_lcdifv2_config {
	LCDIFV2_Type *base;
	void (*irq_config_func)(const struct device *dev);
	lcdifv2_display_config_t display_config;
	const struct pinctrl_dev_config *pincfg;
	const struct gpio_dt_spec backlight_gpio;
};

struct mcux_lcdifv2_data {
	/* Pointer to active framebuffer */
	const uint8_t *active_fb;
	/* Pointers to driver allocated framebuffers */
	uint8_t *fb[CONFIG_MCUX_LCDIFV2_FB_NUM];
	enum display_pixel_format pixel_format;
	size_t pixel_bytes;
	size_t fb_bytes;
	struct k_sem sem;
	/* Tracks index of next active driver framebuffer */
	uint8_t next_idx;
};

static int mcux_lcdifv2_write(const struct device *dev, const uint16_t x, const uint16_t y,
			      const struct display_buffer_descriptor *desc, const void *buf)
{
	const struct mcux_lcdifv2_config *config = dev->config;
	struct mcux_lcdifv2_data *dev_data = dev->data;
	int h_idx;
	const uint8_t *src;
	uint8_t *dst;
	int ret = 0;
	bool full_fb = false;

	__ASSERT((dev_data->pixel_bytes * desc->pitch * desc->height) <= desc->buf_size,
		 "Input buffer too small");

	LOG_DBG("W=%d, H=%d, @%d,%d", desc->width, desc->height, x, y);

	if ((x == 0) && (y == 0) && (desc->width == config->display_config.panelWidth) &&
	    (desc->height == config->display_config.panelHeight) && (desc->pitch == desc->width)) {
		/* We can use the display buffer directly, no need to copy it */
		LOG_DBG("Setting FB from %p->%p", (void *)dev_data->active_fb, (void *)buf);
		dev_data->active_fb = buf;
		full_fb = true;
	} else {
		/* We must use partial framebuffer copy */
		if (CONFIG_MCUX_LCDIFV2_FB_NUM == 0) {
			LOG_ERR("Partial display refresh requires driver framebuffers");
			return -ENOTSUP;
		} else if (dev_data->active_fb != dev_data->fb[dev_data->next_idx]) {
			/*
			 * We must copy the entire current framebuffer to new
			 * buffer, since we will change the active buffer
			 * address
			 */
			src = dev_data->active_fb;
			dst = dev_data->fb[dev_data->next_idx];
			memcpy(dst, src, dev_data->fb_bytes);
		}
		/* Now, write the display update into active framebuffer */
		src = buf;
		dst = dev_data->fb[dev_data->next_idx];
		dst += dev_data->pixel_bytes * (y * config->display_config.panelWidth + x);

		for (h_idx = 0; h_idx < desc->height; h_idx++) {
			memcpy(dst, src, dev_data->pixel_bytes * desc->width);
			src += dev_data->pixel_bytes * desc->pitch;
			dst += dev_data->pixel_bytes * config->display_config.panelWidth;
		}

		LOG_DBG("Setting FB from %p->%p", (void *)dev_data->active_fb,
			(void *)dev_data->fb[dev_data->next_idx]);
		/* Set new active framebuffer */
		dev_data->active_fb = dev_data->fb[dev_data->next_idx];
	}

#ifdef CONFIG_HAS_MCUX_CACHE
	DCACHE_CleanByRange((uint32_t)dev_data->active_fb, dev_data->fb_bytes);
#endif

	/* Set framebuffer address for the layer */
	LCDIFV2_SetLayerBufferAddr(config->base, LCDIFV2_LAYER_INDEX,
				   (uint32_t)dev_data->active_fb);

#if CONFIG_MCUX_LCDIFV2_FB_NUM != 0
	/* Update index of active framebuffer */
	dev_data->next_idx = (dev_data->next_idx + 1) % CONFIG_MCUX_LCDIFV2_FB_NUM;
#endif

	/* Enable vertical blanking interrupt */
	LCDIFV2_EnableInterrupts(config->base, 0, kLCDIFV2_VerticalBlankingInterrupt);

	/* Enable the display output */
	LCDIFV2_EnableDisplay(config->base, true);

	/* Trigger shadow load to apply the buffer address change */
	LCDIFV2_TriggerLayerShadowLoad(config->base, LCDIFV2_LAYER_INDEX);

	/* Wait for frame send to complete */
	k_sem_take(&dev_data->sem, K_FOREVER);

	return ret;
}

static int mcux_lcdifv2_display_blanking_off(const struct device *dev)
{
	const struct mcux_lcdifv2_config *config = dev->config;

	LCDIFV2_EnableDisplay(config->base, true);

	return 0;
}

static int mcux_lcdifv2_display_blanking_on(const struct device *dev)
{
	const struct mcux_lcdifv2_config *config = dev->config;

	LCDIFV2_EnableDisplay(config->base, false);

	return 0;
}

static int mcux_lcdifv2_set_pixel_format(const struct device *dev,
					 const enum display_pixel_format pixel_format)
{
	struct mcux_lcdifv2_data *dev_data = dev->data;
	const struct mcux_lcdifv2_config *config = dev->config;

	if (!(pixel_format & mcux_lcdifv2_supported_fmts)) {
		LOG_ERR("Unsupported pixel format");
		return -ENOTSUP;
	}

	dev_data->pixel_format = pixel_format;
	dev_data->pixel_bytes = DISPLAY_BITS_PER_PIXEL(pixel_format) / BITS_PER_BYTE;
	dev_data->fb_bytes = config->display_config.panelWidth *
			     config->display_config.panelHeight * dev_data->pixel_bytes;

	/* Update frame buffer size according to the pixel format change. */
	for (int i = 0; i < CONFIG_MCUX_LCDIFV2_FB_NUM; i++) {
		k_heap_free(&display_heap, dev_data->fb[i]);
		dev_data->fb[i] =
			k_heap_aligned_alloc(&display_heap, 64, dev_data->fb_bytes, K_FOREVER);
		if (dev_data->fb[i] == NULL) {
			LOG_ERR("Could not allocate memory for framebuffers");
			return -ENOMEM;
		}
		memset(dev_data->fb[i], 0, dev_data->fb_bytes);
	}

	/* Set pixel format and buffer configuration for the layer */
	lcdifv2_buffer_config_t buffer_config = {0};

	buffer_config.strideBytes = config->display_config.panelWidth * dev_data->pixel_bytes;

	if (pixel_format == PIXEL_FORMAT_RGB_565) {
		buffer_config.pixelFormat = kLCDIFV2_PixelFormatRGB565;
	} else if (pixel_format == PIXEL_FORMAT_RGB_888) {
		buffer_config.pixelFormat = kLCDIFV2_PixelFormatRGB888;
	} else if (pixel_format == PIXEL_FORMAT_ARGB_8888) {
		buffer_config.pixelFormat = kLCDIFV2_PixelFormatARGB8888;
	}

	LCDIFV2_SetLayerBufferConfig(config->base, LCDIFV2_LAYER_INDEX, &buffer_config);

	return 0;
}

static int mcux_lcdifv2_set_orientation(const struct device *dev,
					const enum display_orientation orientation)
{
	if (orientation == DISPLAY_ORIENTATION_NORMAL) {
		return 0;
	}
	LOG_ERR("Changing display orientation not implemented");
	return -ENOTSUP;
}

static void mcux_lcdifv2_get_capabilities(const struct device *dev,
					  struct display_capabilities *capabilities)
{
	const struct mcux_lcdifv2_config *config = dev->config;

	memset(capabilities, 0, sizeof(struct display_capabilities));
	capabilities->x_resolution = config->display_config.panelWidth;
	capabilities->y_resolution = config->display_config.panelHeight;
	capabilities->supported_pixel_formats = mcux_lcdifv2_supported_fmts;
	capabilities->current_pixel_format = ((struct mcux_lcdifv2_data *)dev->data)->pixel_format;
	capabilities->current_orientation = DISPLAY_ORIENTATION_NORMAL;
}

static void mcux_lcdifv2_isr(const struct device *dev)
{
	const struct mcux_lcdifv2_config *config = dev->config;
	struct mcux_lcdifv2_data *dev_data = dev->data;
	uint32_t status;

	status = LCDIFV2_GetInterruptStatus(config->base, 0);
	LCDIFV2_ClearInterruptStatus(config->base, 0, status);

	if (status & kLCDIFV2_VerticalBlankingInterrupt) {
		/*
		 * Disable vertical blanking interrupt, post to
		 * sem to notify that frame send is complete.
		 */
		LCDIFV2_DisableInterrupts(config->base, 0, kLCDIFV2_VerticalBlankingInterrupt);
		k_sem_give(&dev_data->sem);
	}
}

static int mcux_lcdifv2_init(const struct device *dev)
{
	const struct mcux_lcdifv2_config *config = dev->config;
	struct mcux_lcdifv2_data *dev_data = dev->data;
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

	/* Initialize LCDIFV2 peripheral */
	LCDIFV2_Init(config->base);

	/* Initialize display configuration */
	LCDIFV2_SetDisplayConfig(config->base, &config->display_config);

	/* Set default pixel format obtained from device tree */
	mcux_lcdifv2_set_pixel_format(dev, dev_data->pixel_format);

	LCDIFV2_SetLayerSize(config->base, LCDIFV2_LAYER_INDEX, config->display_config.panelWidth,
			     config->display_config.panelHeight);
	LCDIFV2_SetLayerOffset(config->base, LCDIFV2_LAYER_INDEX, 0, 0);

	config->irq_config_func(dev);

#if CONFIG_MCUX_LCDIFV2_FB_NUM != 0
	dev_data->active_fb = dev_data->fb[0];
	LCDIFV2_SetLayerBufferAddr(config->base, LCDIFV2_LAYER_INDEX,
				   (uint32_t)dev_data->active_fb);
#endif

	/* Enable the layer */
	LCDIFV2_EnableLayer(config->base, LCDIFV2_LAYER_INDEX, true);

	return 0;
}

static DEVICE_API(display, mcux_lcdifv2_api) = {
	.blanking_on = mcux_lcdifv2_display_blanking_on,
	.blanking_off = mcux_lcdifv2_display_blanking_off,
	.write = mcux_lcdifv2_write,
	.get_capabilities = mcux_lcdifv2_get_capabilities,
	.set_pixel_format = mcux_lcdifv2_set_pixel_format,
	.set_orientation = mcux_lcdifv2_set_orientation,
};

#define MCUX_LCDIFV2_DEVICE_INIT(id)                                                               \
	PINCTRL_DT_INST_DEFINE(id);                                                                \
	static void mcux_lcdifv2_config_func_##id(const struct device *dev);                       \
	static const struct mcux_lcdifv2_config mcux_lcdifv2_config_##id = {                       \
		.base = (LCDIFV2_Type *)DT_INST_REG_ADDR(id),                                      \
		.irq_config_func = mcux_lcdifv2_config_func_##id,                                  \
		.display_config =                                                                  \
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
						 ? kLCDIFV2_HsyncActiveHigh                        \
						 : kLCDIFV2_HsyncActiveLow) |                      \
					(DT_PROP(DT_INST_CHILD(id, display_timings), vsync_active) \
						 ? kLCDIFV2_VsyncActiveHigh                        \
						 : kLCDIFV2_VsyncActiveLow) |                      \
					(DT_PROP(DT_INST_CHILD(id, display_timings), de_active)    \
						 ? kLCDIFV2_DataEnableActiveHigh                   \
						 : kLCDIFV2_DataEnableActiveLow) |                 \
					(DT_PROP(DT_INST_CHILD(id, display_timings),               \
						 pixelclk_active)                                  \
						 ? kLCDIFV2_DriveDataOnRisingClkEdge               \
						 : kLCDIFV2_DriveDataOnFallingClkEdge),            \
				.lineOrder = kLCDIFV2_LineOrderRGB,                                \
			},                                                                         \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(id),                                      \
		.backlight_gpio = GPIO_DT_SPEC_INST_GET_OR(id, backlight_gpios, {0}),              \
	};                                                                                         \
	static struct mcux_lcdifv2_data mcux_lcdifv2_data_##id = {                                 \
		.next_idx = 0,                                                                     \
		.pixel_format = DT_INST_PROP(id, pixel_format),                                    \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(id, &mcux_lcdifv2_init, NULL, &mcux_lcdifv2_data_##id,               \
			      &mcux_lcdifv2_config_##id, POST_KERNEL,                              \
			      CONFIG_DISPLAY_INIT_PRIORITY, &mcux_lcdifv2_api);                    \
	static void mcux_lcdifv2_config_func_##id(const struct device *dev)                        \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(id), DT_INST_IRQ(id, priority), mcux_lcdifv2_isr,         \
			    DEVICE_DT_INST_GET(id), 0);                                            \
		irq_enable(DT_INST_IRQN(id));                                                      \
	}

DT_INST_FOREACH_STATUS_OKAY(MCUX_LCDIFV2_DEVICE_INIT)
