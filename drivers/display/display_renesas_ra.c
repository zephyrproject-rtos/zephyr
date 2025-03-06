/*
 * Copyright (c) 2024-2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_ra_glcdc

#include "display_renesas_ra.h"
#include "r_glcdc.h"
#include <zephyr/drivers/clock_control/renesas_ra_cgc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(display_renesas_ra, CONFIG_DISPLAY_LOG_LEVEL);

void glcdc_line_detect_isr(void);
struct display_ra_config {
	const struct pinctrl_dev_config *pincfg;
	const struct gpio_dt_spec backlight_gpio;
	const struct device *clock_dev;
	struct clock_control_ra_subsys_cfg clock_glcdc_subsys;
	uint16_t height;
	uint16_t width;
	uint32_t display_frame_size;
	enum display_pixel_format pixel_format;
	void (*irq_configure)(void);
};

struct display_ra_data {
	glcdc_instance_ctrl_t display_ctrl;
	display_cfg_t display_fsp_cfg;
	uint8_t *p_base;
	uint32_t frame_buffer_len;
	const uint8_t *pend_buf;
	const uint8_t *front_buf;
	uint8_t pixel_size;
	uint8_t *frame_buffer;
	struct k_sem sem;
};

static void renesas_ra_glcdc_isr(const struct device *dev)
{
	struct display_ra_data *data = dev->data;

	glcdc_line_detect_isr();
	if (data->front_buf != data->pend_buf) {
		data->front_buf = data->pend_buf;
		k_sem_give(&data->sem);
	}
}

static int ra_display_write(const struct device *dev, const uint16_t x, const uint16_t y,
			    const struct display_buffer_descriptor *desc, const void *buf)
{
	struct display_ra_data *data = dev->data;
	const struct display_ra_config *config = dev->config;
	uint8_t *dst = NULL;
	const uint8_t *src = buf;
	const uint8_t *l_pend_buf = NULL;
	uint16_t row;
	int err;

	__ASSERT(desc->width <= desc->pitch, "Pitch is smaller than width");
	__ASSERT((desc->pitch * BYTE_PER_PIXEL * desc->height) <= desc->buf_size,
		 "Input buffer too small");

	if (x == 0 && y == 0 && desc->height == DISPLAY_VSIZE && desc->width == DISPLAY_HSIZE &&
	    desc->pitch == DISPLAY_HSIZE) {
		l_pend_buf = buf;
	} else {
		if (CONFIG_RENESAS_RA_GLCDC_FB_NUM == 0) {
			LOG_ERR("Partial write requires internal frame buffer");
			return -ENOTSUP;
		}
		dst = data->frame_buffer;

		if (CONFIG_RENESAS_RA_GLCDC_FB_NUM == 2) {
			if (data->front_buf == data->frame_buffer) {
				dst = data->frame_buffer + data->frame_buffer_len;
			}

			memcpy(dst, data->front_buf, data->frame_buffer_len);
		}

		l_pend_buf = dst;

		/* dst = pointer to upper left pixel of the rectangle
		 *       to be updated in frame buffer.
		 */
		dst += (x * data->pixel_size);
		dst += (y * config->width * data->pixel_size);

		for (row = 0; row < desc->height; row++) {
			(void)memcpy(dst, src, desc->width * data->pixel_size);
			dst += (config->width * data->pixel_size);
			src += (desc->pitch * data->pixel_size);
		}
	}

	if (data->front_buf == l_pend_buf) {
		return 0;
	}

	k_sem_reset(&data->sem);

	data->pend_buf = l_pend_buf;

	err = R_GLCDC_BufferChange(&data->display_ctrl, (uint8_t *)data->pend_buf,
				   DISPLAY_FRAME_LAYER_1);
	if (err) {
		LOG_ERR("GLCDC buffer change failed");
		return -EIO;
	}

	k_sem_take(&data->sem, K_FOREVER);

	return 0;
}

static int ra_display_read(const struct device *dev, const uint16_t x, const uint16_t y,
			   const struct display_buffer_descriptor *desc, void *buf)
{
	struct display_ra_data *data = dev->data;
	const struct display_ra_config *config = dev->config;
	uint8_t *dst = buf;
	const uint8_t *src = data->front_buf;
	uint16_t row;

	/* src = pointer to upper left pixel of the rectangle to be read from frame buffer */
	src += (x * data->pixel_size);
	src += (y * config->width * data->pixel_size);

	for (row = 0; row < desc->height; row++) {
		(void)memcpy(dst, src, desc->width * data->pixel_size);
		src += (config->width * data->pixel_size);
		dst += (desc->pitch * data->pixel_size);
	}

	return 0;
}

