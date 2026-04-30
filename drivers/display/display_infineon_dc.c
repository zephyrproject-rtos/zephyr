/*
 * Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/errno.h>
#define DT_DRV_COMPAT infineon_display_controller

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/mipi_dsi.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "cy_graphics.h"
#include "cy_sysclk.h"

LOG_MODULE_REGISTER(infineon_display_controller, CONFIG_DISPLAY_LOG_LEVEL);

K_SEM_DEFINE(dc_sem, 0, 1);

/* Is the GPU enabled */
#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(gcnano), infineon_gpu, okay)
#define GPU_ENABLED (1)
#else
#define GPU_ENABLED (0)
#endif

/* Address of the memory section that is reserved for frame buffers */
#define GFX_BUF_ADDR_BASE DT_REG_ADDR(DT_NODELABEL(gfx_mem))

/* Device max DPHY clock in Hz unit */
#define MIPI_MAX_PHY_CLK_HZ                           (2500000000UL)

cy_stc_mipidsi_display_params_t dsi_display_params = {0};
cy_stc_mipidsi_config_t dsi_config = {0};

cy_stc_gfx_layer_config_t GFXSS_graphics_layer = {
	.layer_type = GFX_LAYER_GRAPHICS,
	.buffer_address = (gctADDRESS *)GFX_BUF_ADDR_BASE,
	.uv_buffer_address = (gctADDRESS *)GFX_BUF_ADDR_BASE,
	.tiling_type = vivLINEAR,
	.pos_x = 0,
	.pos_y = 0,
	.zorder = 0,
	.layer_enable = true,
	.visibility = true,
};

cy_stc_gfx_dc_config_t GFXSS_dc_config = {
	.gfx_layer_config = &GFXSS_graphics_layer,
	.ovl0_layer_config = NULL,
	.ovl1_layer_config = NULL,
	.display_type = GFX_DISP_TYPE_DSI_DPI,
	.display_size = vivDISPLAY_CUSTOMIZED,
};

cy_stc_gfx_gpu_cfg_t GFXSS_gpu_config = {
	.enable = false,
};

cy_stc_gfx_config_t GFXSS_config = {
	.dc_cfg = &GFXSS_dc_config,
	.gpu_cfg = &GFXSS_gpu_config,
	.mipi_dsi_cfg = &dsi_config,
	.display_update_type = GFX_DOUBLE_BUFFER,
	.clockHz = 400000000U,
};

cy_stc_gfx_context_t gfx_context;

struct infineon_dc_config {
	const struct device *panel;
	/* DSI display config */
	uint32_t pixel_clock;
	uint16_t hsync_width;
	uint16_t hfp;
	uint16_t hbp;
	uint16_t vsync_width;
	uint16_t vfp;
	uint16_t vbp;
	/* DSI block config */
	uint32_t num_lanes;
	uint32_t per_lane_mbps;
	uint32_t in_pixel_format;
	cy_en_mipidsi_pixel_format_t dpi_fmt;
	uint32_t irq_num;
	void (*irq_config_func)(const struct device *dev);
};

#if defined(CONFIG_LV_USE_DRAW_VG_LITE) && (GPU_ENABLED == 1)
void z_vg_lite_isr_hook(const struct device *dev, uint32_t flags)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(flags);

	cy_stc_gfx_context_t dummy_ctx;

	Cy_GFXSS_Clear_GPU_Interrupt(GFXSS, &dummy_ctx);
}
#endif

static int ifx_dc_blanking_on(const struct device *dev)
{
	const struct infineon_dc_config *config = dev->config;

	return display_blanking_on(config->panel);
}

static int ifx_dc_blanking_off(const struct device *dev)
{
	const struct infineon_dc_config *config = dev->config;

	return display_blanking_off(config->panel);
}

static int ifx_dc_write(const struct device *dev, const uint16_t x,
				const uint16_t y,
				const struct display_buffer_descriptor *desc,
				const void *buf)
{
	static bool first = true;

	/* ignore the first frame buffer update to avoid displaying uninitialized data */
	if (first) {
		first = false;
		return 0;
	}

	/* Reset the semaphore to discard any stale DC interrupt signals,
	 * ensuring we only unblock on the interrupt that follows this
	 * frame buffer update.
	 */
	k_sem_reset(&dc_sem);

	/* The graphics subsystem takes care of shipping data to the display.
	 * All we have to do here is update the frame buffer pointer so that
	 * it can render the current frame
	 */
	Cy_GFXSS_Set_FrameBuffer(GFXSS, (uint32_t *)buf, &gfx_context);

	/* wait for the frame buffer update to complete */
	k_sem_take(&dc_sem, K_FOREVER);
	return 0;
}

static int ifx_dc_set_brightness(const struct device *dev, const uint8_t brightness)
{
	const struct infineon_dc_config *config = dev->config;

	return display_set_brightness(config->panel, brightness);
}

