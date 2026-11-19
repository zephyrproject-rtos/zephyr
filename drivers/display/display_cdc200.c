/*
 * Copyright (C) 2024 Alif Semiconductor.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "zephyr/kernel/internal/mm.h"
#define DT_DRV_COMPAT tes_cdc_2_1

#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>

#include <string.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/device_mmio.h>
#include "display_cdc200.h"
#include <zephyr/cache.h>
#include <zephyr/linker/devicetree_regions.h>

LOG_MODULE_REGISTER(CDC200, CONFIG_DISPLAY_LOG_LEVEL);

/* Helper functions */
/* Global configuration setup functions. */
static void cdc200_shadow_reload_control(uintptr_t regs)
{
	sys_write32(CDC_SRCTRL_IMR, regs + CDC_SRCTRL);
}

static void cdc200_set_irq_mask(uintptr_t regs, uint32_t irqs)
{
	sys_write32(irqs, regs + CDC_IRQ_MASK0);
}

uint32_t cdc200_get_irq_status(uintptr_t regs)
{
	return sys_read32(regs + CDC_IRQ_STATUS0);
}

static void cdc200_set_irq_clear(uintptr_t regs, uint32_t irqs)
{
	sys_write32(irqs, regs + CDC_IRQ_CLEAR0);
}

static void cdc200_global_enable(uintptr_t regs)
{
	uint32_t temp = sys_read32(regs + CDC_GLB_CTRL);

	temp |= CDC_GLB_CTRL_CDC_EN;
	sys_write32(temp, regs + CDC_GLB_CTRL);
}

static void cdc200_global_disable(uintptr_t regs)
{
	uint32_t temp = sys_read32(regs + CDC_GLB_CTRL);

	temp &= ~CDC_GLB_CTRL_CDC_EN;
	sys_write32(temp, regs + CDC_GLB_CTRL);
}

static void cdc200_irq_setup(const struct device *dev)
{
	uintptr_t regs = DEVICE_MMIO_GET(dev);
	const struct cdc200_config *config = dev->config;
	const struct cdc200_panel_config *panel_cfg = &config->panel_cfg;
	uint32_t linpos =
		(panel_cfg->vsync_len + panel_cfg->vbp + panel_cfg->active_height) & 0xffff;

	/* Enable just the Line IRQ. */
	sys_write32(linpos, regs + CDC_LINE_IRQ_POS);
	cdc200_set_irq_mask(regs, CDC_IRQ_LINE);
}

static void cdc200_global_timings_set(const struct device *dev)
{
	uintptr_t regs = DEVICE_MMIO_GET(dev);
	const struct cdc200_config *config = dev->config;
	const struct cdc200_panel_config *panel_cfg = &config->panel_cfg;

	uint32_t sync_size = (((panel_cfg->hsync_len - 1) & 0xffff) << 16) +
			     ((panel_cfg->vsync_len - 1) & 0xffff);
	uint32_t back_porch =
		(((panel_cfg->hbp) & 0xffff) << 16) + ((panel_cfg->vbp) & 0xffff) + sync_size;
	uint32_t active_width = ((panel_cfg->active_width & 0xffff) << 16) +
				(panel_cfg->active_height & 0xffff) + back_porch;
	uint32_t total_width =
		((panel_cfg->hfp & 0xffff) << 16) + (panel_cfg->vfp & 0xffff) + active_width;

	/* Setup global signal polarity. */
	sys_write32(panel_cfg->polarity, regs + CDC_GLB_CTRL);

	/* Setup Global Timing registers. */
	sys_write32(sync_size, regs + CDC_SYNC_SIZE_CFG);
	sys_write32(back_porch, regs + CDC_BP_CFG);
	sys_write32(active_width, regs + CDC_ACTW_CFG);
	sys_write32(total_width, regs + CDC_TOTALW_CFG);
}

static void cdc200_global_set_bg_color(const struct device *dev)
{
	uintptr_t regs = DEVICE_MMIO_GET(dev);
	const struct cdc200_config *config = dev->config;

	sys_write32(config->bg_color, regs + CDC_BACKGND_COLOR);
}

/* Layer setup functions. */
static void cdc200_layer_enable(uintptr_t regs, enum cdc_layer idx)
{
	uint32_t temp = 0;

	switch (idx) {
	case CDC_LAYER_1:
		temp = sys_read32(regs + CDC_L1_CTRL);
		temp |= CDC_LN_CTRL_LAYER_EN;
		sys_write32(temp, regs + CDC_L1_CTRL);
		break;
	case CDC_LAYER_2:
		temp = sys_read32(regs + CDC_L2_CTRL);
		temp |= CDC_LN_CTRL_LAYER_EN;
		sys_write32(temp, regs + CDC_L2_CTRL);
		break;
	default:
		LOG_ERR("Incorrect Layer ID.");
	}
}

#if defined(CONFIG_PM_DEVICE)

static void cdc200_layer_disable(uintptr_t regs, enum cdc_layer idx)
{
	uint32_t temp = 0;

	switch (idx) {
	case CDC_LAYER_1:
		temp = sys_read32(regs + CDC_L1_CTRL);
		temp &= ~CDC_LN_CTRL_LAYER_EN;
		sys_write32(temp, regs + CDC_L1_CTRL);
		break;
	case CDC_LAYER_2:
		temp = sys_read32(regs + CDC_L2_CTRL);
		temp &= ~CDC_LN_CTRL_LAYER_EN;
		sys_write32(temp, regs + CDC_L2_CTRL);
		break;
	default:
		LOG_ERR("Incorrect Layer ID.");
	}
}