static int ra_display_blanking_on(const struct device *dev)
{
	const struct display_ra_config *config = dev->config;
	int err;

	if (config->backlight_gpio.port != NULL) {
		err = gpio_pin_set_dt(&config->backlight_gpio, 0);
		if (err) {
			LOG_ERR("Disable backlight failed! (%d)", err);
			return err;
		}
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static int ra_display_blanking_off(const struct device *dev)
{
	const struct display_ra_config *config = dev->config;
	int err;

	if (config->backlight_gpio.port != NULL) {
		err = gpio_pin_set_dt(&config->backlight_gpio, 1);
		if (err) {
			LOG_ERR("Enable backlight failed! (%d)", err);
			return err;
		}
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static void ra_display_get_capabilities(const struct device *dev,
					struct display_capabilities *capabilities)
{
	const struct display_ra_config *config = dev->config;

	memset(capabilities, 0, sizeof(struct display_capabilities));
	capabilities->x_resolution = config->width;
	capabilities->y_resolution = config->height;
	capabilities->current_orientation = DISPLAY_ORIENTATION_NORMAL;
	capabilities->supported_pixel_formats =
		PIXEL_FORMAT_RGB_888 | PIXEL_FORMAT_ARGB_8888 | PIXEL_FORMAT_RGB_565;
	capabilities->current_pixel_format = (config->pixel_format == PIXEL_FORMAT_RGB_888)
						     ? PIXEL_FORMAT_ARGB_8888
						     : config->pixel_format;
}

static int ra_display_set_pixel_format(const struct device *dev,
				       const enum display_pixel_format pixel_format)
{
	const struct display_ra_config *config = dev->config;

	if (pixel_format == config->pixel_format) {
		return 0;
	}
	LOG_ERR("Pixel format changes must be set in dts at build time.");
	return -ENOTSUP;
}

static DEVICE_API(display, display_api) = {
	.blanking_on = ra_display_blanking_on,
	.blanking_off = ra_display_blanking_off,
	.get_capabilities = ra_display_get_capabilities,
	.set_pixel_format = ra_display_set_pixel_format,
	.write = ra_display_write,
	.read = ra_display_read,
};

static int display_init(const struct device *dev)
{
	const struct display_ra_config *config = dev->config;
	struct display_ra_data *data = dev->data;
	int err;

#if BSP_FEATURE_BSP_HAS_GRAPHICS_DOMAIN

	R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_OM_LPC_BATT);
	FSP_HARDWARE_REGISTER_WAIT(
		(R_SYSTEM->PDCTRGD & (R_SYSTEM_PDCTRGD_PDCSF_Msk | R_SYSTEM_PDCTRGD_PDPGSF_Msk)),
		R_SYSTEM_PDCTRGD_PDPGSF_Msk);
	R_SYSTEM->PDCTRGD = 0;
	FSP_HARDWARE_REGISTER_WAIT(
		(R_SYSTEM->PDCTRGD & (R_SYSTEM_PDCTRGD_PDCSF_Msk | R_SYSTEM_PDCTRGD_PDPGSF_Msk)),
		0);
	R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_OM_LPC_BATT);

#endif
	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		LOG_ERR("pin function initial failed");
		return err;
	}
	k_sem_init(&data->sem, 0, 1);

	err = clock_control_on(config->clock_dev,
			       (clock_control_subsys_t)&config->clock_glcdc_subsys);
	if (err) {
		LOG_ERR("Enable GLCDC clock failed!");
		return err;
	}

	err = R_GLCDC_Open(&data->display_ctrl, &data->display_fsp_cfg);
	if (err) {
		LOG_ERR("GLCDC open failed");
		return -EIO;
	}

	err = gpio_pin_configure_dt(&config->backlight_gpio, GPIO_OUTPUT_ACTIVE);
	if (err) {
		LOG_ERR("config backlight gpio failed");
		return err;
	}

	err = R_GLCDC_Start(&data->display_ctrl);
	if (err) {
		LOG_ERR("GLCDC start failed");
		return -EIO;
	}

	config->irq_configure();

	return 0;
}

#define IRQ_CONFIGURE_FUNC(id)                                                                     \
	static void glcdc_renesas_ra_configure_func_##id(void)                                     \
	{                                                                                          \
		R_ICU->IELSR[DT_INST_IRQ_BY_NAME(id, line, irq)] =                                 \
			BSP_PRV_IELS_ENUM(EVENT_GLCDC_LINE_DETECT);                                \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(id, line, irq),                                    \
			    DT_INST_IRQ_BY_NAME(id, line, priority), renesas_ra_glcdc_isr,         \
			    DEVICE_DT_INST_GET(id), 0);                                            \
		irq_enable(DT_INST_IRQ_BY_NAME(id, line, irq));                                    \
	}

#define IRQ_CONFIGURE_DEFINE(id) .irq_configure = glcdc_renesas_ra_configure_func_##id

