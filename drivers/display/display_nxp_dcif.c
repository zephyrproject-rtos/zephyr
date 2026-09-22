/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_dcif

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/cache.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/linker/devicetree_regions.h>
#include <zephyr/sys/util.h>
#include <fsl_dcif.h>

LOG_MODULE_REGISTER(display_nxp_dcif, CONFIG_DISPLAY_LOG_LEVEL);

/* This driver drives DCIF layer 0 as a single, full-panel framebuffer. */
#define NXP_DCIF_LAYER 0U
/* Interrupts are routed to CPU domain 0 in a single-domain Zephyr system. */
#define NXP_DCIF_DOMAIN 0U

struct nxp_dcif_config {
	DCIF_Type *base;
	void (*irq_config_func)(const struct device *dev);
	const struct pinctrl_dev_config *pincfg;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	clock_control_subsys_t dcpixel_cfg;
	const struct gpio_dt_spec backlight_gpio;
	const struct gpio_dt_spec power_gpio;
	struct reset_dt_spec reset;
	dcif_dpi_config_t dpi_config;
	dcif_output_config_t output_config;
	/* Initial pixel format from devicetree */
	enum display_pixel_format init_pixel_format;
	/* Pointer to first framebuffer */
	uint8_t *fb_ptr;
};

struct nxp_dcif_data {
	/* Pointer to active framebuffer */
	const uint8_t *active_fb;
	uint8_t *fb[CONFIG_NXP_DCIF_FB_NUM];
	dcif_layer_format_t format;
	uint8_t pixel_bytes;
	uint16_t pitch_bytes;
	struct k_sem sem;
	uint32_t fb_bytes;
	/* Index of next active framebuffer */
	uint8_t next_idx;
};

static int nxp_dcif_write(const struct device *dev, const uint16_t x, const uint16_t y,
			   const struct display_buffer_descriptor *desc, const void *buf)
{
	const struct nxp_dcif_config *config = dev->config;
	struct nxp_dcif_data *data = dev->data;
	uint32_t h_idx;
	const uint8_t *src;
	uint8_t *dst;

	__ASSERT((data->pixel_bytes * desc->pitch * desc->height) <= desc->buf_size,
		 "Input buffer too small");

	LOG_DBG("W=%d, H=%d @%d,%d", desc->width, desc->height, x, y);

	if ((x == 0) && (y == 0) && (desc->width == config->output_config.width) &&
	    (desc->height == config->output_config.height) && (desc->pitch == desc->width)) {
		/* We can use the display buffer directly, without copying */
		LOG_DBG("Setting FB from %p->%p", (void *)data->active_fb, (void *)buf);
		data->active_fb = buf;
	} else {
		/* We must use partial framebuffer copy */
		if (CONFIG_NXP_DCIF_FB_NUM == 0) {
			LOG_ERR("Partial display refresh requires driver framebuffers");
			return -ENOTSUP;
		} else if (data->active_fb != data->fb[data->next_idx]) {
			/*
			 * Copy the entirety of the current framebuffer to new
			 * buffer, since we are changing the active buffer address
			 */
			src = data->active_fb;
			dst = data->fb[data->next_idx];
			memcpy(dst, src, data->fb_bytes);
		}
		/* Write the display update to the active framebuffer */
		src = buf;
		dst = data->fb[data->next_idx];
		dst += data->pixel_bytes * x + (y * data->pitch_bytes);

		for (h_idx = 0; h_idx < desc->height; h_idx++) {
			memcpy(dst, src, data->pixel_bytes * desc->width);
			src += data->pixel_bytes * desc->pitch;
			dst += data->pitch_bytes;
		}

		if (data->format == kDCIF_LayerPixelFormatNV21) {
			/*
			 * The UV plane is interleaved at the same byte-width per row as
			 * Y (width/2 U bytes + width/2 V bytes), but at half the row
			 * count (4:2:0 subsampling), so x/y must be even.
			 */
			__ASSERT(((x % 2) == 0) && ((y % 2) == 0), "NV12 requires even x/y");

			src = (const uint8_t *)buf + desc->pitch * desc->height;
			dst = data->fb[data->next_idx];
			dst += data->pitch_bytes * config->output_config.height;
			dst += data->pixel_bytes * x + ((y / 2) * data->pitch_bytes);

			for (h_idx = 0; h_idx < desc->height / 2; h_idx++) {
				memcpy(dst, src, data->pixel_bytes * desc->width);
				src += data->pixel_bytes * desc->pitch;
				dst += data->pitch_bytes;
			}
		}

		LOG_DBG("Setting FB from %p->%p", (void *)data->active_fb,
			(void *)data->fb[data->next_idx]);
		/* Set new active framebuffer */
		data->active_fb = data->fb[data->next_idx];
	}

	if (IS_ENABLED(CONFIG_NXP_DCIF_MAINTAIN_CACHE)) {
		sys_cache_data_flush_range((void *)data->active_fb, data->fb_bytes);
	}

	k_sem_reset(&data->sem);

	/* Set new framebuffer and trigger a shadow load, taking effect at next VSYNC. */
	DCIF_SetLayerStride(config->base, NXP_DCIF_LAYER, data->pitch_bytes);
	DCIF_SetLayerAddr(config->base, NXP_DCIF_LAYER, (uint32_t)data->active_fb);
	if (data->format == kDCIF_LayerPixelFormatNV21) {
		DCIF_SetLayerUVAddr(config->base, NXP_DCIF_LAYER,
				    (uint32_t)data->active_fb +
					    data->pitch_bytes * config->output_config.height);
	}
	DCIF_TriggerLayerShadowLoad(config->base, NXP_DCIF_LAYER);

	DCIF_EnableOutput(config->base, true);

#if CONFIG_NXP_DCIF_FB_NUM != 0
	/* Update index of active framebuffer */
	data->next_idx = (data->next_idx + 1) % CONFIG_NXP_DCIF_FB_NUM;
#endif
	/* Wait for frame to complete */
	k_sem_take(&data->sem, K_FOREVER);

	return 0;
}