#endif /* CONFIG_PM_DEVICE */

static void cdc200_layer_blend_cfg_set(uintptr_t regs, enum cdc_layer idx, int f1, int f2)
{
	switch (idx) {
	case CDC_LAYER_1:
		sys_write32((f1 & CDC_LN_BLEND_CFG_F1_SEL_MASK) << CDC_LN_BLEND_CFG_F1_SEL_SHIFT |
				    (f2 & CDC_LN_BLEND_CFG_F2_SEL_MASK),
			    regs + CDC_L1_BLEND_CFG);
		break;
	case CDC_LAYER_2:
		sys_write32((f1 & CDC_LN_BLEND_CFG_F1_SEL_MASK) << CDC_LN_BLEND_CFG_F1_SEL_SHIFT |
				    (f2 & CDC_LN_BLEND_CFG_F2_SEL_MASK),
			    regs + CDC_L2_BLEND_CFG);
		break;
	default:
		LOG_ERR("Incorrect Layer ID.");
	}
}

static void cdc200_layer_const_alpha_set(uintptr_t regs, enum cdc_layer idx, int alpha)
{
	switch (idx) {
	case CDC_LAYER_1:
		sys_write32(alpha & CDC_LN_CONST_ALPHA_CONSTA_MASK, regs + CDC_L1_CONST_ALPHA);
		break;
	case CDC_LAYER_2:
		sys_write32(alpha & CDC_LN_CONST_ALPHA_CONSTA_MASK, regs + CDC_L2_CONST_ALPHA);
		break;
	default:
		LOG_ERR("Incorrect Layer ID.");
	}
}

static int cdc200_layer_fb_setup(const struct device *dev, enum cdc_layer idx)
{
	const struct cdc200_config *config = dev->config;
	const struct cdc200_layer_config *layer = config->layer + idx;
	uintptr_t regs = DEVICE_MMIO_GET(dev);

	const uint32_t fb_len = (layer->x1 - layer->x0) * layer->pixel_size;
	const uint32_t fb_llen = fb_len + BUS_WIDTH;

	switch (idx) {
	case CDC_LAYER_1:
		sys_write32((uint32_t)k_mem_phys_addr(layer->fb), regs + CDC_L1_CFB_ADDR);
		sys_write32(layer->fmt, regs + CDC_L1_PIX_FORMAT);
		sys_write32(
			((fb_len & CDC_LN_CFB_LENGTH_PITCH_MASK) << CDC_LN_CFB_LENGTH_PITCH_SHIFT) |
				(fb_llen & CDC_LN_CFB_LENGTH_LLENGTH_MASK),
			regs + CDC_L1_CFB_LENGTH);
		sys_write32(layer->y1 - layer->y0, regs + CDC_L1_CFB_LINES);
		break;
	case CDC_LAYER_2:
		sys_write32((uint32_t)k_mem_phys_addr(layer->fb), regs + CDC_L2_CFB_ADDR);
		sys_write32(layer->fmt, regs + CDC_L2_PIX_FORMAT);
		sys_write32(
			((fb_len & CDC_LN_CFB_LENGTH_PITCH_MASK) << CDC_LN_CFB_LENGTH_PITCH_SHIFT) |
				(fb_llen & CDC_LN_CFB_LENGTH_LLENGTH_MASK),
			regs + CDC_L2_CFB_LENGTH);
		sys_write32(layer->y1 - layer->y0, regs + CDC_L2_CFB_LINES);
		break;
	default:
		LOG_ERR("Incorrect Layer ID.");
	}
	return 0;
}

static int cdc200_layer_dflt_color_set(const struct device *dev, enum cdc_layer idx)
{
	const struct cdc200_config *config = dev->config;
	const struct cdc200_layer_config *layer = config->layer + idx;
	const struct cdc200_panel_config *panel_cfg = &config->panel_cfg;
	uintptr_t regs = DEVICE_MMIO_GET(dev);
	uint32_t temp = 0;

	const uint32_t lw = layer->x1 - layer->x0;
	const uint32_t lh = layer->y1 - layer->y0;

	/*
	 * If the default color is not enabled or
	 * layer is enabled with no windowing - skip default layer color.
	 * Else, setup default layer color & enable blending with it.
	 */
	if (!layer->def_color_en || (layer->layer_en && (!(lw - panel_cfg->active_width) &&
							 !(lh - panel_cfg->active_height)))) {
		return 0;
	}

	switch (idx) {
	case CDC_LAYER_1:
		temp = sys_read32(regs + CDC_L1_CTRL);
		temp |= CDC_LN_CTRL_DFLT_CBLEND_EN;
		sys_write32(temp, regs + CDC_L1_CTRL);
		sys_write32(layer->def_color, regs + CDC_L1_DFLT_COLOR);
		break;
	case CDC_LAYER_2:
		temp = sys_read32(regs + CDC_L2_CTRL);
		temp |= CDC_LN_CTRL_DFLT_CBLEND_EN;
		sys_write32(temp, regs + CDC_L2_CTRL);
		sys_write32(layer->def_color, regs + CDC_L2_DFLT_COLOR);
		break;
	default:
		LOG_ERR("Incorrect Layer ID.");
	}
	return 0;
}

