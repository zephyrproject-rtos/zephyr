/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT socionext_dpu

#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>

#include <fsl_dpu.h>

#define APP_DPU_DC ((DISPLAY_SEERIS_Type *)0x4B780000U)

LOG_MODULE_REGISTER(display_socionext_dpu, CONFIG_DISPLAY_LOG_LEVEL);

struct dpu_config {
	DISPLAY_SEERIS_Type *base;
	void (*irq_config_func)(const struct device *dev);
	dpu_const_frame_config_t cfConfig;
	dpu_layer_blend_config_t lbConfig;
	dpu_fetch_unit_config_t fetchConfig;
	dpu_src_buffer_config_t sbConfig;
	dpu_display_config_t displayConfig;
	dpu_display_timing_config_t display_timing;
	/* Pointer to start of first framebuffer */
	uint8_t *fb_ptr;
	/* Number of bytes used for each framebuffer */
	uint32_t fb_bytes;
};

struct dpu_data {
	/* Pointer to active framebuffer */
	const uint8_t *active_fb;
	uint8_t *fb[CONFIG_DPU_FB_NUM];
	enum display_pixel_format pixel_format;
	dpu_pixel_format_t rgb_format;
	uint8_t pixel_bytes;
	struct k_sem sem;
	/* Tracks index of next active driver framebuffer */
	uint8_t next_idx;
};

static void dpu_trigger_content_stream_shadowload(const struct device *dev)
{
	const struct dpu_config *config = dev->config;

	DPU_TriggerPipelineShadowLoad(config->base, kDPU_PipelineExtDst0);
}