static void nxp_dcif_get_capabilities(const struct device *dev,
				       struct display_capabilities *capabilities)
{
	const struct nxp_dcif_config *config = dev->config;
	struct nxp_dcif_data *data = dev->data;

	memset(capabilities, 0, sizeof(struct display_capabilities));

	capabilities->x_resolution = config->output_config.width;
	capabilities->y_resolution = config->output_config.height;
	capabilities->supported_pixel_formats =
		(PIXEL_FORMAT_RGB_565 | PIXEL_FORMAT_ARGB_8888 | PIXEL_FORMAT_RGB_888 |
		 PIXEL_FORMAT_BGR_888 | PIXEL_FORMAT_ABGR_8888 | PIXEL_FORMAT_NV12);
	capabilities->current_orientation = DISPLAY_ORIENTATION_NORMAL;

	switch (data->format) {
	case kDCIF_LayerPixelFormatRGB565:
		capabilities->current_pixel_format = PIXEL_FORMAT_RGB_565;
		break;
	case kDCIF_LayerPixelFormatRGB888:
		capabilities->current_pixel_format = PIXEL_FORMAT_RGB_888;
		break;
	case kDCIF_LayerPixelFormatBGR888:
		capabilities->current_pixel_format = PIXEL_FORMAT_BGR_888;
		break;
	case kDCIF_LayerPixelFormatARGB8888:
		capabilities->current_pixel_format = PIXEL_FORMAT_ARGB_8888;
		break;
	case kDCIF_LayerPixelFormatABGR8888:
		capabilities->current_pixel_format = PIXEL_FORMAT_ABGR_8888;
		break;
	case kDCIF_LayerPixelFormatNV21:
		capabilities->current_pixel_format = PIXEL_FORMAT_NV12;
		break;
	default:
		/* Other DCIF formats don't have a Zephyr enum yet */
		break;
	}
}

static void *nxp_dcif_get_framebuffer(const struct device *dev)
{
	struct nxp_dcif_data *data = dev->data;

	return (void *)data->active_fb;
}

static int nxp_dcif_display_blanking_off(const struct device *dev)
{
	const struct nxp_dcif_config *config = dev->config;

	DCIF_EnableOutput(config->base, true);

	return gpio_pin_set_dt(&config->backlight_gpio, 1);
}

static int nxp_dcif_display_blanking_on(const struct device *dev)
{
	const struct nxp_dcif_config *config = dev->config;

	DCIF_EnableOutput(config->base, false);

	return gpio_pin_set_dt(&config->backlight_gpio, 0);
}