static void cdc200_layer_blending_setup(const struct device *dev, enum cdc_layer idx)
{
	const struct cdc200_config *config = dev->config;
	const struct cdc200_layer_config *layer = config->layer + idx;
	uintptr_t regs = DEVICE_MMIO_GET(dev);

	switch (layer->fmt) {
	case CDC_PIXEL_FORMAT_RGB888:
	case CDC_PIXEL_FORMAT_RGB565:
		cdc200_layer_blend_cfg_set(regs, idx, CDC_BLEND_CONST_ALPHA,
					   CDC_BLEND_CONST_ALPHA_INV);
		break;
	default:
		cdc200_layer_blend_cfg_set(regs, idx, layer->curr_layer_bf_f1,
					   layer->subj_layer_bf_f2);
	}
	cdc200_layer_const_alpha_set(regs, idx, layer->const_alpha);
}

static int cdc200_layer_window_set(const struct device *dev, enum cdc_layer idx)
{
	const struct cdc200_config *config = dev->config;
	const struct cdc200_layer_config *layer = config->layer + idx;
	const struct cdc200_panel_config *panel_cfg = &config->panel_cfg;
	uintptr_t regs = DEVICE_MMIO_GET(dev);

	uint32_t layer_hstart = panel_cfg->hsync_len + panel_cfg->hbp + layer->x0;
	uint32_t layer_hend = panel_cfg->hsync_len + panel_cfg->hbp + layer->x1 - 1;
	uint32_t layer_vstart = panel_cfg->vsync_len + panel_cfg->vbp + layer->y0;
	uint32_t layer_vend = panel_cfg->vsync_len + panel_cfg->vbp + layer->y1 - 1;

	if ((layer->x1 > panel_cfg->active_width) || (layer->y1 > panel_cfg->active_height)) {
		LOG_ERR("Incorrect window parameters for layer %d", idx + 1);
		return -EINVAL;
	}

	switch (idx) {
	case CDC_LAYER_1:
		sys_write32((layer_hstart & CDC_LN_WIN_HPOS_START_POS_MASK) |
				    ((layer_hend & CDC_LN_WIN_HPOS_STOP_POS_MASK)
				     << CDC_LN_WIN_HPOS_STOP_POS_SHIFT),
			    regs + CDC_L1_WIN_HPOS);
		sys_write32((layer_vstart & CDC_LN_WIN_VPOS_START_POS_MASK) |
				    ((layer_vend & CDC_LN_WIN_VPOS_STOP_POS_MASK)
				     << CDC_LN_WIN_VPOS_STOP_POS_SHIFT),
			    regs + CDC_L1_WIN_VPOS);
		break;
	case CDC_LAYER_2:
		sys_write32((layer_hstart & CDC_LN_WIN_HPOS_START_POS_MASK) |
				    ((layer_hend & CDC_LN_WIN_HPOS_STOP_POS_MASK)
				     << CDC_LN_WIN_HPOS_STOP_POS_SHIFT),
			    regs + CDC_L2_WIN_HPOS);
		sys_write32((layer_vstart & CDC_LN_WIN_VPOS_START_POS_MASK) |
				    ((layer_vend & CDC_LN_WIN_VPOS_STOP_POS_MASK)
				     << CDC_LN_WIN_VPOS_STOP_POS_SHIFT),
			    regs + CDC_L2_WIN_VPOS);
		break;
	default:
		LOG_ERR("Incorrect Layer ID.");
	}
	return 0;
}

static int cdc200_setup_registers(const struct device *dev)
{
	uintptr_t regs = DEVICE_MMIO_GET(dev);
	const struct cdc200_config *config = dev->config;
	const struct cdc200_layer_config *layer = config->layer;
	int ret = 0;

	/* Layer(s) setup */
	for (int i = CDC_LAYER_1; i <= CDC_LAYER_2; i++) {
		cdc200_layer_dflt_color_set(dev, i);
		cdc200_shadow_reload_control(regs);
		cdc200_layer_blending_setup(dev, i);
		if (layer[i].layer_en) {
			ret = cdc200_layer_window_set(dev, i);
			if (ret) {
				LOG_ERR("Failed setting window on layer - %d", (i + 1));
				return ret;
			}
			cdc200_layer_fb_setup(dev, i);
			cdc200_layer_enable(regs, i);
		}
		cdc200_shadow_reload_control(regs);
	}

	/* Global registers setup */
	cdc200_global_timings_set(dev);
	cdc200_global_set_bg_color(dev);
	cdc200_irq_setup(dev);
	return 0;
}

/* API functions */
/* Generic APIs */
static int cdc200_blanking_on(const struct device *dev)
{
	const struct cdc200_config *config = dev->config;

	return display_blanking_on(config->panel_dev);
}

static int cdc200_blanking_off(const struct device *dev)
{
	const struct cdc200_config *config = dev->config;

	return display_blanking_off(config->panel_dev);
}

