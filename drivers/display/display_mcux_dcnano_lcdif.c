/*
 * Copyright 2023,2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_dcnano_lcdif

#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>

#include <fsl_lcdif.h>
#ifdef CONFIG_HAS_MCUX_CACHE
#include <fsl_cache.h>
#endif


LOG_MODULE_REGISTER(display_mcux_dcnano_lcdif, CONFIG_DISPLAY_LOG_LEVEL);

struct mcux_dcnano_lcdif_config {
	LCDIF_Type *base;
	void (*irq_config_func)(const struct device *dev);
	const struct gpio_dt_spec backlight_gpio;
	lcdif_dpi_config_t dpi_config;
	/* Pointer to start of first framebuffer */
	uint8_t *fb_ptr;
	/* Number of bytes used for each framebuffer */
	uint32_t fb_bytes;
};

struct mcux_dcnano_lcdif_data {
	/* Pointer to active framebuffer */
	const uint8_t *active_fb;
	uint8_t *fb[CONFIG_MCUX_DCNANO_LCDIF_FB_NUM];
	lcdif_fb_config_t fb_config;
	uint8_t pixel_bytes;
	struct k_sem sem;
	/* Tracks index of next active driver framebuffer */
	uint8_t next_idx;
};

static int mcux_dcnano_lcdif_write(const struct device *dev, const uint16_t x,
			     const uint16_t y,
			     const struct display_buffer_descriptor *desc,
			     const void *buf)
{
	const struct mcux_dcnano_lcdif_config *config = dev->config;
	struct mcux_dcnano_lcdif_data *data = dev->data;
	uint32_t h_idx;
	const uint8_t *src;
	uint8_t *dst;

	__ASSERT((data->pixel_bytes * desc->pitch * desc->height) <=
		desc->buf_size, "Input buffer too small");

	LOG_DBG("W=%d, H=%d @%d,%d", desc->width, desc->height, x, y);

	if ((x == 0) && (y == 0) &&
		(desc->width == config->dpi_config.panelWidth) &&
		(desc->height == config->dpi_config.panelHeight) &&
		(desc->pitch == desc->width)) {
		/* We can use the display buffer directly, without copying */
		LOG_DBG("Setting FB from %p->%p",
			(void *)data->active_fb, (void *)buf);
		data->active_fb = buf;
	} else {
		/* We must use partial framebuffer copy */
		if (CONFIG_MCUX_DCNANO_LCDIF_FB_NUM == 0)  {
			LOG_ERR("Partial display refresh requires driver framebuffers");
			return -ENOTSUP;
		} else if (data->active_fb != data->fb[data->next_idx]) {
			/*
			 * Copy the entirety of the current framebuffer to new
			 * buffer, since we are changing the active buffer address
			 */
			src = data->active_fb;
			dst = data->fb[data->next_idx];
			memcpy(dst, src, config->fb_bytes);
		}
		/* Write the display update to the active framebuffer */
		src = buf;
		dst = data->fb[data->next_idx];
		dst += data->pixel_bytes * (y * config->dpi_config.panelWidth + x);

		for (h_idx = 0; h_idx < desc->height; h_idx++) {
			memcpy(dst, src, data->pixel_bytes * desc->width);
			src += data->pixel_bytes * desc->pitch;
			dst += data->pixel_bytes * config->dpi_config.panelWidth;
		}
		LOG_DBG("Setting FB from %p->%p", (void *) data->active_fb,
			(void *) data->fb[data->next_idx]);
		/* Set new active framebuffer */
		data->active_fb = data->fb[data->next_idx];
	}

#if defined(CONFIG_HAS_MCUX_CACHE) && defined(CONFIG_MCUX_DCNANO_LCDIF_MAINTAIN_CACHE)
	CACHE64_CleanCacheByRange((uint32_t) data->active_fb,
					config->fb_bytes);
#endif

	k_sem_reset(&data->sem);

	/* Set new framebuffer */
	LCDIF_SetFrameBufferStride(config->base, 0,
		config->dpi_config.panelWidth * data->pixel_bytes);
	LCDIF_SetFrameBufferAddr(config->base, 0,
		(uint32_t)data->active_fb);
	LCDIF_SetFrameBufferConfig(config->base, 0, &data->fb_config);

#if DT_ENUM_IDX_OR(DT_NODELABEL(lcdif), version, 0) == 1
	LCDIF_Start(config->base);
	LCDIF_SetUpdateReady(config->base);
#endif

#if CONFIG_MCUX_DCNANO_LCDIF_FB_NUM != 0
	/* Update index of active framebuffer */
	data->next_idx = (data->next_idx + 1) % CONFIG_MCUX_DCNANO_LCDIF_FB_NUM;
#endif
	/* Wait for frame to complete */
	k_sem_take(&data->sem, K_FOREVER);

	return 0;
}