static int nxp_dcif_set_pixel_format(const struct device *dev,
				      const enum display_pixel_format pixel_format)
{
	struct nxp_dcif_data *data = dev->data;
	const struct nxp_dcif_config *config = dev->config;

	switch (pixel_format) {
	case PIXEL_FORMAT_RGB_565:
		data->format = kDCIF_LayerPixelFormatRGB565;
		data->pixel_bytes = 2;
		break;
	case PIXEL_FORMAT_RGB_888:
		data->format = kDCIF_LayerPixelFormatRGB888;
		data->pixel_bytes = 3;
		break;
	case PIXEL_FORMAT_BGR_888:
		data->format = kDCIF_LayerPixelFormatBGR888;
		data->pixel_bytes = 3;
		break;
	case PIXEL_FORMAT_ARGB_8888:
		data->format = kDCIF_LayerPixelFormatARGB8888;
		data->pixel_bytes = 4;
		break;
	case PIXEL_FORMAT_ABGR_8888:
		data->format = kDCIF_LayerPixelFormatABGR8888;
		data->pixel_bytes = 4;
		break;
	case PIXEL_FORMAT_NV12:
		data->format = kDCIF_LayerPixelFormatNV21;
		data->pixel_bytes = 1;
		break;
	default:
		return -ENOTSUP;
	}

	DCIF_SetCscMode(config->base, NXP_DCIF_LAYER,
			pixel_format == PIXEL_FORMAT_NV12 ? kDCIF_CscYCbCr2RGB : kDCIF_CscDisable);

	/*
	 * Update the pitch bytes and framebuffer size based on new pixel format,
	 * they will be used in pixel write. DCIF requires the layer stride to be
	 * 16-byte aligned (DCIF_FB_ALIGN).
	 */
	data->pitch_bytes = ROUND_UP((config->output_config.width * data->pixel_bytes),
				     DCIF_FB_ALIGN);
	/*
	 * NV12 is 2-plane: a full Y plane followed by a half-height, interleaved
	 * UV plane.
	 */
	data->fb_bytes = data->pitch_bytes * config->output_config.height *
			 (pixel_format == PIXEL_FORMAT_NV12 ? 2U : 1U);

	/* Update frame buffer pointer. */
	for (int i = 0; i < CONFIG_NXP_DCIF_FB_NUM; i++) {
		data->fb[i] = config->fb_ptr + (data->fb_bytes * i);
	}
	data->active_fb = config->fb_ptr;

	/* Clear the framebuffer since the frame size may be larger. */
	memset(config->fb_ptr, 0, data->fb_bytes * CONFIG_NXP_DCIF_FB_NUM);

	dcif_layer_config_t layer_config = {0};

	layer_config.enable = true;
	layer_config.format = data->format;
	layer_config.width = config->output_config.width;
	layer_config.height = config->output_config.height;
	layer_config.globalAlpha = 0xFFU;
	layer_config.alphaBlendMode = kDCIF_AlphaBlendOverride;

	DCIF_SetLayerConfig(config->base, NXP_DCIF_LAYER, &layer_config);

	return 0;
}

static void nxp_dcif_isr(const struct device *dev)
{
	const struct nxp_dcif_config *config = dev->config;
	struct nxp_dcif_data *data = dev->data;
	uint32_t status;

	status = DCIF_GetInterruptStatus(config->base, NXP_DCIF_DOMAIN);
	DCIF_ClearInterruptStatus(config->base, NXP_DCIF_DOMAIN, status);

	if (0 != (status & kDCIF_InterruptVsync)) {
		k_sem_give(&data->sem);
	}
}