static int cdc200_generic_write(const struct device *dev, const uint16_t x, const uint16_t y,
			 const struct display_buffer_descriptor *desc, const void *buf)
{
	const struct cdc200_config *config = dev->config;
	/* Always get config for Layer 0 by default. */
	const struct cdc200_layer_config *layer = config->layer;
	struct cdc200_data *data = dev->data;
	const uint8_t *src = buf;
	uint32_t width;
	uint8_t *dst, *fb;
	uint8_t *dst_start;
	uint16_t row;
	size_t pix_size;
	bool wait_vsync = false;

	width = layer->x1 - layer->x0;
	pix_size = layer->pixel_size;

	if (desc->height == config->panel_cfg.active_height &&
	    desc->width == config->panel_cfg.active_width &&
	    desc->pitch == desc->width) {
		/* Full frame- use buffer directly */
		dst_start = fb = (uint8_t *)buf;
		dst = ((uint8_t *)(buf)) + desc->buf_size;
		wait_vsync = true;
	} else {
		if (!IS_ENABLED(CONFIG_CDC200_DRIVER_FRAMEBUFFER)) {
			LOG_ERR("Partial frame update requires driver framebuffer support");
			return -ENOTSUP;
		}
		/* Partial frame- use driver buffer with partial copy */
		fb = dst = layer->fb;
		dst += (y * width + x) * pix_size;
		dst_start = dst;
		for (row = 0; row < desc->height; row++) {
			memcpy(dst, src, desc->width * layer->pixel_size);
			dst += (width * layer->pixel_size);
			src += (desc->pitch * layer->pixel_size);
		}
	}

	data->next_fb[CDC_LAYER_1] = fb;
	sys_cache_data_flush_range(dst_start, (size_t)(dst - dst_start));

	/* Wait for vsync to indicate buffer is loaded */
	if (wait_vsync) {
		k_sem_take(&data->vsync_sem, K_FOREVER);
	}

	return 0;
}

static int cdc200_generic_read(const struct device *dev, const uint16_t x, const uint16_t y,
			const struct display_buffer_descriptor *desc, void *buf)
{
	const struct cdc200_config *config = dev->config;
	/* Always get config for Layer 0 by default. */
	const struct cdc200_layer_config *layer = config->layer;
	struct cdc200_data *data = dev->data;
	const uint8_t *src;
	uint8_t *dst = buf;
	uint32_t width;
	uint16_t row;
	size_t pix_size;

	width = layer->x1 - layer->x0;
	pix_size = layer->pixel_size;

	src = data->curr_fb[CDC_LAYER_1];
	src += (y * width + x) * pix_size;

	for (row = 0; row < desc->height; row++) {
		memcpy(dst, src, desc->width * layer->pixel_size);
		src += (width * layer->pixel_size);
		dst += (desc->pitch * layer->pixel_size);
	}

	return 0;
}

static void *cdc200_generic_get_fb(const struct device *dev)
{
	struct cdc200_data *data = dev->data;
	void *ret = NULL;

	ret = (void *)data->curr_fb[CDC_LAYER_1];

	return ret;
}

static void cdc200_generic_get_caps(const struct device *dev,
				    struct display_capabilities *capabilities)
{
	const struct cdc200_config *config = dev->config;

	memset(capabilities, 0, sizeof(*capabilities));
	capabilities->x_resolution = config->panel_cfg.active_width;
	capabilities->y_resolution = config->panel_cfg.active_height;
	capabilities->supported_pixel_formats =
		PIXEL_FORMAT_ARGB_8888 | PIXEL_FORMAT_RGB_888 | PIXEL_FORMAT_RGB_565;
	capabilities->current_orientation = DISPLAY_ORIENTATION_NORMAL;

	switch (config->layer->fmt) {
	case CDC_PIXEL_FORMAT_ARGB8888:
		capabilities->current_pixel_format = PIXEL_FORMAT_ARGB_8888;
		break;
	case CDC_PIXEL_FORMAT_RGB888:
		capabilities->current_pixel_format = PIXEL_FORMAT_RGB_888;
		break;
	case CDC_PIXEL_FORMAT_RGB565:
		capabilities->current_pixel_format = PIXEL_FORMAT_RGB_565;
		break;
	default:
		break;
	}
}

static int cdc200_set_pixel_format(const struct device *dev, const enum display_pixel_format format)
{
	const struct cdc200_config *config = dev->config;
	const struct cdc200_layer_config *layer = config->layer;
	uint32_t pixel_fmt = 0;

	switch (layer->fmt) {
	case CDC_PIXEL_FORMAT_ARGB8888:
		pixel_fmt = PIXEL_FORMAT_ARGB_8888;
		break;
	case CDC_PIXEL_FORMAT_RGB888:
		pixel_fmt = PIXEL_FORMAT_RGB_888;
		break;
	case CDC_PIXEL_FORMAT_RGB565:
		pixel_fmt = PIXEL_FORMAT_RGB_565;
		break;
	default:
		break;
	}

	if (format == pixel_fmt) {
		return 0;
	}
	LOG_INF("Pixel Format change not supported.");
	return -ENOTSUP;
}

/* ISR function */
static void cdc200_isr(const struct device *dev)
{
	uintptr_t regs = DEVICE_MMIO_GET(dev);
	uint32_t irq_st = cdc200_get_irq_status(regs);
	struct cdc200_data *data = dev->data;

	if (irq_st & CDC_IRQ_LINE) {
		cdc200_set_irq_clear(regs, CDC_IRQ_LINE);

		bool swapped = false;

		if (data->curr_fb[CDC_LAYER_1] != data->next_fb[CDC_LAYER_1]) {
			sys_write32((uint32_t)k_mem_phys_addr(
				data->next_fb[CDC_LAYER_1]), regs + CDC_L1_CFB_ADDR);
			data->curr_fb[CDC_LAYER_1] = data->next_fb[CDC_LAYER_1];
			swapped = true;
		}

		if (data->curr_fb[CDC_LAYER_2] != data->next_fb[CDC_LAYER_2]) {
			sys_write32((uint32_t)k_mem_phys_addr(
				data->next_fb[CDC_LAYER_2]), regs + CDC_L2_CFB_ADDR);
			data->curr_fb[CDC_LAYER_2] = data->next_fb[CDC_LAYER_2];
			swapped = true;
		}
		cdc200_shadow_reload_control(regs);

		if (swapped) {
			k_sem_give(&data->vsync_sem);
		}
	}
}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(clocks)
static int cdc200_setup_pixel_clock(const struct device *dev)
{
	const struct cdc200_config *config = dev->config;
	struct cdc200_data *data = dev->data;
	int ret;

	ret = clock_control_on(config->clk_dev, config->pix_cid);
	if (ret) {
		LOG_ERR("Pixel-CLK source configuration failed! ret - %d", ret);
		return ret;
	}

	ret = clock_control_set_rate(config->clk_dev, config->pix_cid,
			(clock_control_subsys_rate_t)data->clk_freq);
	if (ret) {
		LOG_ERR("Pixel-CLK set frequency failed! ret - %d", ret);
		return ret;
	}

	return 0;

}

