/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_lcdifv3

#include <zephyr/drivers/display.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/kernel.h>
#include <fsl_lcdifv3.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/cache.h>

LOG_MODULE_REGISTER(display_mcux_lcdifv3, CONFIG_DISPLAY_LOG_LEVEL);

/* Required by DEVICE_MMIO_NAMED_* macros */
#define DEV_CFG(_dev)  ((const struct mcux_lcdifv3_config *)(_dev)->config)
#define DEV_DATA(_dev) ((struct mcux_lcdifv3_data *)(_dev)->data)

struct mcux_lcdifv3_config {
	DEVICE_MMIO_NAMED_ROM(reg_base);

	const struct device *disp_pix_clk_dev;
	clock_control_subsys_t disp_pix_clk_subsys;
	uint32_t disp_pix_clk_rate;
	const struct device *media_axi_clk_dev;
	clock_control_subsys_t media_axi_clk_subsys;
	uint32_t media_axi_clk_rate;
	const struct device *media_apb_clk_dev;
	clock_control_subsys_t media_apb_clk_subsys;
	uint32_t media_apb_clk_rate;

	void (*irq_config_func)(const struct device *dev);
	lcdifv3_buffer_config_t buffer_config;
	lcdifv3_display_config_t display_config;
	enum display_pixel_format pixel_format;
	uint8_t *fb_ptr;
	size_t fb_bytes;
};

struct mcux_lcdifv3_data {
	DEVICE_MMIO_NAMED_RAM(reg_base);
	/* Pointer to active framebuffer */
	const uint8_t *active_fb;
	uint8_t *fb[CONFIG_MCUX_LCDIFV3_FB_NUM];
	uint8_t pixel_bytes;
	struct k_sem sem;
	/* Tracks index of next active driver framebuffer */
	uint8_t next_idx;
};

static void dump_reg(LCDIF_Type *base)
{
	/* Dump LCDIF Registers */
	LOG_DBG("CTRL: 0x%x", base->CTRL.RW);
	LOG_DBG("DISP_PARA: 0x%x", base->DISP_PARA);
	LOG_DBG("DISP_SIZE: 0x%x", base->DISP_SIZE);
	LOG_DBG("HSYN_PARA: 0x%x", base->HSYN_PARA);
	LOG_DBG("VSYN_PARA: 0x%x", base->VSYN_PARA);
	LOG_DBG("VSYN_HSYN_WIDTH: 0x%x", base->VSYN_HSYN_WIDTH);
	LOG_DBG("INT_STATUS_D0: 0x%x", base->INT_STATUS_D0);
	LOG_DBG("INT_STATUS_D1: 0x%x", base->INT_STATUS_D1);
	LOG_DBG("CTRLDESCL_1: 0x%x", base->CTRLDESCL_1[0]);
	LOG_DBG("CTRLDESCL_3: 0x%x", base->CTRLDESCL_3[0]);
	LOG_DBG("CTRLDESCL_LOW_4: 0x%x", base->CTRLDESCL_LOW_4[0]);
	LOG_DBG("CTRLDESCL_HIGH_4: 0x%x", base->CTRLDESCL_HIGH_4[0]);
	LOG_DBG("CTRLDESCL_5: 0x%x", base->CTRLDESCL_5[0]);
}