static int dpu_write(const struct device *dev, const uint16_t x, const uint16_t y,
		     const struct display_buffer_descriptor *desc, const void *buf)
{
	const struct dpu_config *config = dev->config;
	struct dpu_data *data = dev->data;
	uint32_t h_idx;
	const uint8_t *src;
	uint8_t *dst;
	bool buf_aligned = ((uintptr_t)buf % 32) == 0;

	__ASSERT((data->pixel_bytes * desc->pitch * desc->height) <= desc->buf_size,
		 "Input buffer too small");

	LOG_DBG("W=%d, H=%d @%d,%d", desc->width, desc->height, x, y);

	if ((x == 0) && (y == 0) && (desc->width == config->display_timing.width) &&
	    (desc->height == config->display_timing.height) && (desc->pitch == desc->width) &&
	    buf_aligned) {
		/* We can use the display buffer directly, without copying */
		LOG_DBG("Setting FB from %p->%p", (void *)data->active_fb, (void *)buf);
		data->active_fb = buf;
	} else {
		/* We must use partial framebuffer copy */
		if (CONFIG_DPU_FB_NUM == 0) {
			LOG_ERR("Display refresh requires driver framebuffers");
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
		dst += data->pixel_bytes * (y * config->display_timing.width + x);

		for (h_idx = 0; h_idx < desc->height; h_idx++) {
			memcpy(dst, src, data->pixel_bytes * desc->width);
			src += data->pixel_bytes * desc->pitch;
			dst += data->pixel_bytes * config->display_timing.width;
		}
		LOG_DBG("Setting FB from %p->%p", (void *)data->active_fb,
			(void *)data->fb[data->next_idx]);
		/* Set new active framebuffer */
		data->active_fb = data->fb[data->next_idx];
	}

	/* Set new framebuffer */
	DPU_SetFetchUnitSrcBufferAddr(config->base, kDPU_FetchYuv0, 0,
				      (uint32_t)(uintptr_t)data->active_fb);
	dpu_trigger_content_stream_shadowload(dev);

#if CONFIG_DPU_FB_NUM != 0
	/* Update index of active framebuffer */
	data->next_idx = (data->next_idx + 1) % CONFIG_DPU_FB_NUM;
#endif
	/* Wait for frame send to complete */
	k_sem_take(&data->sem, K_FOREVER);

	return 0;
}

static void dpu_get_capabilities(const struct device *dev,
				 struct display_capabilities *capabilities)
{
	const struct dpu_config *config = dev->config;
	struct dpu_data *data = dev->data;

	memset(capabilities, 0, sizeof(struct display_capabilities));
	capabilities->x_resolution = config->display_timing.width;
	capabilities->y_resolution = config->display_timing.height;
	capabilities->supported_pixel_formats = data->pixel_format;
	capabilities->current_pixel_format = data->pixel_format;
	capabilities->current_orientation = DISPLAY_ORIENTATION_NORMAL;
}

static void *dpu_get_framebuffer(const struct device *dev)
{
	struct dpu_data *data = dev->data;

	return (void *)data->active_fb;
}

static int dpu_set_pixel_format(const struct device *dev,
				const enum display_pixel_format pixel_format)
{
	struct dpu_data *data = dev->data;

	switch (pixel_format) {
	case PIXEL_FORMAT_BGR_565:
		data->rgb_format = kDPU_PixelFormatRGB565;
		data->pixel_bytes = 2;
		break;
	case PIXEL_FORMAT_RGB_888:
		data->rgb_format = kDPU_PixelFormatRGB888;
		data->pixel_bytes = 3;
		break;
	case PIXEL_FORMAT_ARGB_8888:
		data->rgb_format = kDPU_PixelFormatARGB8888;
		data->pixel_bytes = 4;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static void dpu_isr(const struct device *dev)
{
	const struct dpu_config *config = dev->config;
	struct dpu_data *data = dev->data;
	uint32_t status;

	status = DPU_GetInterruptsPendingFlags(APP_DPU_DC, 0);

	if (0 != (status & kDPU_Group0ExtDst0ShadowLoadInterrupt)) {
		DPU_ClearInterruptsPendingFlags(APP_DPU_DC, 0, status);
		k_sem_give(&data->sem);
	}
}

static int dpu_init(const struct device *dev)
{
	const struct dpu_config *config = dev->config;
	struct dpu_data *data = dev->data;
	int ret = 0;

	config->irq_config_func(dev);

	for (int i = 0; i < CONFIG_DPU_FB_NUM; i++) {
		/* Record pointers to each driver framebuffer */
		data->fb[i] = config->fb_ptr + (config->fb_bytes * i);
	}
	data->active_fb = config->fb_ptr;

	k_sem_init(&data->sem, 1, 1);

	/* Clear external memory, as it is uninitialized */
	memset(config->fb_ptr, 0, config->fb_bytes * CONFIG_DPU_FB_NUM);

	dpu_const_frame_config_t cfConfig;
	dpu_layer_blend_config_t lbConfig;
	dpu_fetch_unit_config_t fetchConfig;
	dpu_src_buffer_config_t sbConfig;
	dpu_display_timing_config_t displayTimingConfig;
	dpu_display_config_t displayConfig;

	DPU_Init(config->base);
	DPU_PreparePathConfig(config->base);
	/* Step 1: Configure the const stream */

	/* Pipeline Configuration */
	DPU_InitPipeline(config->base, kDPU_PipelineExtDst0);
	/* ExtDst: set the dynamic source */
	DPU_InitExtDst(config->base, kDPU_ExtDst0, DPU_MAKE_SRC_REG1(kDPU_UnitSrcLayerBlend1));
	/* Layer blend: set the primary and seconday sources */
	DPU_InitLayerBlend(config->base, kDPU_LayerBlend1,
			   DPU_MAKE_SRC_REG2(kDPU_UnitSrcConstFrame0, kDPU_UnitSrcFetchYUV0));
	DPU_LayerBlendGetDefaultConfig(&lbConfig);
	DPU_SetLayerBlendConfig(config->base, kDPU_LayerBlend1, &lbConfig);
	DPU_EnableLayerBlend(config->base, kDPU_LayerBlend1, true);
	/* Init domain blend */
	DPU_InitDomainBlend(config->base, kDPU_DomainBlend0);
	cfConfig.frameWidth = config->display_timing.width;
	cfConfig.frameHeight = config->display_timing.height;
	cfConfig.constColor = DPU_MAKE_CONST_COLOR(0, 0, 0, 0);
	DPU_InitConstFrame(config->base, kDPU_ConstFrame0);
	DPU_SetConstFrameConfig(config->base, kDPU_ConstFrame0, &cfConfig);
	DPU_FetchUnitGetDefaultConfig(&fetchConfig);
	fetchConfig.frameHeight = config->display_timing.height;
	fetchConfig.frameWidth = config->display_timing.width;
	DPU_InitFetchUnit(config->base, kDPU_FetchYuv0, &fetchConfig);
	DPU_SrcBufferGetDefaultConfig(&sbConfig);

	/* Convert pixel format from devicetree to the format used by HAL */
	ret = dpu_set_pixel_format(dev, data->pixel_format);
	if (ret) {
		return ret;
	}

	sbConfig.bitsPerPixel = data->pixel_bytes * 8;
	sbConfig.pixelFormat = data->rgb_format;
	sbConfig.constColor = DPU_MAKE_CONST_COLOR(0, 0, 0, 0);
	sbConfig.strideBytes = config->display_timing.width * data->pixel_bytes;
	sbConfig.bufferHeight = config->display_timing.height;
	sbConfig.bufferWidth = config->display_timing.width;
	sbConfig.baseAddr = data->fb[0];
	DPU_SetFetchUnitSrcBufferConfig(config->base, kDPU_FetchYuv0, 0, &sbConfig);
	DPU_SetFetchUnitOffset(config->base, kDPU_FetchYuv0, 0, 0, 0);
	DPU_EnableFetchUnitSrcBuffer(config->base, kDPU_FetchYuv0, 0, true);
	dpu_trigger_content_stream_shadowload(dev);
	DPU_EnableInterrupts(APP_DPU_DC, 0, kDPU_Group0ExtDst0ShadowLoadInterrupt);
	DPU_TriggerDisplayDbShadowLoad(config->base, kDPU_DomainBlend0);

	/* Step 2: Configure the display stream. */
	DPU_DisplayTimingGetDefaultConfig(&displayTimingConfig);
	displayTimingConfig.flags = config->display_timing.flags;
	displayTimingConfig.width = config->display_timing.width;
	displayTimingConfig.hsw = config->display_timing.hsw;
	displayTimingConfig.hfp = config->display_timing.hfp;
	displayTimingConfig.hbp = config->display_timing.hbp;
	displayTimingConfig.height = config->display_timing.height;
	displayTimingConfig.vsw = config->display_timing.vsw;
	displayTimingConfig.vfp = config->display_timing.vfp - 1U;
	displayTimingConfig.vbp = config->display_timing.vbp + 1U;
	DPU_InitDisplayTiming(config->base, 0, &displayTimingConfig);
	DPU_DisplayGetDefaultConfig(&displayConfig);

	/* Only show the content stream in normal mode. */
	displayConfig.displayMode = kDPU_DisplayOnlyPrim;
	displayConfig.primAreaStartX = 1;
	displayConfig.primAreaStartY = 1;
	DPU_SetDisplayConfig(config->base, 0, &displayConfig);
	DPU_TriggerDisplayShadowLoad(config->base, 0);
	DPU_StartDisplay(config->base, 0);

	return 0;
}

static DEVICE_API(display, dpu_api) = {
	.set_pixel_format = dpu_set_pixel_format,
	.write = dpu_write,
	.get_capabilities = dpu_get_capabilities,
	.get_framebuffer = dpu_get_framebuffer,
};

#define GET_PIXEL_FORMAT(id)                                                                       \
	((DT_INST_ENUM_IDX(id, pixel_format) == 0)                                                 \
		 ? PIXEL_FORMAT_BGR_565                                                            \
		 : ((DT_INST_ENUM_IDX(id, pixel_format) == 1) ? PIXEL_FORMAT_RGB_888               \
							      : PIXEL_FORMAT_ARGB_8888))
#define DPU_PIXEL_BYTES(id)                                                                        \
	((DT_INST_ENUM_IDX(id, pixel_format) == 0)                                                 \
		 ? 2                                                                               \
		 : ((DT_INST_ENUM_IDX(id, pixel_format) == 1) ? 3 : 4))

#define DPU_FB_SIZE(id) DT_INST_PROP(id, width) * DT_INST_PROP(id, height) * DPU_PIXEL_BYTES(id)

/* When using external framebuffer mem, we should not allocate framebuffers
 * in SRAM. Instead, we use external framebuffer address and size
 * from devicetree.
 */
#ifdef CONFIG_DPU_EXTERNAL_FB_MEM
#define DPU_FRAMEBUFFER_DECL(id)
#define DPU_FRAMEBUFFER(id) (uint8_t *)CONFIG_DPU_EXTERNAL_FB_ADDR
#else
#define DPU_FRAMEBUFFER_DECL(id)                                                                   \
	uint8_t __aligned(32) dpu_frame_buffer_##id[DT_INST_PROP(id, width) *                      \
						    DT_INST_PROP(id, height) *                     \
						    DPU_PIXEL_BYTES(id) * CONFIG_DPU_FB_NUM]
#define DPU_FRAMEBUFFER(id) dpu_frame_buffer_##id
#endif

#define DPU_DEVICE_INIT(id)                                                                        \
	static void dpu_config_func_##id(const struct device *dev)                                 \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(id), DT_INST_IRQ(id, priority), dpu_isr,                  \
			    DEVICE_DT_INST_GET(id), 0);                                            \
		irq_enable(DT_INST_IRQN(id));                                                      \
	}                                                                                          \
	DPU_FRAMEBUFFER_DECL(id);                                                                  \
	struct dpu_data dpu_data_##id = {                                                          \
		.next_idx = 0,                                                                     \
		.pixel_bytes = DPU_PIXEL_BYTES(id),                                                \
		.pixel_format = GET_PIXEL_FORMAT(id),                                              \
	};                                                                                         \
	struct dpu_config dpu_config_##id = {                                                      \
		.base = (DISPLAY_SEERIS_Type *)DT_INST_REG_ADDR(id),                               \
		.display_timing =                                                                  \
			{                                                                          \
				.width = DT_INST_PROP(id, width),                                  \
				.height = DT_INST_PROP(id, height),                                \
				.hsw = DT_PROP(DT_INST_CHILD(id, display_timings), hsync_len),     \
				.hfp = DT_PROP(DT_INST_CHILD(id, display_timings), hfront_porch),  \
				.hbp = DT_PROP(DT_INST_CHILD(id, display_timings), hback_porch),   \
				.vsw = DT_PROP(DT_INST_CHILD(id, display_timings), vsync_len),     \
				.vfp = DT_PROP(DT_INST_CHILD(id, display_timings), vfront_porch),  \
				.vbp = DT_PROP(DT_INST_CHILD(id, display_timings), vback_porch),   \
				.flags =                                                           \
					(DT_PROP(DT_INST_CHILD(id, display_timings), hsync_active) \
						 ? kDPU_DisplayHsyncActiveLow                      \
						 : kDPU_DisplayHsyncActiveHigh) |                  \
					(DT_PROP(DT_INST_CHILD(id, display_timings), vsync_active) \
						 ? kDPU_DisplayVsyncActiveLow                      \
						 : kDPU_DisplayVsyncActiveHigh) |                  \
					(DT_PROP(DT_INST_CHILD(id, display_timings), de_active)    \
						 ? kDPU_DisplayDataEnableActiveLow                 \
						 : kDPU_DisplayDataEnableActiveHigh),              \
			},                                                                         \
		.irq_config_func = dpu_config_func_##id,                                           \
		.fb_ptr = DPU_FRAMEBUFFER(id),                                                     \
		.fb_bytes = DPU_FB_SIZE(id)};                                                      \
	DEVICE_DT_INST_DEFINE(id, &dpu_init, NULL, &dpu_data_##id, &dpu_config_##id, POST_KERNEL,  \
			      CONFIG_DISPLAY_INIT_PRIORITY, &dpu_api);

DT_INST_FOREACH_STATUS_OKAY(DPU_DEVICE_INIT)