static int cdc200_setup_clocks(const struct device *dev)
{
	const struct cdc200_config *config = dev->config;
	int ret;

	/* Enable DPI clock. */
	ret = clock_control_on(config->clk_dev, config->dpi_cid);
	if (ret) {
		LOG_ERR("Enable DPI clock source for APB & AXI interface failed! ret - %d", ret);
		return ret;
	}

	/* Setup and Enable Pixel clock. */
	ret = cdc200_setup_pixel_clock(dev);
	if (ret) {
		return ret;
	}

	return 0;
}

void cdc200_set_enable(const struct device *dev, bool enable)
{
	uintptr_t regs = DEVICE_MMIO_GET(dev);

	if (enable) {
		cdc200_global_enable(regs);
	} else {
		cdc200_global_disable(regs);
	}
}

static uint32_t cdc200_pixel_clock_valid(const struct device *dev)
{
	const struct cdc200_config *config = dev->config;
	const struct cdc200_panel_config *pcfg = &config->panel_cfg;
	struct cdc200_data *data = dev->data;
	uint32_t htotal = 0;
	uint32_t vtotal = 0;

	htotal = (pcfg->hsync_len + pcfg->hbp + pcfg->hfp + pcfg->active_width);
	vtotal = (pcfg->vsync_len + pcfg->vbp + pcfg->vfp + pcfg->active_height);

	LOG_DBG("Min pixel clock for 1 FPS required: %d", htotal * vtotal);
	LOG_DBG("Expected FPS: %f", (double)data->clk_freq / (htotal * vtotal));
	return ((htotal * vtotal) <= data->clk_freq);
}
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(clocks) */

/* Init function */
static int cdc200_init(const struct device *dev)
{
	const struct cdc200_config *config = dev->config;
	const struct cdc200_panel_config *pcfg = &config->panel_cfg;
	const struct cdc200_layer_config *layer = NULL;
	struct cdc200_data *data = dev->data;
	int ret;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
	LOG_DBG("MMIO address - 0x%x", (uint32_t)DEVICE_MMIO_GET(dev));

	data->dev = dev;
	k_sem_init(&data->vsync_sem, 0, 1);

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(clocks)
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(clocks) */

#if !defined(CONFIG_MIPI_DSI)
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	LOG_DBG("Applying Pinctrl state. ret - %d", ret);
	if (ret < 0) {
		return ret;
	}
#endif /* !defined(CONFIG_MIPI_DSI) */

	/* Setup the CDC200 controller... */
	ret = cdc200_setup_registers(dev);
	if (ret) {
		return ret;
	}

	/* Enable controller */
	cdc200_set_enable(dev, true);

	config->irq_config_func(dev);

	LOG_DBG("Device setup parameters.");
	LOG_DBG("\tIRQ number for Scanline interrupt - %d", config->irq);
	LOG_DBG("\tEnabled Layers - %d", data->layer_count);
	LOG_DBG("\tBackground color - 0x%06x", config->bg_color);
	LOG_DBG("\thtimings(%d, %d, %d, %d)\tvtimings(%d, %d, %d, %d)",
			pcfg->hsync_len, pcfg->hbp, pcfg->active_width, pcfg->hfp,
			pcfg->vsync_len, pcfg->vbp, pcfg->active_height, pcfg->vfp);

	LOG_DBG("Layer Parameters.");
	for (uint32_t i = CDC_LAYER_1; i < CDC_LAYER_MAX; i++) {
		layer = config->layer + i;
		LOG_DBG("\tLayer %d FB-Addr - 0x%x", (i + 1), (uint32_t)layer->fb);
		LOG_DBG("\tLayer %d FB size - %d", (i + 1), layer->fb_size);
		LOG_DBG("\tLayer %d FB Format - %d", (i + 1), layer->fmt);

		LOG_DBG("\tLayer %d windowing (x0, y0) - (%d, %d)", (i + 1), layer->x0, layer->y0);
		LOG_DBG("\tLayer %d windowing (x1, y1) - (%d, %d)", (i + 1), layer->x1, layer->y1);
	}
	return 0;
}

static DEVICE_API(display, cdc200_display_api) = {
	.blanking_on = cdc200_blanking_on,
	.blanking_off = cdc200_blanking_off,
	.write = cdc200_generic_write,
	.read = cdc200_generic_read,
	.get_framebuffer = cdc200_generic_get_fb,
	.get_capabilities = cdc200_generic_get_caps,
	.set_pixel_format = cdc200_set_pixel_format,
};

#if defined(CONFIG_PM_DEVICE)

/**
 * @brief CDC200 suspend handler.
 *
 * Stops CDC200 display output, disables all active layers, and gates
 * the pixel and DPI clocks. Does not currently save layer/enable state;
 * resume unconditionally reprograms all registers from devicetree config.
 *
 * @param dev CDC200 device struct.
 * @return 0 on success, negative errno otherwise.
 */