static void ifx_dc_get_capabilities(const struct device *dev,
					  struct display_capabilities *capabilities)
{
	const struct infineon_dc_config *config = dev->config;
	uint8_t bytes_per_pixel;

	display_get_capabilities(config->panel, capabilities);
	capabilities->current_pixel_format = config->in_pixel_format;

	/* determine the number of bytes per pixel based on the video format */
	switch (capabilities->current_pixel_format) {
	case PANEL_PIXEL_FORMAT_MONO01:
	case PANEL_PIXEL_FORMAT_MONO10:
	case PANEL_PIXEL_FORMAT_L_8:
		bytes_per_pixel = 1U;
		break;
	case PANEL_PIXEL_FORMAT_RGB_565:
	case PANEL_PIXEL_FORMAT_RGB_565X:
	case PANEL_PIXEL_FORMAT_AL_88:
		bytes_per_pixel = 2U;
		break;
	case PANEL_PIXEL_FORMAT_RGB_888:
		bytes_per_pixel = 3U;
		break;
	case PANEL_PIXEL_FORMAT_ARGB_8888:
	case PANEL_PIXEL_FORMAT_XRGB_8888:
		bytes_per_pixel = 4U;
		break;
	default:
		LOG_ERR("Unsupported pixel format %d", capabilities->current_pixel_format);
		bytes_per_pixel = 2U;
		break;
	}

	/* Make sure that the display width (in bytes) is a multiple of the stride length */
	uint8_t remainder_bytes = (capabilities->x_resolution * bytes_per_pixel) % 128;

	if (remainder_bytes != 0) {
		if ((128 - remainder_bytes) % bytes_per_pixel != 0) {
			LOG_ERR("Unsupported pixel format %d with x resolution %d",
				capabilities->current_pixel_format, capabilities->x_resolution);
		} else {
			capabilities->x_resolution += (128 - remainder_bytes) / bytes_per_pixel;
		}
	}
}

static int ifx_dc_init(const struct device *dev)
{
	const struct infineon_dc_config *config = dev->config;
	cy_en_gfx_status_t gfx_status;
	struct display_capabilities capabilities;

	if (!device_is_ready(config->panel)) {
		LOG_ERR("Panel device %s not ready!", config->panel->name);
		return -ENODEV;
	}

	/* set peri 1 group divider for GFXSS */
	Cy_SysClk_PeriGroupSetDivider((1 << 8) | 3, 3U);

	Cy_SysClk_PeriGroupSlaveInit(CY_MMIO_GFXSS_GPU_PERI_NR, CY_MMIO_GFXSS_GPU_GROUP_NR,
				CY_MMIO_GFXSS_GPU_SLAVE_NR, CY_MMIO_GFXSS_GPU_CLK_HF_NR);

	Cy_SysClk_PeriGroupSlaveInit(CY_MMIO_GFXSS_DC_PERI_NR, CY_MMIO_GFXSS_DC_GROUP_NR,
				CY_MMIO_GFXSS_DC_SLAVE_NR, CY_MMIO_GFXSS_DC_CLK_HF_NR);

	Cy_SysClk_PeriGroupSlaveInit(CY_MMIO_GFXSS_MIPIDSI_PERI_NR, CY_MMIO_GFXSS_MIPIDSI_GROUP_NR,
				CY_MMIO_GFXSS_MIPIDSI_SLAVE_NR, CY_MMIO_GFXSS_MIPIDSI_CLK_HF_NR);

	/* get the panel capabilities so that we can know the output pixel format */
	display_get_capabilities(config->panel, &capabilities);

	/* Map the panel pixel format to the mipi dsi output pixel format */
	switch (capabilities.current_pixel_format) {
	case PANEL_PIXEL_FORMAT_RGB_888:
		dsi_config.dpi_fmt = CY_MIPIDSI_FMT_RGB888;
		GFXSS_dc_config.display_format = vivD24;
		break;
	case PANEL_PIXEL_FORMAT_RGB_565:
		dsi_config.dpi_fmt = CY_MIPIDSI_FMT_RGB565;
		GFXSS_dc_config.display_format = vivD16CFG2;
		break;
	default:
		LOG_ERR("Unsupported pixel format %d", capabilities.current_pixel_format);
		return -ENOTSUP;
	}

	/* get the display controller capabilities, which may have updated x-resolution
	 * values based on stride limitations
	 */
	ifx_dc_get_capabilities(dev, &capabilities);

	/* configure the mipi dsi interface */
	dsi_display_params.pixel_clock    = config->pixel_clock;
	dsi_display_params.hdisplay       = capabilities.x_resolution;
	dsi_display_params.hsync_width    = config->hsync_width;
	dsi_display_params.hfp            = config->hfp;
	dsi_display_params.hbp            = config->hbp;
	dsi_display_params.vdisplay       = capabilities.y_resolution;
	dsi_display_params.vsync_width    = config->vsync_width;
	dsi_display_params.vfp            = config->vfp;
	dsi_display_params.vbp            = config->vbp;

	dsi_config.num_of_lanes   = config->num_lanes;
	dsi_config.per_lane_mbps  = config->per_lane_mbps;
	dsi_config.dsi_mode       = DSI_VIDEO_MODE;
	dsi_config.max_phy_clk    = MIPI_MAX_PHY_CLK_HZ;
	dsi_config.mode_flags     = VID_MODE_TYPE_BURST | ENABLE_LOW_POWER_CMD | ENABLE_LOW_POWER;
	dsi_config.display_params = &dsi_display_params;

	/* map the input format type to the display controller input format */
	switch (config->in_pixel_format) {
	case PANEL_PIXEL_FORMAT_RGB_565:
		GFXSS_graphics_layer.input_format_type = vivRGB565;
		break;
	case PIXEL_FORMAT_ARGB_8888:
		GFXSS_graphics_layer.input_format_type = vivARGB8888;
		break;
	default:
		LOG_ERR("Unsupported input pixel format %d", config->in_pixel_format);
		return -ENOTSUP;
	}

	/* update the display controller configuration with the attached panel size */
	GFXSS_config.dc_cfg->gfx_layer_config->width  = capabilities.x_resolution;
	GFXSS_config.dc_cfg->gfx_layer_config->height = capabilities.y_resolution;

	GFXSS_config.dc_cfg->display_width  = capabilities.x_resolution;
	GFXSS_config.dc_cfg->display_height = capabilities.y_resolution;

	/* enable the GPU if it is present in device tree */
	if (GPU_ENABLED) {
		GFXSS_config.gpu_cfg->enable = true;
	}

	gfx_status = Cy_GFXSS_Init(GFXSS, &GFXSS_config, &gfx_context);
	if (CY_GFX_BAD_PARAM == gfx_status) {
		return -EINVAL;
	} else if (CY_GFX_TIMEOUT == gfx_status) {
		return -EIO;
	}

	config->irq_config_func(dev);
	irq_enable(config->irq_num);

	if (GPU_ENABLED) {
		Cy_GFXSS_Clear_GPU_Interrupt(GFXSS, &gfx_context);
		Cy_GFXSS_Enable_GPU_Interrupt(GFXSS);
	}

	LOG_DBG("Infineon display controller init succeeded");

	return 0;
}