static int mcux_lcdifv3_write(const struct device *dev, const uint16_t x, const uint16_t y,
			      const struct display_buffer_descriptor *desc, const void *buf)
{
	const struct mcux_lcdifv3_config *config = dev->config;
	struct mcux_lcdifv3_data *data = dev->data;
	LCDIF_Type *base = (LCDIF_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	uint32_t h_idx;
	const uint8_t *src;
	uint8_t *dst;

	__ASSERT((data->pixel_bytes * desc->pitch * desc->height) <= desc->buf_size,
		 "Input buffer too small");

	LOG_DBG("W=%d, H=%d @%d,%d", desc->width, desc->height, x, y);

	if ((x == 0) && (y == 0) && (desc->width == config->display_config.panelWidth) &&
	    (desc->height == config->display_config.panelHeight) && (desc->pitch == desc->width)) {
		/* We can use the display buffer directly, without copying */
		LOG_DBG("Setting FB from %p->%p", (void *)data->active_fb, (void *)buf);
		data->active_fb = buf;
	} else {
		/* We must use partial framebuffer copy */
		if (CONFIG_MCUX_LCDIFV3_FB_NUM == 0) {
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
		dst += data->pixel_bytes * (y * config->display_config.panelWidth + x);

		for (h_idx = 0; h_idx < desc->height; h_idx++) {
			memcpy(dst, src, data->pixel_bytes * desc->width);
			src += data->pixel_bytes * desc->pitch;
			dst += data->pixel_bytes * config->display_config.panelWidth;
		}
		LOG_DBG("Setting FB from %p->%p", (void *)data->active_fb,
			(void *)data->fb[data->next_idx]);
		/* Set new active framebuffer */
		data->active_fb = data->fb[data->next_idx];
	}

	sys_cache_data_flush_and_invd_range((void *)data->active_fb, config->fb_bytes);

	k_sem_reset(&data->sem);

	/* Set new framebuffer */
	LCDIFV3_SetLayerBufferAddr(base, 0, (uint32_t)(uintptr_t)data->active_fb);
	LCDIFV3_TriggerLayerShadowLoad(base, 0);

#if CONFIG_MCUX_LCDIFV3_FB_NUM != 0
	/* Update index of active framebuffer */
	data->next_idx = (data->next_idx + 1) % CONFIG_MCUX_LCDIFV3_FB_NUM;
#endif
	/* Wait for frame to complete */
	k_sem_take(&data->sem, K_FOREVER);

	return 0;
}

static void *mcux_lcdifv3_get_framebuffer(const struct device *dev)
{
	struct mcux_lcdifv3_data *dev_data = dev->data;

	return (void *)dev_data->active_fb;
}

static void mcux_lcdifv3_get_capabilities(const struct device *dev,
					  struct display_capabilities *capabilities)
{
	const struct mcux_lcdifv3_config *config = dev->config;

	memset(capabilities, 0, sizeof(struct display_capabilities));
	capabilities->x_resolution = config->display_config.panelWidth;
	capabilities->y_resolution = config->display_config.panelHeight;
	capabilities->supported_pixel_formats = config->pixel_format;
	capabilities->current_pixel_format = config->pixel_format;
	capabilities->current_orientation = DISPLAY_ORIENTATION_NORMAL;
}

static void mcux_lcdifv3_isr(const struct device *dev)
{
	struct mcux_lcdifv3_data *dev_data = dev->data;
	uint32_t status;
	LCDIF_Type *base = (LCDIF_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);

	status = LCDIFV3_GetInterruptStatus(base);
	LCDIFV3_ClearInterruptStatus(base, status);

	k_sem_give(&dev_data->sem);
}

static int mcux_lcdifv3_configure_clock(const struct device *dev)
{
	const struct mcux_lcdifv3_config *config = dev->config;

	clock_control_set_rate(config->disp_pix_clk_dev, config->disp_pix_clk_subsys,
			       (clock_control_subsys_rate_t)(uintptr_t)config->disp_pix_clk_rate);
	return 0;
}

static int mcux_axi_apb_configure_clock(const struct device *dev)
{
	const struct mcux_lcdifv3_config *config = dev->config;
	uint32_t clk_freq;

	/* configure media_axi_clk */
	if (!device_is_ready(config->media_axi_clk_dev)) {
		LOG_ERR("media_axi clock control device not ready");
		return -ENODEV;
	}
	clock_control_set_rate(config->media_axi_clk_dev, config->media_axi_clk_subsys,
			       (clock_control_subsys_rate_t)(uintptr_t)config->media_axi_clk_rate);
	if (clock_control_get_rate(config->media_axi_clk_dev, config->media_axi_clk_subsys,
				   &clk_freq)) {
		return -EINVAL;
	}
	LOG_DBG("media_axi clock frequency %d", clk_freq);

	/* configure media_apb_clk */
	if (!device_is_ready(config->media_apb_clk_dev)) {
		LOG_ERR("media_apb clock control device not ready");
		return -ENODEV;
	}
	clock_control_set_rate(config->media_apb_clk_dev, config->media_apb_clk_subsys,
			       (clock_control_subsys_rate_t)(uintptr_t)config->media_apb_clk_rate);
	if (clock_control_get_rate(config->media_apb_clk_dev, config->media_apb_clk_subsys,
				   &clk_freq)) {
		return -EINVAL;
	}
	LOG_DBG("media_apb clock frequency %d", clk_freq);

	return 0;
}

static int mcux_lcdifv3_init(const struct device *dev)
{
	const struct mcux_lcdifv3_config *config = dev->config;
	struct mcux_lcdifv3_data *data = dev->data;
	uint32_t clk_freq;

	DEVICE_MMIO_NAMED_MAP(dev, reg_base, K_MEM_CACHE_NONE | K_MEM_DIRECT_MAP);
	LCDIF_Type *base = (LCDIF_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);

	config->irq_config_func(dev);

	for (int i = 0; i < CONFIG_MCUX_LCDIFV3_FB_NUM; i++) {
		/* Record pointers to each driver framebuffer */
		data->fb[i] = config->fb_ptr + (config->fb_bytes * i);
	}
	data->active_fb = config->fb_ptr;

	k_sem_init(&data->sem, 1, 1);

	/* Clear external memory, as it is uninitialized */
	memset(config->fb_ptr, 0, config->fb_bytes * CONFIG_MCUX_LCDIFV3_FB_NUM);

	/* configure disp_pix_clk */
	if (!device_is_ready(config->disp_pix_clk_dev)) {
		LOG_ERR("cam_pix clock control device not ready");
		return -ENODEV;
	}

	mcux_axi_apb_configure_clock(dev);
	mcux_lcdifv3_configure_clock(dev);

	if (clock_control_get_rate(config->disp_pix_clk_dev, config->disp_pix_clk_subsys,
				   &clk_freq)) {
		LOG_ERR("Failed to get disp_pix_clk");
		return -EINVAL;
	}
	LOG_INF("disp_pix clock frequency %d", clk_freq);

	lcdifv3_buffer_config_t buffer_config = config->buffer_config;
	lcdifv3_display_config_t display_config = config->display_config;
	/* Set the Pixel format */
	if (config->pixel_format == PIXEL_FORMAT_BGR_565) {
		buffer_config.pixelFormat = kLCDIFV3_PixelFormatRGB565;
	} else if (config->pixel_format == PIXEL_FORMAT_RGB_888) {
		buffer_config.pixelFormat = kLCDIFV3_PixelFormatRGB888;
	} else if (config->pixel_format == PIXEL_FORMAT_ARGB_8888) {
		buffer_config.pixelFormat = kLCDIFV3_PixelFormatARGB8888;
	}

	LCDIFV3_Init(base);

	LCDIFV3_SetDisplayConfig(base, &display_config);
	LCDIFV3_EnableDisplay(base, true);
	LCDIFV3_SetLayerBufferConfig(base, 0, &buffer_config);
	LCDIFV3_SetLayerSize(base, 0, display_config.panelWidth, display_config.panelHeight);
	LCDIFV3_EnableLayer(base, 0, true);
	LCDIFV3_EnablePlanePanic(base);
	LCDIFV3_SetLayerBufferAddr(base, 0, (uint64_t)data->fb[0]);
	LCDIFV3_TriggerLayerShadowLoad(base, 0);
	LCDIFV3_EnableInterrupts(base, kLCDIFV3_VerticalBlankingInterrupt);

	LOG_INF("%s init succeeded", dev->name);

	dump_reg(base);

	return 0;
}

static const struct display_driver_api mcux_lcdifv3_api = {
	.write = mcux_lcdifv3_write,
	.get_framebuffer = mcux_lcdifv3_get_framebuffer,
	.get_capabilities = mcux_lcdifv3_get_capabilities,
};

#define GET_PIXEL_FORMAT(id)                                                                       \
	((DT_INST_ENUM_IDX(id, pixel_format) == 0)                                                 \
		 ? PIXEL_FORMAT_BGR_565                                                            \
		 : ((DT_INST_ENUM_IDX(id, pixel_format) == 1) ? PIXEL_FORMAT_RGB_888               \
							      : PIXEL_FORMAT_ARGB_8888))

#define GET_PIXEL_BYTES(id)                                                                        \
	((DT_INST_ENUM_IDX(id, pixel_format) == 0)                                                 \
		 ? 2                                                                               \
		 : ((DT_INST_ENUM_IDX(id, pixel_format) == 1) ? 3 : 4))

#define MCUX_LCDIFV3_FRAMEBUFFER_DECL(id)                                                          \
	uint8_t __aligned(64)                                                                      \
	mcux_lcdifv3_frame_buffer_##id[DT_INST_PROP(id, width) * DT_INST_PROP(id, height) *        \
				       GET_PIXEL_BYTES(id) * CONFIG_MCUX_LCDIFV3_FB_NUM]
#define MCUX_LCDIFV3_FRAMEBUFFER(id) mcux_lcdifv3_frame_buffer_##id

#define MCUX_LCDIFV3_DEVICE_INIT(id)                                                               \
	static void mcux_lcdifv3_config_func_##id(const struct device *dev)                        \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(id), DT_INST_IRQ(id, priority), mcux_lcdifv3_isr,         \
			    DEVICE_DT_INST_GET(id), 0);                                            \
		irq_enable(DT_INST_IRQN(id));                                                      \
	}                                                                                          \
	MCUX_LCDIFV3_FRAMEBUFFER_DECL(id);                                                         \
	static struct mcux_lcdifv3_data mcux_lcdifv3_data_##id = {                                 \
		.next_idx = 0,                                                                     \
		.pixel_bytes = GET_PIXEL_BYTES(id),                                                \
	};                                                                                         \
	static const struct mcux_lcdifv3_config mcux_lcdifv3_config_##id = {                       \
		DEVICE_MMIO_NAMED_ROM_INIT(reg_base, DT_DRV_INST(id)),                             \
		.disp_pix_clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_IDX(id, 0)),              \
		.disp_pix_clk_subsys =                                                             \
			(clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_IDX(id, 0, name),           \
		.disp_pix_clk_rate = DT_PROP(DT_INST_CHILD(id, display_timings), clock_frequency), \
		.media_axi_clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_IDX(id, 1)),             \
		.media_axi_clk_subsys =                                                            \
			(clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_IDX(id, 1, name),           \
		.media_axi_clk_rate = DT_INST_PROP(id, media_axi_clk_rate),                        \
		.media_apb_clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_IDX(id, 2)),             \
		.media_apb_clk_subsys =                                                            \
			(clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_IDX(id, 2, name),           \
		.media_apb_clk_rate = DT_INST_PROP(id, media_apb_clk_rate),                        \
		.irq_config_func = mcux_lcdifv3_config_func_##id,                                  \
		.buffer_config =                                                                   \
			{                                                                          \
				.strideBytes = GET_PIXEL_BYTES(id) * DT_INST_PROP(id, width),      \
			},                                                                         \
		.display_config =                                                                  \
			{                                                                          \
				.panelWidth = DT_INST_PROP(id, width),                             \
				.panelHeight = DT_INST_PROP(id, height),                           \
				.lineOrder = kLCDIFV3_LineOrderRGBOrYUV,                           \
				.hsw = DT_PROP(DT_INST_CHILD(id, display_timings), hsync_len),     \
				.hfp = DT_PROP(DT_INST_CHILD(id, display_timings), hfront_porch),  \
				.hbp = DT_PROP(DT_INST_CHILD(id, display_timings), hback_porch),   \
				.vsw = DT_PROP(DT_INST_CHILD(id, display_timings), vsync_len),     \
				.vfp = DT_PROP(DT_INST_CHILD(id, display_timings), vfront_porch),  \
				.vbp = DT_PROP(DT_INST_CHILD(id, display_timings), vback_porch),   \
				.polarityFlags =                                                   \
					(DT_PROP(DT_INST_CHILD(id, display_timings), hsync_active) \
						 ? kLCDIFV3_HsyncActiveLow                         \
						 : kLCDIFV3_HsyncActiveHigh) |                     \
					(DT_PROP(DT_INST_CHILD(id, display_timings), vsync_active) \
						 ? kLCDIFV3_VsyncActiveLow                         \
						 : kLCDIFV3_VsyncActiveHigh) |                     \
					(DT_PROP(DT_INST_CHILD(id, display_timings), de_active)    \
						 ? kLCDIFV3_DataEnableActiveLow                    \
						 : kLCDIFV3_DataEnableActiveHigh) |                \
					(DT_PROP(DT_INST_CHILD(id, display_timings),               \
						 pixelclk_active)                                  \
						 ? kLCDIFV3_DriveDataOnRisingClkEdge               \
						 : kLCDIFV3_DriveDataOnFallingClkEdge),            \
			},                                                                         \
		.pixel_format = GET_PIXEL_FORMAT(id),                                              \
		.fb_ptr = MCUX_LCDIFV3_FRAMEBUFFER(id),                                            \
		.fb_bytes =                                                                        \
			DT_INST_PROP(id, width) * DT_INST_PROP(id, height) * GET_PIXEL_BYTES(id),  \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(id, &mcux_lcdifv3_init, NULL, &mcux_lcdifv3_data_##id,               \
			      &mcux_lcdifv3_config_##id, POST_KERNEL,                              \
			      CONFIG_DISPLAY_INIT_PRIORITY, &mcux_lcdifv3_api);

DT_INST_FOREACH_STATUS_OKAY(MCUX_LCDIFV3_DEVICE_INIT)
