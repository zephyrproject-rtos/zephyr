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

/* Layer address shall be 64-byte aligned */
#define LCDIFV2_FB_ALIGN 64U

/* Every layer scans these out; packed YUV 4:2:2 only the layers that have a CSC */
#define LCDIFV2_RGB_FORMATS                                                                        \
	(PIXEL_FORMAT_RGB_565 | PIXEL_FORMAT_RGB_888 | PIXEL_FORMAT_ARGB_8888 |                    \
	 PIXEL_FORMAT_XRGB_8888 | PIXEL_FORMAT_ABGR_8888)
#define LCDIFV2_CSC_FORMATS PIXEL_FORMAT_YUYV

/*
 * Global alpha (AlphaOverride) and per-pixel alpha (AlphaEmbedded) are the two
 * blends the layer stack composes; scaling the pixel alpha by a plane alpha and
 * premultiplied color need the Porter-Duff factors, which pair two layers.
 */
#define LCDIFV2_BLEND_MODES (DISPLAY_BLEND_NONE | DISPLAY_BLEND_COVERAGE)

/* Layer size and position fields are 12-bit, the stride 16-bit */
#define LCDIFV2_MAX_DIM 0xFFFU

/* Interrupts are routed to CPU domain 0 */
#define LCDIFV2_INT_DOMAIN 0U

/* Longer than any frame: a VSYNC that does not come means the display is not running */
#define LCDIFV2_VSYNC_TIMEOUT K_MSEC(200)

struct mcux_lcdifv2_config {
	LCDIFV2_Type *base;
	void (*irq_config_func)(const struct device *dev);
	lcdifv2_display_config_t display_config;
	const struct pinctrl_dev_config *pincfg;
	const struct gpio_dt_spec backlight_gpio;
};

/* A layer: what the next commit programs, and the buffer the layer reads now */
struct mcux_lcdifv2_plane {
	struct display_plane_config staged;
	const void *held;
	bool dirty;
	bool enabled;
};

struct mcux_lcdifv2_data {
	/* Pointers to driver allocated framebuffers */
	uint8_t *fb[CONFIG_MCUX_LCDIFV2_FB_NUM];
	enum display_pixel_format pixel_format;
	size_t pixel_bytes;
	size_t fb_bytes;
	/* Tracks index of next active driver framebuffer */
	uint8_t next_idx;
	struct mcux_lcdifv2_plane planes[LCDIFV2_LAYER_COUNT];
	/* Serializes staging and commits across the threads driving the planes */
	struct k_mutex lock;
	struct k_sem vsync_sem;
	bool display_on;
};