static void mcux_dcnano_lcdif_get_capabilities(const struct device *dev,
		struct display_capabilities *capabilities)
{
	const struct mcux_dcnano_lcdif_config *config = dev->config;
	struct mcux_dcnano_lcdif_data *data = dev->data;

	capabilities->y_resolution = config->dpi_config.panelHeight;
	capabilities->x_resolution = config->dpi_config.panelWidth;
	capabilities->supported_pixel_formats =
		(PIXEL_FORMAT_BGR_565 | PIXEL_FORMAT_ARGB_8888);
	capabilities->current_orientation = DISPLAY_ORIENTATION_NORMAL;
	switch (data->fb_config.format) {
	case kLCDIF_PixelFormatRGB565:
		/* Zephyr stores RGB565 as big endian, and LCDIF
		 * expects little endian. Use BGR565 format to resolve
		 * this.
		 */
		capabilities->current_pixel_format = PIXEL_FORMAT_BGR_565;
		break;
#if DT_ENUM_IDX_OR(DT_NODELABEL(lcdif), version, 0) == 1
	case kLCDIF_PixelFormatARGB8888:
#else
	case kLCDIF_PixelFormatXRGB8888:
#endif
		capabilities->current_pixel_format = PIXEL_FORMAT_ARGB_8888;
		break;
	default:
		/* Other LCDIF formats don't have a Zephyr enum yet */
		return;
	}
}

static void *mcux_dcnano_lcdif_get_framebuffer(const struct device *dev)
{
	struct mcux_dcnano_lcdif_data *data = dev->data;

	return (void *)data->active_fb;
}

static int mcux_dcnano_lcdif_display_blanking_off(const struct device *dev)
{
	const struct mcux_dcnano_lcdif_config *config = dev->config;

	return gpio_pin_set_dt(&config->backlight_gpio, 1);
}

static int mcux_dcnano_lcdif_display_blanking_on(const struct device *dev)
{
	const struct mcux_dcnano_lcdif_config *config = dev->config;

	return gpio_pin_set_dt(&config->backlight_gpio, 0);
}

static int mcux_dcnano_lcdif_set_pixel_format(const struct device *dev,
					const enum display_pixel_format
					pixel_format)
{
	struct mcux_dcnano_lcdif_data *data = dev->data;

	switch (pixel_format) {
	case PIXEL_FORMAT_BGR_565:
		/* Zephyr stores RGB565 as big endian, and LCDIF
		 * expects little endian. Use BGR565 format to resolve
		 * this.
		 */
		data->fb_config.format = kLCDIF_PixelFormatRGB565;
		data->pixel_bytes = 2;
		break;
	case PIXEL_FORMAT_ARGB_8888:
#if DT_ENUM_IDX_OR(DT_NODELABEL(lcdif), version, 0) == 1
		data->fb_config.format = kLCDIF_PixelFormatARGB8888;
#else
		data->fb_config.format = kLCDIF_PixelFormatXRGB8888;
#endif
		data->pixel_bytes = 4;
		break;
	default:
		return -ENOTSUP;
	}
	return 0;
}

static void mcux_dcnano_lcdif_isr(const struct device *dev)
{
	const struct mcux_dcnano_lcdif_config *config = dev->config;
	struct mcux_dcnano_lcdif_data *data = dev->data;
	uint32_t status;

	status = LCDIF_GetAndClearInterruptPendingFlags(config->base);

	if (0 != (status & kLCDIF_Display0FrameDoneInterrupt)) {
		k_sem_give(&data->sem);
	}
}