/* This function is executed in the interrupt context */
static void dc_isr(const struct device *dev)
{
	ARG_UNUSED(dev);

	cy_stc_gfx_context_t dummy_ctx;

	Cy_GFXSS_Clear_DC_Interrupt(GFXSS, &dummy_ctx);

	k_sem_give(&dc_sem);
}

static DEVICE_API(display, ifx_dc_api) = {
	.blanking_on = ifx_dc_blanking_on,
	.blanking_off = ifx_dc_blanking_off,
	.write = ifx_dc_write,
	.set_brightness = ifx_dc_set_brightness,
	.get_capabilities = ifx_dc_get_capabilities,
};

#define INFINEON_DISPLAY_CONTROLLER_DEFINE(id)                                                     \
                                                                                                   \
	static void ifx_gfxss_dc_irq_config_func##id(const struct device *dev)                     \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(id), DT_INST_IRQ(id, priority), dc_isr,                   \
			    DEVICE_DT_INST_GET(id), 0);                                            \
	}                                                                                          \
                                                                                                   \
	static const struct infineon_dc_config config_##id = {                                     \
		.panel = DEVICE_DT_GET(DT_PHANDLE(DT_DRV_INST(id), panel)),                        \
		.in_pixel_format = DT_INST_PROP(id, input_pixel_format),                           \
		.pixel_clock = DT_INST_PROP(id, pixel_clock_khz),                                  \
		.hsync_width = DT_INST_PROP(id, hsync_width),                                      \
		.hfp = DT_INST_PROP(id, hfp),                                                      \
		.hbp = DT_INST_PROP(id, hbp),                                                      \
		.vsync_width = DT_INST_PROP(id, vsync_width),                                      \
		.vfp = DT_INST_PROP(id, vfp),                                                      \
		.vbp = DT_INST_PROP(id, vbp),                                                      \
		.num_lanes = DT_INST_PROP(id, data_lanes),                                         \
		.per_lane_mbps = DT_INST_PROP(id, per_lane_mbps),                                  \
		.irq_num = DT_INST_IRQN(id),                                                       \
		.irq_config_func = ifx_gfxss_dc_irq_config_func##id,                               \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(id, ifx_dc_init, NULL, NULL, &config_##id,                           \
			      POST_KERNEL, 89, &ifx_dc_api);

DT_INST_FOREACH_STATUS_OKAY(INFINEON_DISPLAY_CONTROLLER_DEFINE)