static int mcux_lcdifv2_hal_format(enum display_pixel_format fmt, lcdifv2_pixel_format_t *out)
{
	switch (fmt) {
	case PIXEL_FORMAT_RGB_565:
		*out = kLCDIFV2_PixelFormatRGB565;
		break;
	case PIXEL_FORMAT_RGB_888:
		*out = kLCDIFV2_PixelFormatRGB888;
		break;
	case PIXEL_FORMAT_ARGB_8888:
	case PIXEL_FORMAT_XRGB_8888:
		*out = kLCDIFV2_PixelFormatARGB8888;
		break;
	case PIXEL_FORMAT_ABGR_8888:
		*out = kLCDIFV2_PixelFormatABGR8888;
		break;
	case PIXEL_FORMAT_YUYV:
		*out = kLCDIFV2_PixelFormatYUYV;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

/* The layer fetches 32 bits at a time: a line holds a whole number of fetches */
static uint8_t mcux_lcdifv2_width_align(enum display_pixel_format fmt)
{
	switch (DISPLAY_BITS_PER_PIXEL(fmt)) {
	case 16:
		return 2U;
	case 24:
		return 4U;
	default:
		return 1U;
	}
}

static uint32_t mcux_lcdifv2_plane_formats(uint8_t plane)
{
	return LCDIFV2_RGB_FORMATS | ((plane < LCDIFV2_LAYER_CSC_COUNT) ? LCDIFV2_CSC_FORMATS : 0U);
}

/*
 * Map a plane's blend mode and alpha onto the layer's alpha mode, or refuse.
 * Opaque is a global alpha of 255, never kLCDIFV2_AlphaDisable: that mode
 * needs SAFETY_EN, and a safety layer is left out of the blend, which hides
 * the layers above it. ERR051315: the blend covers the whole layer area,
 * not only where two layers overlap.
 */
static int mcux_lcdifv2_blend_config(uint8_t plane, const struct display_plane_config *cfg,
				     lcdifv2_blend_config_t *blend)
{
	*blend = (lcdifv2_blend_config_t){.globalAlpha = 255U, .alphaMode = kLCDIFV2_AlphaOverride};

	/* Layer 0 is the background: nothing to blend over */
	if (plane == 0U) {
		return 0;
	}

	if (cfg->blend_mode == DISPLAY_BLEND_NONE) {
		blend->globalAlpha = cfg->alpha;
		return 0;
	}

	/* The X byte of XRGB would be read as the pixel alpha */
	if (cfg->blend_mode == DISPLAY_BLEND_COVERAGE && cfg->alpha == 255U &&
	    cfg->pixel_format != PIXEL_FORMAT_XRGB_8888) {
		blend->alphaMode = kLCDIFV2_AlphaEmbedded;
		return 0;
	}

	return -ENOTSUP;
}

/* All of it lands in the shadow registers until the layer's shadow load */
static void mcux_lcdifv2_program_layer(const struct device *dev, uint8_t plane)
{
	const struct mcux_lcdifv2_config *config = dev->config;
	struct mcux_lcdifv2_data *dev_data = dev->data;
	const struct display_plane_config *cfg = &dev_data->planes[plane].staged;
	lcdifv2_buffer_config_t buffer_config = {0};
	lcdifv2_blend_config_t blend;

	if (cfg->buf == NULL) {
		LCDIFV2_EnableLayer(config->base, plane, false);
		return;
	}

	(void)mcux_lcdifv2_hal_format(cfg->pixel_format, &buffer_config.pixelFormat);
	buffer_config.strideBytes =
		cfg->desc.pitch * DISPLAY_BITS_PER_PIXEL(cfg->pixel_format) / BITS_PER_BYTE;
	(void)mcux_lcdifv2_blend_config(plane, cfg, &blend);

	LCDIFV2_SetLayerSize(config->base, plane, cfg->desc.width, cfg->desc.height);
	LCDIFV2_SetLayerOffset(config->base, plane, cfg->x, cfg->y);
	LCDIFV2_SetLayerBufferConfig(config->base, plane, &buffer_config);
	if (plane < LCDIFV2_LAYER_CSC_COUNT) {
		LCDIFV2_SetCscMode(config->base, plane,
				   (cfg->pixel_format == PIXEL_FORMAT_YUYV) ? kLCDIFV2_CscYCbCr2RGB
									    : kLCDIFV2_CscDisable);
	}
	LCDIFV2_SetLayerBlendConfig(config->base, plane, &blend);
#ifdef CONFIG_HAS_MCUX_CACHE
	DCACHE_CleanByRange((uint32_t)cfg->buf, cfg->desc.buf_size);
#endif
	LCDIFV2_SetLayerBufferAddr(config->base, plane, (uint32_t)cfg->buf);
	LCDIFV2_EnableLayer(config->base, plane, true);
}

/*
 * Program every dirty layer, trigger their shadow loads together so they latch
 * at the same VSYNC, and wait for that VSYNC: from then on the new buffers are
 * read and the previous ones are free. A blanked display latches at its first
 * VSYNC after blanking off, so there is nothing to wait for.
 */
static int mcux_lcdifv2_commit_locked(const struct device *dev)
{
	const struct mcux_lcdifv2_config *config = dev->config;
	struct mcux_lcdifv2_data *dev_data = dev->data;
	bool any = false;
	int ret = 0;

	for (uint8_t i = 0; i < LCDIFV2_LAYER_COUNT; i++) {
		if (dev_data->planes[i].dirty) {
			mcux_lcdifv2_program_layer(dev, i);
			any = true;
		}
	}
	if (!any) {
		return 0;
	}

	for (uint8_t i = 0; i < LCDIFV2_LAYER_COUNT; i++) {
		if (dev_data->planes[i].dirty) {
			LCDIFV2_TriggerLayerShadowLoad(config->base, i);
		}
	}

	if (dev_data->display_on) {
		k_sem_reset(&dev_data->vsync_sem);
		LCDIFV2_ClearInterruptStatus(config->base, LCDIFV2_INT_DOMAIN,
					     kLCDIFV2_VsyncEdgeInterrupt);
		LCDIFV2_EnableInterrupts(config->base, LCDIFV2_INT_DOMAIN,
					 kLCDIFV2_VsyncEdgeInterrupt);
		if (k_sem_take(&dev_data->vsync_sem, LCDIFV2_VSYNC_TIMEOUT) != 0) {
			LCDIFV2_DisableInterrupts(config->base, LCDIFV2_INT_DOMAIN,
						  kLCDIFV2_VsyncEdgeInterrupt);
			LOG_ERR("No VSYNC: display not running");
			ret = -EIO;
		}
	}

	for (uint8_t i = 0; i < LCDIFV2_LAYER_COUNT; i++) {
		struct mcux_lcdifv2_plane *plane = &dev_data->planes[i];

		if (plane->dirty) {
			plane->held = plane->staged.buf;
			plane->enabled = plane->staged.buf != NULL;
			plane->dirty = false;
		}
	}

	return ret;
}

static int mcux_lcdifv2_get_plane_capabilities(const struct device *dev, uint8_t plane,
					       struct display_plane_capabilities *caps)
{
	const struct mcux_lcdifv2_config *config = dev->config;

	if (plane >= LCDIFV2_LAYER_COUNT) {
		return -EINVAL;
	}

	caps->supported_pixel_formats = mcux_lcdifv2_plane_formats(plane);
	caps->supported_blend_modes = (plane == 0U) ? 0U : LCDIFV2_BLEND_MODES;
	caps->max_width = config->display_config.panelWidth;
	caps->max_height = config->display_config.panelHeight;
	caps->width_align = mcux_lcdifv2_width_align(PIXEL_FORMAT_RGB_888);

	return 0;
}

static int mcux_lcdifv2_set_plane(const struct device *dev, uint8_t plane,
				  const struct display_plane_config *cfg)
{
	const struct mcux_lcdifv2_config *config = dev->config;
	struct mcux_lcdifv2_data *dev_data = dev->data;
	const struct display_buffer_descriptor *desc = &cfg->desc;
	lcdifv2_pixel_format_t hal_fmt;
	lcdifv2_blend_config_t blend;
	uint32_t stride;

	if (plane >= LCDIFV2_LAYER_COUNT) {
		return -EINVAL;
	}

	if (cfg->buf != NULL) {
		if ((cfg->pixel_format & mcux_lcdifv2_plane_formats(plane)) == 0U ||
		    mcux_lcdifv2_hal_format(cfg->pixel_format, &hal_fmt) != 0 ||
		    mcux_lcdifv2_blend_config(plane, cfg, &blend) != 0) {
			return -ENOTSUP;
		}

		stride = (uint32_t)desc->pitch * DISPLAY_BITS_PER_PIXEL(cfg->pixel_format) /
			 BITS_PER_BYTE;
		if (desc->width == 0U || desc->height == 0U || desc->pitch < desc->width ||
		    desc->width > LCDIFV2_MAX_DIM || desc->height > LCDIFV2_MAX_DIM ||
		    (desc->width % mcux_lcdifv2_width_align(cfg->pixel_format)) != 0U ||
		    cfg->x + desc->width > config->display_config.panelWidth ||
		    cfg->y + desc->height > config->display_config.panelHeight ||
		    stride > UINT16_MAX || desc->buf_size < stride * desc->height ||
		    ((uintptr_t)cfg->buf % 8U) != 0U) {
			return -EINVAL;
		}
	}

	k_mutex_lock(&dev_data->lock, K_FOREVER);
	dev_data->planes[plane].staged = *cfg;
	dev_data->planes[plane].dirty = true;
	k_mutex_unlock(&dev_data->lock);

	return 0;
}

static int mcux_lcdifv2_commit(const struct device *dev)
{
	struct mcux_lcdifv2_data *dev_data = dev->data;
	int ret;

	k_mutex_lock(&dev_data->lock, K_FOREVER);
	ret = mcux_lcdifv2_commit_locked(dev);
	k_mutex_unlock(&dev_data->lock);

	return ret;
}

static int mcux_lcdifv2_write(const struct device *dev, const uint16_t x, const uint16_t y,
			      const struct display_buffer_descriptor *desc, const void *buf)
{
	const struct mcux_lcdifv2_config *config = dev->config;
	struct mcux_lcdifv2_data *dev_data = dev->data;
	struct mcux_lcdifv2_plane *plane0 = &dev_data->planes[0];
	const uint8_t *src;
	uint8_t *dst;
	int ret;

	__ASSERT((dev_data->pixel_bytes * desc->pitch * desc->height) <= desc->buf_size,
		 "Input buffer too small");

	LOG_DBG("W=%d, H=%d, @%d,%d", desc->width, desc->height, x, y);

	k_mutex_lock(&dev_data->lock, K_FOREVER);

	plane0->staged = (struct display_plane_config){
		.desc = {.buf_size = dev_data->fb_bytes,
			 .width = config->display_config.panelWidth,
			 .height = config->display_config.panelHeight,
			 .pitch = config->display_config.panelWidth},
		.pixel_format = dev_data->pixel_format,
		.alpha = 255,
		.blend_mode = DISPLAY_BLEND_NONE,
	};

	if ((x == 0) && (y == 0) && (desc->width == config->display_config.panelWidth) &&
	    (desc->height == config->display_config.panelHeight) && (desc->pitch == desc->width)) {
		/* We can use the display buffer directly, no need to copy it */
		LOG_DBG("Setting FB from %p->%p", (void *)plane0->held, (void *)buf);
		plane0->staged.buf = buf;
	} else {
		/* We must use partial framebuffer copy */
		if (CONFIG_MCUX_LCDIFV2_FB_NUM == 0) {
			LOG_ERR("Partial display refresh requires driver framebuffers");
			k_mutex_unlock(&dev_data->lock);
			return -ENOTSUP;
		} else if (plane0->held != NULL &&
			   plane0->held != dev_data->fb[dev_data->next_idx]) {
			/*
			 * We must copy the entire current framebuffer to new
			 * buffer, since we will change the active buffer
			 * address
			 */
			src = plane0->held;
			dst = dev_data->fb[dev_data->next_idx];
			memcpy(dst, src, dev_data->fb_bytes);
		}
		/* Now, write the display update into active framebuffer */
		src = buf;
		dst = dev_data->fb[dev_data->next_idx];
		dst += dev_data->pixel_bytes * (y * config->display_config.panelWidth + x);

		for (int h_idx = 0; h_idx < desc->height; h_idx++) {
			memcpy(dst, src, dev_data->pixel_bytes * desc->width);
			src += dev_data->pixel_bytes * desc->pitch;
			dst += dev_data->pixel_bytes * config->display_config.panelWidth;
		}

		LOG_DBG("Setting FB from %p->%p", (void *)plane0->held,
			(void *)dev_data->fb[dev_data->next_idx]);
		plane0->staged.buf = dev_data->fb[dev_data->next_idx];
#if CONFIG_MCUX_LCDIFV2_FB_NUM != 0
		dev_data->next_idx = (dev_data->next_idx + 1) % CONFIG_MCUX_LCDIFV2_FB_NUM;
#endif
	}
	plane0->dirty = true;

	/* A write is what starts the display when nothing started it at init */
	if (!dev_data->display_on) {
		LCDIFV2_EnableDisplay(config->base, true);
		dev_data->display_on = true;
	}

	ret = mcux_lcdifv2_commit_locked(dev);
	k_mutex_unlock(&dev_data->lock);

	return ret;
}

static int mcux_lcdifv2_display_blanking_off(const struct device *dev)
{
	const struct mcux_lcdifv2_config *config = dev->config;
	struct mcux_lcdifv2_data *dev_data = dev->data;

	k_mutex_lock(&dev_data->lock, K_FOREVER);
	LCDIFV2_EnableDisplay(config->base, true);
	dev_data->display_on = true;
	k_mutex_unlock(&dev_data->lock);

	return 0;
}

static int mcux_lcdifv2_display_blanking_on(const struct device *dev)
{
	const struct mcux_lcdifv2_config *config = dev->config;
	struct mcux_lcdifv2_data *dev_data = dev->data;

	k_mutex_lock(&dev_data->lock, K_FOREVER);
	LCDIFV2_EnableDisplay(config->base, false);
	dev_data->display_on = false;
	k_mutex_unlock(&dev_data->lock);

	return 0;
}

static int mcux_lcdifv2_set_pixel_format(const struct device *dev,
					 const enum display_pixel_format pixel_format)
{
	struct mcux_lcdifv2_data *dev_data = dev->data;
	const struct mcux_lcdifv2_config *config = dev->config;
	lcdifv2_pixel_format_t hal_fmt;

	if ((pixel_format & mcux_lcdifv2_plane_formats(0)) == 0U ||
	    mcux_lcdifv2_hal_format(pixel_format, &hal_fmt) != 0) {
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
			k_heap_aligned_alloc(&display_heap, LCDIFV2_FB_ALIGN,
				dev_data->fb_bytes, K_FOREVER);
		if (dev_data->fb[i] == NULL) {
			LOG_ERR("Could not allocate memory for framebuffers");
			return -ENOMEM;
		}
		memset(dev_data->fb[i], 0, dev_data->fb_bytes);
	}

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

	capabilities->x_resolution = config->display_config.panelWidth;
	capabilities->y_resolution = config->display_config.panelHeight;
	capabilities->supported_pixel_formats = mcux_lcdifv2_plane_formats(0);
	capabilities->current_pixel_format = ((struct mcux_lcdifv2_data *)dev->data)->pixel_format;
	capabilities->current_orientation = DISPLAY_ORIENTATION_NORMAL;
	capabilities->num_planes = LCDIFV2_LAYER_COUNT;
}

static void mcux_lcdifv2_isr(const struct device *dev)
{
	const struct mcux_lcdifv2_config *config = dev->config;
	struct mcux_lcdifv2_data *dev_data = dev->data;
	uint32_t status;

	status = LCDIFV2_GetInterruptStatus(config->base, LCDIFV2_INT_DOMAIN);
	LCDIFV2_ClearInterruptStatus(config->base, LCDIFV2_INT_DOMAIN, status);

	/* The shadow registers loaded at this VSYNC: the commit is on screen */
	if (status & kLCDIFV2_VsyncEdgeInterrupt) {
		LCDIFV2_DisableInterrupts(config->base, LCDIFV2_INT_DOMAIN,
					  kLCDIFV2_VsyncEdgeInterrupt);
		k_sem_give(&dev_data->vsync_sem);
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

	k_mutex_init(&dev_data->lock);
	k_sem_init(&dev_data->vsync_sem, 0, 1);

	/* Initialize LCDIFV2 peripheral */
	LCDIFV2_Init(config->base);

	/* Initialize display configuration */
	LCDIFV2_SetDisplayConfig(config->base, &config->display_config);

	/* Set default pixel format obtained from device tree */
	err = mcux_lcdifv2_set_pixel_format(dev, dev_data->pixel_format);
	if (err != 0) {
		return err;
	}

	config->irq_config_func(dev);

#if CONFIG_MCUX_LCDIFV2_FB_NUM != 0
	/* Without a driver framebuffer the layer has no address yet: the first write enables it */
	dev_data->planes[0].staged = (struct display_plane_config){
		.buf = dev_data->fb[0],
		.desc = {.buf_size = dev_data->fb_bytes,
			 .width = config->display_config.panelWidth,
			 .height = config->display_config.panelHeight,
			 .pitch = config->display_config.panelWidth},
		.pixel_format = dev_data->pixel_format,
		.alpha = 255,
		.blend_mode = DISPLAY_BLEND_NONE,
	};
	mcux_lcdifv2_program_layer(dev, 0);
	dev_data->planes[0].held = dev_data->fb[0];
	dev_data->planes[0].enabled = true;
#endif

	/* With no layer enabled the display scans out its background color */
	if (IS_ENABLED(CONFIG_MCUX_LCDIFV2_START_ON_INIT)) {
		LCDIFV2_EnableDisplay(config->base, true);
		if (CONFIG_MCUX_LCDIFV2_FB_NUM != 0) {
			LCDIFV2_TriggerLayerShadowLoad(config->base, 0);
		}
		dev_data->display_on = true;
	}

	return 0;
}

static DEVICE_API(display, mcux_lcdifv2_api) = {
	.blanking_on = mcux_lcdifv2_display_blanking_on,
	.blanking_off = mcux_lcdifv2_display_blanking_off,
	.write = mcux_lcdifv2_write,
	.get_capabilities = mcux_lcdifv2_get_capabilities,
	.set_pixel_format = mcux_lcdifv2_set_pixel_format,
	.set_orientation = mcux_lcdifv2_set_orientation,
	.get_plane_capabilities = mcux_lcdifv2_get_plane_capabilities,
	.set_plane = mcux_lcdifv2_set_plane,
	.commit = mcux_lcdifv2_commit,
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