static int mcux_dcnano_lcdif_init(const struct device *dev)
{
	const struct mcux_dcnano_lcdif_config *config = dev->config;
	struct mcux_dcnano_lcdif_data *data = dev->data;
	int ret;

	ret = gpio_pin_configure_dt(&config->backlight_gpio, GPIO_OUTPUT_ACTIVE);
	if (ret) {
		return ret;
	}

	/* Convert pixel format from devicetree to the format used by HAL */
	ret = mcux_dcnano_lcdif_set_pixel_format(dev, data->fb_config.format);
	if (ret) {
		return ret;
	}

	LCDIF_Init(config->base);

	LCDIF_DpiModeSetConfig(config->base, 0, &config->dpi_config);

#if DT_ENUM_IDX_OR(DT_NODELABEL(lcdif), version, 0) == 1
	lcdif_panel_config_t panel_config;

	LCDIF_PanelGetDefaultConfig(&panel_config);
	LCDIF_SetPanelConfig(config->base, 0, &panel_config);
#endif

	LCDIF_EnableInterrupts(config->base, kLCDIF_Display0FrameDoneInterrupt);
	config->irq_config_func(dev);

	for (int i = 0; i < CONFIG_MCUX_DCNANO_LCDIF_FB_NUM; i++) {
		/* Record pointers to each driver framebuffer */
		data->fb[i] = config->fb_ptr + (config->fb_bytes * i);
	}
	data->active_fb = config->fb_ptr;

	k_sem_init(&data->sem, 1, 1);

#ifdef CONFIG_MCUX_DCNANO_LCDIF_EXTERNAL_FB_MEM
	/* Clear external memory, as it is uninitialized */
	memset(config->fb_ptr, 0, config->fb_bytes * CONFIG_MCUX_DCNANO_LCDIF_FB_NUM);
#endif

	return 0;
}

static DEVICE_API(display, mcux_dcnano_lcdif_api) = {
	.blanking_on = mcux_dcnano_lcdif_display_blanking_on,
	.blanking_off = mcux_dcnano_lcdif_display_blanking_off,
	.set_pixel_format = mcux_dcnano_lcdif_set_pixel_format,
	.write = mcux_dcnano_lcdif_write,
	.get_capabilities = mcux_dcnano_lcdif_get_capabilities,
	.get_framebuffer = mcux_dcnano_lcdif_get_framebuffer,
};

#define MCUX_DCNANO_LCDIF_PIXEL_BYTES(n)					\
	(DISPLAY_BITS_PER_PIXEL(DT_INST_PROP(n, pixel_format)) / BITS_PER_BYTE)
#define MCUX_DCNANO_LCDIF_FB_SIZE(n) DT_INST_PROP(n, width) *			\
	DT_INST_PROP(n, height) * MCUX_DCNANO_LCDIF_PIXEL_BYTES(n)

/* When using external framebuffer mem, we should not allocate framebuffers
 * in SRAM. Instead, we use external framebuffer address and size
 * from devicetree.
 */
#ifdef CONFIG_MCUX_DCNANO_LCDIF_EXTERNAL_FB_MEM
#define MCUX_DCNANO_LCDIF_FRAMEBUFFER_DECL(n)
#define MCUX_DCNANO_LCDIF_FRAMEBUFFER(n)					\
	(uint8_t *)CONFIG_MCUX_DCNANO_LCDIF_EXTERNAL_FB_ADDR
#else
#define MCUX_DCNANO_LCDIF_FRAMEBUFFER_DECL(n) uint8_t __aligned(LCDIF_FB_ALIGN)	\
		mcux_dcnano_lcdif_frame_buffer_##n[DT_INST_PROP(n, width) *	\
					 DT_INST_PROP(n, height) *		\
					 MCUX_DCNANO_LCDIF_PIXEL_BYTES(n) *	\
					 CONFIG_MCUX_DCNANO_LCDIF_FB_NUM]
#define MCUX_DCNANO_LCDIF_FRAMEBUFFER(n) mcux_dcnano_lcdif_frame_buffer_##n
#endif

#if DT_ENUM_IDX_OR(DT_NODELABEL(lcdif), version, 0) == 1
#define MCUX_DCNANO_LCDIF_FB_CONFIG(n)						\
		.fb_config = {							\
			.enable = true,						\
			.inOrder = kLCDIF_PixelInputOrderARGB,			\
			.rotateFlipMode = kLCDIF_Rotate0,			\
			.alpha.enable = false,					\
			.colorkey.enable = false,				\
			.topLeftX = 0U,						\
			.topLeftY = 0U,						\
			.width = DT_INST_PROP(n, width),			\
			.height = DT_INST_PROP(n, height),			\
			.format = DT_INST_PROP(n, pixel_format),		\
		},