static int nxp_dcif_init(const struct device *dev)
{
	const struct nxp_dcif_config *config = dev->config;
	struct nxp_dcif_data *data = dev->data;
	dcif_layer_config_t layer_config = {0};
	int ret;

	if (config->pincfg != NULL) {
		ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
		if (ret != 0) {
			return ret;
		}
	}

	if (config->clock_dev != NULL) {
		if (!device_is_ready(config->clock_dev)) {
			return -ENODEV;
		}

		ret = clock_control_on(config->clock_dev, config->clock_subsys);
		if (ret != 0) {
			return ret;
		}

		ret = clock_control_configure(config->clock_dev, config->dcpixel_cfg, NULL);
		if (ret != 0) {
			return ret;
		}
	}

	if (config->power_gpio.port != NULL) {
		ret = gpio_pin_configure_dt(&config->power_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret) {
			return ret;
		}
	}

	ret = gpio_pin_configure_dt(&config->backlight_gpio, GPIO_OUTPUT_ACTIVE);
	if (ret) {
		return ret;
	}

	if (config->reset.dev != NULL) {
		if (!device_is_ready(config->reset.dev)) {
			LOG_ERR("reset controller not ready");
			return -ENODEV;
		}

		ret = reset_line_deassert_dt(&config->reset);
		if (ret != 0) {
			LOG_ERR("Failed to deassert reset line (%d)", ret);
			return ret;
		}
	}

	ret = nxp_dcif_set_pixel_format(dev, config->init_pixel_format);
	if (ret) {
		return ret;
	}

	DCIF_Init(config->base);

	DCIF_SetOutputConfig(config->base, &config->output_config);
	DCIF_DpiSetConfig(config->base, &config->dpi_config);

	/* Configure layer 0 as the full-panel framebuffer source. */
	layer_config.enable = true;
	layer_config.format = data->format;
	layer_config.topLeftX = 0;
	layer_config.topLeftY = 0;
	layer_config.width = config->output_config.width;
	layer_config.height = config->output_config.height;
	layer_config.globalAlpha = 0xFFU;
	layer_config.alphaBlendMode = kDCIF_AlphaBlendOverride;

	ret = DCIF_SetLayerConfig(config->base, NXP_DCIF_LAYER, &layer_config);
	if (ret != kStatus_Success) {
		LOG_ERR("Failed to configure DCIF layer (%d)", ret);
		return -EINVAL;
	}

	DCIF_EnableLayer(config->base, NXP_DCIF_LAYER, true);

	DCIF_EnableInterrupts(config->base, NXP_DCIF_DOMAIN, kDCIF_InterruptVsync);
	config->irq_config_func(dev);

	for (int i = 0; i < CONFIG_NXP_DCIF_FB_NUM; i++) {
		/* Record pointers to each driver framebuffer */
		data->fb[i] = config->fb_ptr + (data->fb_bytes * i);
	}
	data->active_fb = config->fb_ptr;

	k_sem_init(&data->sem, 1, 1);

	/* Clear external memory, as it is uninitialized */
	memset(config->fb_ptr, 0, data->fb_bytes * CONFIG_NXP_DCIF_FB_NUM);

	DCIF_SetLayerStride(config->base, NXP_DCIF_LAYER, data->pitch_bytes);
	DCIF_SetLayerAddr(config->base, NXP_DCIF_LAYER, (uint32_t)data->active_fb);
	if (data->format == kDCIF_LayerPixelFormatNV21) {
		DCIF_SetLayerUVAddr(config->base, NXP_DCIF_LAYER,
				    (uint32_t)data->active_fb +
					    data->pitch_bytes * config->output_config.height);
	}
	DCIF_TriggerLayerShadowLoad(config->base, NXP_DCIF_LAYER);

	return 0;
}

static DEVICE_API(display, nxp_dcif_api) = {
	.blanking_on = nxp_dcif_display_blanking_on,
	.blanking_off = nxp_dcif_display_blanking_off,
	.set_pixel_format = nxp_dcif_set_pixel_format,
	.write = nxp_dcif_write,
	.get_capabilities = nxp_dcif_get_capabilities,
	.get_framebuffer = nxp_dcif_get_framebuffer,
};

/* Place the frame buffer in secondary RAM if specified, otherwise use default RAM */
#define NXP_DCIF_FB_PLACEMENT(n)							\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, ext_ram),					\
	(Z_GENERIC_SECTION(LINKER_DT_NODE_REGION_NAME(DT_INST_PHANDLE(n, ext_ram)))),	\
	())

/* Use 4 Bpp to calculate the largest possible framebuffer size. */
#define NXP_DCIF_FRAMEBUFFER_DECL(n)								\
	NXP_DCIF_FB_PLACEMENT(n) static uint8_t __aligned(DCIF_FB_ALIGN)			\
		nxp_dcif_frame_buffer_##n[CONFIG_NXP_DCIF_FB_NUM * DT_INST_PROP(n, height) *	\
					ROUND_UP((DT_INST_PROP(n, width) * 4U), DCIF_FB_ALIGN)]