#define RENESAS_RA_FRAME_BUFFER_LEN(id)                                                            \
	(BYTE_PER_PIXEL * DT_INST_PROP(id, height) * DT_INST_PROP(id, width))

#define RENESAS_RA_DEVICE_INIT(id)                                                                 \
	PINCTRL_DT_INST_DEFINE(id);                                                                \
	IRQ_CONFIGURE_FUNC(id)                                                                     \
	Z_GENERIC_SECTION(".sdram")                                                                \
	static uint8_t __aligned(64)                                                               \
	fb_background##id[CONFIG_RENESAS_RA_GLCDC_FB_NUM * RENESAS_RA_FRAME_BUFFER_LEN(id)];       \
	static const glcdc_extended_cfg_t display_extend_cfg##id = {                               \
		.tcon_hsync = GLCDC_TCON_PIN_1,                                                    \
		.tcon_vsync = GLCDC_TCON_PIN_0,                                                    \
		.tcon_de = GLCDC_TCON_PIN_2,                                                       \
		.correction_proc_order = GLCDC_CORRECTION_PROC_ORDER_BRIGHTNESS_CONTRAST2GAMMA,    \
		.clksrc = GLCDC_CLK_SRC_INTERNAL,                                                  \
		.clock_div_ratio = GLCDC_PANEL_CLK_DIVISOR_8,                                      \
		.dithering_mode = GLCDC_DITHERING_MODE_TRUNCATE,                                   \
		.dithering_pattern_A = GLCDC_DITHERING_PATTERN_11,                                 \
		.dithering_pattern_B = GLCDC_DITHERING_PATTERN_11,                                 \
		.dithering_pattern_C = GLCDC_DITHERING_PATTERN_11,                                 \
		.dithering_pattern_D = GLCDC_DITHERING_PATTERN_11,                                 \
		.phy_layer = NULL,                                                                 \
	};                                                                                         \
	static struct display_ra_data ra_data##id = {                                              \
		.frame_buffer = fb_background##id,                                                 \
		.frame_buffer_len = RENESAS_RA_FRAME_BUFFER_LEN(id),                               \
		.front_buf = fb_background##id,                                                    \
		.pend_buf = fb_background##id,                                                     \
		.pixel_size = BYTE_PER_PIXEL,                                                      \
		.p_base = (uint8_t *)&fb_background##id,                                           \
		.display_fsp_cfg = {                                                               \
			.input[0] = {.p_base = (uint32_t *)&fb_background##id,                     \
				     .hsize = DISPLAY_HSIZE,                                       \
				     .vsize = DISPLAY_VSIZE,                                       \
				     .hstride = DISPLAY_BUFFER_STRIDE_PIXELS_INPUT0,               \
				     .format =                                                     \
					     (INPUT_FORMAT_PIXEL == PANEL_PIXEL_FORMAT_RGB_565)    \
						     ? DISPLAY_IN_FORMAT_16BITS_RGB565             \
					     : (INPUT_FORMAT_PIXEL == PANEL_PIXEL_FORMAT_RGB_888)  \
						     ? DISPLAY_IN_FORMAT_32BITS_RGB888             \
						     : DISPLAY_IN_FORMAT_32BITS_ARGB8888,          \
				     .line_descending_enable = false,                              \
				     .lines_repeat_enable = false,                                 \
				     .lines_repeat_times = 0},                                     \
			.input[1] = {.p_base = NULL,                                               \
				     .hsize = DISPLAY_HSIZE,                                       \
				     .vsize = DISPLAY_VSIZE,                                       \
				     .hstride = DISPLAY_BUFFER_STRIDE_PIXELS_INPUT1,               \
				     .format = DISPLAY_IN_FORMAT_16BITS_RGB565,                    \
				     .line_descending_enable = false,                              \
				     .lines_repeat_enable = false,                                 \
				     .lines_repeat_times = 0},                                     \
			.layer[0] = {.coordinate = {.x = 0, .y = 0},                               \
				     .bg_color = {.byte = {.a = LAYER_ALPHA,                       \
							   .r = LAYER_RED,                         \
							   .g = LAYER_GREEN,                       \
							   .b = LAYER_BLUE}},                      \
				     .fade_control = DISPLAY_FADE_CONTROL_NONE,                    \
				     .fade_speed = 0},                                             \
			.layer[1] = {.coordinate = {.x = 0, .y = 0},                               \
				     .bg_color = {.byte = {.a = LAYER_ALPHA,                       \
							   .r = LAYER_RED,                         \
							   .g = LAYER_GREEN,                       \
							   .b = LAYER_BLUE}},                      \
				     .fade_control = DISPLAY_FADE_CONTROL_NONE,                    \
				     .fade_speed = 0},                                             \
			.output = {.htiming = {.total_cyc =                                        \
						       DT_INST_PROP(id, width) +                   \
						       DT_PROP(DT_INST_CHILD(id, display_timings), \
							       hback_porch) +                      \
						       DT_PROP(DT_INST_CHILD(id, display_timings), \
							       hfront_porch) +                     \
						       DT_PROP(DT_INST_CHILD(id, display_timings), \
							       hsync_len),                         \
					       .display_cyc = DT_INST_PROP(id, width),             \
					       .back_porch =                                       \
						       DT_PROP(DT_INST_CHILD(id, display_timings), \
							       hback_porch),                       \
					       .sync_width =                                       \
						       DT_PROP(DT_INST_CHILD(id, display_timings), \
							       hsync_len),                         \
					       .sync_polarity =                                    \
						       DT_PROP(DT_INST_CHILD(id, display_timings), \
							       hsync_active)},                     \
				   .vtiming = {.total_cyc =                                        \
						       DT_INST_PROP(id, height) +                  \
						       DT_PROP(DT_INST_CHILD(id, display_timings), \
							       vback_porch) +                      \
						       DT_PROP(DT_INST_CHILD(id, display_timings), \
							       vfront_porch) +                     \
						       DT_PROP(DT_INST_CHILD(id, display_timings), \
							       vsync_len),                         \
					       .display_cyc = DT_INST_PROP(id, height),            \
					       .back_porch =                                       \
						       DT_PROP(DT_INST_CHILD(id, display_timings), \
							       vback_porch),                       \
					       .sync_width =                                       \
						       DT_PROP(DT_INST_CHILD(id, display_timings), \
							       vsync_len),                         \
					       .sync_polarity =                                    \
						       DT_PROP(DT_INST_CHILD(id, display_timings), \
							       vsync_active)},                     \
				   .format = (OUTPUT_FORMAT_PIXEL == PANEL_PIXEL_FORMAT_RGB_565)   \
						     ? DISPLAY_OUT_FORMAT_16BITS_RGB565            \
						     : DISPLAY_OUT_FORMAT_24BITS_RGB888,           \
				   .endian = DISPLAY_ENDIAN_LITTLE,                                \
				   .color_order = DISPLAY_COLOR_ORDER_RGB,                         \
				   .data_enable_polarity = DISPLAY_SIGNAL_POLARITY_HIACTIVE,       \
				   .sync_edge = DISPLAY_SIGNAL_SYNC_EDGE_FALLING,                  \
				   .bg_color = {.byte = {.a = OUTPUT_ALPHA,                        \
							 .r = OUTPUT_RED,                          \
							 .g = OUTPUT_GREEN,                        \
							 .b = OUTPUT_BLUE}},                       \
				   .brightness = {.enable = false,                                 \
						  .r = OUTPUT_RED,                                 \
						  .g = OUTPUT_GREEN,                               \
						  .b = OUTPUT_BLUE},                               \
				   .contrast = {.enable = false,                                   \
						.r = OUTPUT_RED,                                   \
						.g = OUTPUT_GREEN,                                 \
						.b = OUTPUT_BLUE},                                 \
				   .dithering_on = false},                                         \
			.p_callback = NULL,                                                        \
			.p_context = NULL,                                                         \
			.p_extend = (void *)(&display_extend_cfg##id),                             \
			.line_detect_ipl = BSP_IRQ_DISABLED,                                       \
			.underflow_1_ipl = BSP_IRQ_DISABLED,                                       \
			.underflow_2_ipl = BSP_IRQ_DISABLED}};                                     \
	static struct display_ra_config ra_config##id = {                                          \
		IRQ_CONFIGURE_DEFINE(id),                                                          \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(id),                                      \
		.backlight_gpio = GPIO_DT_SPEC_INST_GET(id, backlight_gpios),                      \
		.height = DT_INST_PROP(id, height),                                                \
		.width = DT_INST_PROP(id, width),                                                  \
		.pixel_format = DT_INST_PROP(id, input_pixel_format),                              \
		.display_frame_size =                                                              \
			(DT_INST_PROP(id, width)) * (DT_INST_PROP(id, height)) * BYTE_PER_PIXEL,   \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(id)),                               \
		.clock_glcdc_subsys = {.mstp = (uint32_t)DT_INST_CLOCKS_CELL_BY_IDX(id, 0, mstp),  \
				       .stop_bit = DT_INST_CLOCKS_CELL_BY_IDX(id, 0, stop_bit)},   \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(id, &display_init, NULL, &ra_data##id, &ra_config##id, POST_KERNEL,  \
			      CONFIG_DISPLAY_INIT_PRIORITY, &display_api);

DT_INST_FOREACH_STATUS_OKAY(RENESAS_RA_DEVICE_INIT)