static int cdc200_suspend(const struct device *dev)
{
	const struct cdc200_config *config = dev->config;
	uintptr_t regs = DEVICE_MMIO_GET(dev);
	int ret;

	/* Stop display fetch/output */
	cdc200_set_enable(dev, false);

	/* Disable all enabled layers */
	for (int i = CDC_LAYER_1; i < CDC_LAYER_MAX; i++) {
		cdc200_layer_disable(regs, i);
	}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(clocks)
	/* Stop pixel clock */
	ret = clock_control_off(config->clk_dev, config->pix_cid);
	if (ret) {
		LOG_ERR("Failed to disable pixel clock (%d)", ret);
		return ret;
	}

	/* Stop DPI/APB clock */
	ret = clock_control_off(config->clk_dev, config->dpi_cid);
	if (ret) {
		LOG_ERR("Failed to disable DPI clock (%d)", ret);
		return ret;
	}
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(clocks) */

#if !defined(CONFIG_MIPI_DSI)
	if (config->pcfg) {
		ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_SLEEP);
		if (ret < 0 && ret != -ENOENT) {
			LOG_ERR("Failed to apply sleep pinctrl state");
			return ret;
		}
	}
#endif /* !defined(CONFIG_MIPI_DSI) */

	LOG_DBG("PM: Suspended %s", dev->name);

	return 0;
}

/**
 * @brief CDC200 resume handler.
 *
 * Restores pinctrl, clocks, and register state.
 * All layers and outputs are reprogrammed unconditionally from config;
 * this does not currently track or restore the pre-suspend enable state.
 *
 * @param dev CDC200 device struct.
 * @return 0 on success, negative errno otherwise.
 */
static int cdc200_resume(const struct device *dev)
{
	int ret;

#if !defined(CONFIG_MIPI_DSI)
	const struct cdc200_config *config = dev->config;

	if (config->pcfg) {
		ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
		if (ret < 0 && ret != -ENOENT) {
			LOG_ERR("Failed to apply default pinctrl state");
			return ret;
		}
	}
#endif /* !defined(CONFIG_MIPI_DSI) */

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(clocks)
	/* Re-enable clocks */
	ret = cdc200_setup_clocks(dev);
	if (ret) {
		LOG_ERR("Failed to restore clocks");
		return ret;
	}
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(clocks) */

	/* Reprogram all CDC200 registers */
	ret = cdc200_setup_registers(dev);
	if (ret) {
		LOG_ERR("Failed to restore registers");
		return ret;
	}

	LOG_DBG("PM: Resumed %s", dev->name);

	return 0;
}

/**
 * @brief CDC200 PM device action handler
 *
 * Handles power management state transitions for the CDC200 device.
 *
 * @param dev CDC200 device struct
 * @param action PM device action
 * @return 0 if successful, negative errno otherwise
 */
static int cdc200_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		return cdc200_resume(dev);

	case PM_DEVICE_ACTION_SUSPEND:
		return cdc200_suspend(dev);

	case PM_DEVICE_ACTION_TURN_OFF:
	case PM_DEVICE_ACTION_TURN_ON:
		/* Power domain handling is automatic via the PM framework */
		return 0;

	default:
		return -ENOTSUP;
	}
}
#endif /* CONFIG_PM_DEVICE */

/***************** Per-layer Pixel size calculation Macros. ******************/
#define CDC200_PIXEL_SIZE_IDX_0         4 /* ARGB-8888 */
#define CDC200_PIXEL_SIZE_IDX_1         3 /* RGB-888 */
#define CDC200_PIXEL_SIZE_IDX_2         2 /* RGB-565 */
#define CDC200_PIXEL_SIZE_IDX_3         4 /* RGBA-8888 */
#define CDC200_PIXEL_SIZE_IDX_4         1 /* AL-44 */
#define CDC200_PIXEL_SIZE_IDX_5         1 /* L-8 */
#define CDC200_PIXEL_SIZE_IDX_6         2 /* ARGB-1555 */
#define CDC200_PIXEL_SIZE_IDX_7         2 /* ARGB-4444 */
#define CDC200_PIXEL_SIZE_IDX_8         0 /* Undefined */
#define CDC200_PIXEL_SIZE_IDX_UNDEFINED CDC200_PIXEL_SIZE_IDX_8

#define LAYER_PROPERTY(prop, i) UTIL_CAT(prop, i)

#define CDC200_PIXEL_SIZE_LAYER(i, n)                                                              \
	UTIL_CAT(CDC200_PIXEL_SIZE_IDX_,                                                           \
		 DT_INST_ENUM_IDX_OR(i, LAYER_PROPERTY(pixel_fmt_l, n), UNDEFINED))

#if DT_SAME_NODE(DT_CHOSEN(zephyr_sram), DT_NODELABEL(sram0))
/* Frame buffer doesn't need a custom section */
#define FRAME_BUFFER_SECTION
#else
#define FRAME_BUFFER_SECTION Z_GENERIC_SECTION(LINKER_DT_NODE_REGION_NAME(DT_NODELABEL(sram0)))
#endif

#define CDC200_PROP(i, name, fallback) \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(i, name), (DT_INST_PROP(i, name)), (fallback))