#define NXP_DCIF_FRAMEBUFFER(n) nxp_dcif_frame_buffer_##n

#define NXP_DCIF_DEVICE_INIT(n)									\
	static void nxp_dcif_config_func_##n(const struct device *dev)				\
	{											\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), nxp_dcif_isr,		\
			    DEVICE_DT_INST_GET(n), 0);						\
		irq_enable(DT_INST_IRQN(n));							\
	}											\
	NXP_DCIF_FRAMEBUFFER_DECL(n);								\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, pinctrl_0), (PINCTRL_DT_INST_DEFINE(n)), ());	\
	struct nxp_dcif_data nxp_dcif_data_##n = {						\
		.next_idx = 0,									\
	};											\
	struct nxp_dcif_config nxp_dcif_config_##n = {						\
		.base = (DCIF_Type *)DT_INST_REG_ADDR(n),					\
		.irq_config_func = nxp_dcif_config_func_##n,					\
		.pincfg = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, pinctrl_0),			\
			(PINCTRL_DT_INST_DEV_CONFIG_GET(n)), (NULL)),				\
		.init_pixel_format = DT_INST_PROP(n, pixel_format),				\
		.clock_dev = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, clocks),			\
					 (DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n))), (NULL)),	\
		.clock_subsys = COND_CODE_1(							\
			DT_INST_NODE_HAS_PROP(n, clocks),					\
			((clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name)),			\
			((clock_control_subsys_t)0U)),						\
		.dcpixel_cfg = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, clocks),			\
			((clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_NAME(n, source, name)),	\
			((clock_control_subsys_t)0U)),						\
		.backlight_gpio = GPIO_DT_SPEC_INST_GET(n, backlight_gpios),			\
		.power_gpio = GPIO_DT_SPEC_INST_GET_OR(n, power_gpios, {0}),			\
		.reset = RESET_DT_SPEC_INST_GET_OR(n, {0}),					\
		.output_config = {								\
			.interface = kDCIF_OutputDpi,						\
			.width = DT_INST_PROP(n, width),					\
			.height = DT_INST_PROP(n, height),					\
		},										\
		.dpi_config = {									\
			.hsw = DT_PROP(DT_INST_CHILD(n, display_timings), hsync_len),		\
			.hfp = DT_PROP(DT_INST_CHILD(n, display_timings), hfront_porch),	\
			.hbp = DT_PROP(DT_INST_CHILD(n, display_timings), hback_porch),		\
			.vsw = DT_PROP(DT_INST_CHILD(n, display_timings), vsync_len),		\
			.vfp = DT_PROP(DT_INST_CHILD(n, display_timings), vfront_porch),	\
			.vbp = DT_PROP(DT_INST_CHILD(n, display_timings), vback_porch),		\
			.polarityFlags =							\
				(DT_PROP(DT_INST_CHILD(n, display_timings), de_active)		\
					 ? kDCIF_DpiDataEnableActiveHigh			\
					 : kDCIF_DpiDataEnableActiveLow) |			\
				(DT_PROP(DT_INST_CHILD(n, display_timings), pixelclk_active)	\
					 ? kDCIF_DpiDriveDataOnRisingClkEdge			\
					 : kDCIF_DpiDriveDataOnFallingClkEdge) |		\
				(DT_PROP(DT_INST_CHILD(n, display_timings), hsync_active)	\
					 ? kDCIF_DpiHsyncActiveHigh				\
					 : kDCIF_DpiHsyncActiveLow) |				\
				(DT_PROP(DT_INST_CHILD(n, display_timings), vsync_active)	\
					 ? kDCIF_DpiVsyncActiveHigh				\
					 : kDCIF_DpiVsyncActiveLow),				\
			.format = DT_INST_ENUM_IDX(n, data_bus_width),				\
			.displayMode = kDCIF_DpiNormal,						\
			.frameFetch = kDCIF_DpiVfpBegin,					\
			.txFifoFill = kDCIF_DpiVfpBegin,					\
			.enableBackground = true,						\
		},										\
		.fb_ptr = NXP_DCIF_FRAMEBUFFER(n),						\
	};											\
	DEVICE_DT_INST_DEFINE(n, &nxp_dcif_init, NULL, &nxp_dcif_data_##n,			\
			      &nxp_dcif_config_##n, POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY,	\
			      &nxp_dcif_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_DCIF_DEVICE_INIT)