#else
#define MCUX_DCNANO_LCDIF_FB_CONFIG(n)						\
		.fb_config = {							\
			.enable = true,						\
			.enableGamma = false,					\
			.format = DT_INST_PROP(n, pixel_format),		\
		},
#endif

#define MCUX_DCNANO_LCDIF_DEVICE_INIT(n)					\
	static void mcux_dcnano_lcdif_config_func_##n(const struct device *dev)	\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(n),					\
			   DT_INST_IRQ(n, priority),				\
			   mcux_dcnano_lcdif_isr,				\
			   DEVICE_DT_INST_GET(n),				\
			   0);							\
		irq_enable(DT_INST_IRQN(n));					\
	}									\
	MCUX_DCNANO_LCDIF_FRAMEBUFFER_DECL(n);					\
	struct mcux_dcnano_lcdif_data mcux_dcnano_lcdif_data_##n = {		\
		MCUX_DCNANO_LCDIF_FB_CONFIG(n)					\
		.next_idx = 0,							\
		.pixel_bytes = MCUX_DCNANO_LCDIF_PIXEL_BYTES(n),		\
	};									\
	struct mcux_dcnano_lcdif_config mcux_dcnano_lcdif_config_##n = {	\
		.base = (LCDIF_Type *) DT_INST_REG_ADDR(n),			\
		.irq_config_func = mcux_dcnano_lcdif_config_func_##n,		\
		.backlight_gpio = GPIO_DT_SPEC_INST_GET(n, backlight_gpios),	\
		.dpi_config = {							\
			.panelWidth = DT_INST_PROP(n, width),			\
			.panelHeight = DT_INST_PROP(n, height),			\
			.hsw = DT_PROP(DT_INST_CHILD(n, display_timings),	\
					hsync_len),				\
			.hfp = DT_PROP(DT_INST_CHILD(n, display_timings),	\
					hfront_porch),				\
			.hbp = DT_PROP(DT_INST_CHILD(n, display_timings),	\
					hback_porch),				\
			.vsw = DT_PROP(DT_INST_CHILD(n, display_timings),	\
					vsync_len),				\
			.vfp = DT_PROP(DT_INST_CHILD(n, display_timings),	\
					vfront_porch),				\
			.vbp = DT_PROP(DT_INST_CHILD(n, display_timings),	\
					vback_porch),				\
			.polarityFlags = (DT_PROP(DT_INST_CHILD(n,		\
					display_timings), de_active) ?		\
					kLCDIF_DataEnableActiveHigh :		\
					kLCDIF_DataEnableActiveLow) |		\
					(DT_PROP(DT_INST_CHILD(n,		\
					display_timings), pixelclk_active) ?	\
					kLCDIF_DriveDataOnRisingClkEdge :	\
					kLCDIF_DriveDataOnFallingClkEdge) |	\
					(DT_PROP(DT_INST_CHILD(n,		\
					display_timings), hsync_active) ?	\
					kLCDIF_HsyncActiveHigh :		\
					kLCDIF_HsyncActiveLow) |		\
					(DT_PROP(DT_INST_CHILD(n,		\
					display_timings), vsync_active) ?	\
					kLCDIF_VsyncActiveHigh :		\
					kLCDIF_VsyncActiveLow),			\
			.format = DT_INST_ENUM_IDX(n, data_bus_width),		\
		},								\
		.fb_ptr = MCUX_DCNANO_LCDIF_FRAMEBUFFER(n),			\
		.fb_bytes = MCUX_DCNANO_LCDIF_FB_SIZE(n),			\
	};									\
	DEVICE_DT_INST_DEFINE(n,						\
		&mcux_dcnano_lcdif_init,					\
		NULL,								\
		&mcux_dcnano_lcdif_data_##n,					\
		&mcux_dcnano_lcdif_config_##n,					\
		POST_KERNEL,							\
		CONFIG_DISPLAY_INIT_PRIORITY,					\
		&mcux_dcnano_lcdif_api);

DT_INST_FOREACH_STATUS_OKAY(MCUX_DCNANO_LCDIF_DEVICE_INIT)