#define FB_SIZE(i, layer) \
	(CDC200_PIXEL_SIZE_LAYER(i, layer) * \
	 ((CDC200_PROP(i, win_x1_l##layer, DT_INST_PROP(i, width)) - \
	   CDC200_PROP(i, win_x0_l##layer, 0)) * \
	  (CDC200_PROP(i, win_y1_l##layer, DT_INST_PROP(i, height)) - \
	   CDC200_PROP(i, win_y0_l##layer, 0))))

#ifdef CONFIG_CDC200_DRIVER_FRAMEBUFFER

#define ALLOCATE_FB1(i)                                                                            \
	IF_ENABLED(                                                                                \
		DT_INST_PROP(i, enable_l1),                                                        \
		(static uint8_t FRAME_BUFFER_SECTION fb1_##i[FB_SIZE(i, 1)]))                     \

#define ALLOCATE_FB2(i)                                                                            \
	IF_ENABLED(                                                                                \
		DT_INST_PROP(i, enable_l2),                                                        \
		(static uint8_t FRAME_BUFFER_SECTION fb2_##i[FB_SIZE(i, 2)]))

#define FB1(i) COND_CODE_1(DT_INST_PROP(i, enable_l1), (fb1_##i), (NULL))

#define FB2(i) COND_CODE_1(DT_INST_PROP(i, enable_l2), (fb2_##i), (NULL))

#else

#define ALLOCATE_FB1(i)
#define ALLOCATE_FB2(i)
#define FB1(i) (NULL)
#define FB2(i) (NULL)
#endif

/********************** PinCtrl define. **********************/
#ifdef CONFIG_MIPI_DSI
#define CDC200_PINCTRL_INIT(n)
#define CDC200_PINCTRL_GET(n) (NULL)
#else
#define CDC200_PINCTRL_INIT(n) PINCTRL_DT_INST_DEFINE(n)
#define CDC200_PINCTRL_GET(n)  PINCTRL_DT_INST_DEV_CONFIG_GET(n)
#endif

#define CDC200_GET_CLK(i)                                                                         \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(i, clocks),                                              \
		(.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(i)),                                \
		 .dpi_cid = (clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_NAME(i,                \
			 dpi_clk_en, clkid),                                                      \
		 .pix_cid = (clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_NAME(i,                \
			 pixel_clk, clkid),))

/********************** Device Definition per instance Macros. ***********************/
#define CDC200_DEFINE(i)                                                                           \
	CDC200_PINCTRL_INIT(i);                                                                    \
	static void cdc200_config_func_##i(const struct device *dev);                              \
                                                                                                   \
	ALLOCATE_FB1(i);                                                                           \
	ALLOCATE_FB2(i);                                                                           \
	static const struct cdc200_config config##i = {                                            \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(i)),                                              \
                                                                                                   \
		.irq = DT_INST_IRQ_BY_NAME(i, scanline_0, irq),                                    \
		.irq_config_func = cdc200_config_func_##i,                                         \
		.pcfg = CDC200_PINCTRL_GET(i),                                                     \
		.bg_color = DT_INST_PROP(i, bg_color),                                             \
		.panel_dev = DEVICE_DT_GET(DT_INST_PHANDLE(i, panel_dev)),                         \
                                                                                                   \
		CDC200_GET_CLK(i)                                                                  \
                                                                                                   \
		.panel_cfg =                                                                       \
			{                                                                          \
				.polarity = COND_CODE_1(DT_PROP(DT_INST_CHILD(i, display_timings), \
							hsync_active),                             \
							CDC_GLB_CTRL_HSPOL, (0)) |                 \
					    COND_CODE_1(DT_PROP(DT_INST_CHILD(i, display_timings), \
							vsync_active),                             \
							CDC_GLB_CTRL_VSPOL, (0)) |                 \
					    COND_CODE_0(DT_PROP(DT_INST_CHILD(i, display_timings), \
							de_active),                                \
							CDC_GLB_CTRL_BLPOL, (0)) |                 \
					    COND_CODE_1(DT_PROP(DT_INST_CHILD(i, display_timings), \
							pixelclk_active),                          \
							CDC_GLB_CTRL_PCLKPOL, (0)),                \
                                                                                                   \
				.hsync_len = DT_PROP(DT_INST_CHILD(i, display_timings), hsync_len),\
				.vsync_len = DT_PROP(DT_INST_CHILD(i, display_timings), vsync_len),\
				                                                                   \
				.hbp = DT_PROP(DT_INST_CHILD(i, display_timings), hback_porch),    \
				.vbp = DT_PROP(DT_INST_CHILD(i, display_timings), vback_porch),    \
				                                                                   \
				.active_width = DT_INST_PROP(i, width),                            \
				.active_height = DT_INST_PROP(i, height),                          \
				                                                                   \
				.hfp = DT_PROP(DT_INST_CHILD(i, display_timings), hfront_porch),   \
				.vfp = DT_PROP(DT_INST_CHILD(i, display_timings), vfront_porch),   \
			},                                                                         \
		.layer[0] =                                                                        \
			{                                                                          \
				.layer_en = DT_INST_PROP(i, enable_l1),                            \
				.fb = FB1(i),                                                 \
				.fb_size = FB_SIZE(i, 1),                                          \
				.fmt = DT_INST_ENUM_IDX_OR(i, pixel_fmt_l1,                        \
							   CDC_PIXEL_FORMAT_UNDEFINED),            \
				.pixel_size = CDC200_PIXEL_SIZE_LAYER(i, 1),                       \
				.def_color_en = DT_INST_NODE_HAS_PROP(i, def_back_color_l1),       \
				.def_color =                                                       \
					COND_CODE_1(DT_INST_NODE_HAS_PROP(i, def_back_color_l1),   \
						    (DT_INST_PROP(i, def_back_color_l1)), (0)),    \
				.x0 = COND_CODE_1(DT_INST_NODE_HAS_PROP(i, win_x0_l1),             \
						  (DT_INST_PROP(i, win_x0_l1)),                    \
						  (0)),                                           \
				.y0 = COND_CODE_1(DT_INST_NODE_HAS_PROP(i, win_y0_l1),             \
						  (DT_INST_PROP(i, win_y0_l1)),                    \
						  (0)),                                           \
				.x1 = COND_CODE_1(DT_INST_NODE_HAS_PROP(i, win_x1_l1),             \
						  (DT_INST_PROP(i, win_x1_l1)),                    \
						  (DT_INST_PROP(i, width))),                       \
				.y1 = COND_CODE_1(DT_INST_NODE_HAS_PROP(i, win_y1_l1),             \
						  (DT_INST_PROP(i, win_y1_l1)),                    \
						  (DT_INST_PROP(i, height))),                      \
				.const_alpha = (uint8_t)DT_INST_PROP(i, const_alpha_l1),           \
				.curr_layer_bf_f1 = DT_INST_PROP(i, blend_factor1_l1),             \
				.subj_layer_bf_f2 = DT_INST_PROP(i, blend_factor2_l1),             \
			},                                                                         \
		.layer[1] =                                                                        \
			{                                                                          \
				.layer_en = DT_INST_PROP(i, enable_l2),                            \
				.fb = FB2(i),                                                 \
				.fb_size = FB_SIZE(i, 2),                                          \
				.fmt = DT_INST_ENUM_IDX_OR(i, pixel_fmt_l2,                        \
							   CDC_PIXEL_FORMAT_UNDEFINED),            \
				.pixel_size = CDC200_PIXEL_SIZE_LAYER(i, 2),                       \
				.def_color_en = DT_INST_NODE_HAS_PROP(i, def_back_color_l2),       \
				.def_color =                                                       \
					COND_CODE_1(DT_INST_NODE_HAS_PROP(i, def_back_color_l2),   \
						    (DT_INST_PROP(i, def_back_color_l2)), (0)),    \
				.x0 = COND_CODE_1(DT_INST_NODE_HAS_PROP(i, win_x0_l2),             \
						  (DT_INST_PROP(i, win_x0_l2)),                    \
						  (0)),                                           \
				.y0 = COND_CODE_1(DT_INST_NODE_HAS_PROP(i, win_y0_l2),             \
						  (DT_INST_PROP(i, win_y0_l2)),                    \
						  (0)),                                           \
				.x1 = COND_CODE_1(DT_INST_NODE_HAS_PROP(i, win_x1_l2),             \
						  (DT_INST_PROP(i, win_x1_l2)),                    \
						  (DT_INST_PROP(i, width))),                       \
				.y1 = COND_CODE_1(DT_INST_NODE_HAS_PROP(i, win_y1_l2),             \
						  (DT_INST_PROP(i, win_y1_l2)),                    \
						  (DT_INST_PROP(i, height))),                      \
				.const_alpha = (uint8_t)DT_INST_PROP(i, const_alpha_l2),           \
				.curr_layer_bf_f1 = DT_INST_PROP(i, blend_factor1_l2),             \
				.subj_layer_bf_f2 = DT_INST_PROP(i, blend_factor2_l2),             \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	static struct cdc200_data data##i = {                                                      \
		.layer_count = DT_INST_PROP(i, enable_l1) + DT_INST_PROP(i, enable_l2),            \
		.curr_fb[0] = FB1(i),                                                              \
		.next_fb[0] = FB1(i),                                                              \
		.curr_fb[1] = FB2(i),                                                              \
		.next_fb[1] = FB2(i),                                                              \
		.clk_freq = DT_PROP(DT_INST_CHILD(i, display_timings), clock_frequency)            \
	};                                                                                         \
                                                                                                   \
	IF_ENABLED(CONFIG_PM_DEVICE, (PM_DEVICE_DT_INST_DEFINE(i, cdc200_pm_action);))             \
	DEVICE_DT_INST_DEFINE(i, cdc200_init, PM_DEVICE_DT_INST_GET(i), &data##i, &config##i,      \
			      POST_KERNEL, CONFIG_CDC200_INIT_PRIORITY, &cdc200_display_api);      \
												   \
	static int cdc200_clocks_##i(void)                                                         \
	{                                                                                          \
		const struct device *dev = DEVICE_DT_INST_GET(i);                                  \
		int ret;                                                                           \
												   \
		if (!cdc200_pixel_clock_valid(dev)) {                                              \
			LOG_ERR("Incorrect Pixel Clock value supplied in DTS.");                   \
			return -EINVAL;                                                            \
		}                                                                                  \
		ret = cdc200_setup_clocks(dev);                                                    \
		if (ret) {                                                                         \
			LOG_ERR("CDC200 Clock enable failed! Exiting!");                           \
			return ret;                                                                \
		}                                                                                  \
		return 0;                                                                          \
	}                                                                                          \
	SYS_INIT(cdc200_clocks_##i, POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY);                    \
                                                                                                   \
	static void cdc200_config_func_##i(const struct device *dev)                               \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(i, scanline_0, irq),                               \
			    DT_INST_IRQ_BY_NAME(i, scanline_0, priority), cdc200_isr,              \
			    DEVICE_DT_INST_GET(i), 0);                                             \
		irq_enable(DT_INST_IRQ_BY_NAME(i, scanline_0, irq));                               \
	}

DT_INST_FOREACH_STATUS_OKAY(CDC200_DEFINE);
