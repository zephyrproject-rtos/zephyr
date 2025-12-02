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

struct display_ra_config {
	const struct pinctrl_dev_config *pincfg;
	const struct gpio_dt_spec backlight_gpio;
	const struct device *clock_dev;
	struct clock_control_ra_subsys_cfg clock_glcdc_subsys;
	uint16_t height;
	uint16_t width;
	void (*irq_configure)(void);
};

struct display_ra_data {
	glcdc_instance_ctrl_t display_ctrl;
	display_cfg_t display_fsp_cfg;
	const uint8_t *pend_buf;
	const uint8_t *front_buf;
	uint8_t pixel_size;
	enum display_pixel_format current_pixel_format;
	uint8_t *frame_buffer;
	uint32_t frame_buffer_len;
	struct k_sem frame_buf_sem;
};

extern void glcdc_line_detect_isr(void);

static void renesas_ra_glcdc_isr(const struct device *dev)
{
	ARG_UNUSED(dev);
	glcdc_line_detect_isr();
}

static void renesas_ra_callback_adapter(display_callback_args_t *p_args)
{
	const struct device *dev = p_args->p_context;
	struct display_ra_data *data = dev->data;

	if (p_args->event == DISPLAY_EVENT_LINE_DETECTION) {
		if (data->front_buf != data->pend_buf) {
			data->front_buf = data->pend_buf;
		}

		k_sem_give(&data->frame_buf_sem);
	}
}

static int ra_display_write(const struct device *dev, const uint16_t x, const uint16_t y,
			    const struct display_buffer_descriptor *desc, const void *buf)
{
	struct display_ra_data *data = dev->data;
	const struct display_ra_config *config = dev->config;
	const uint8_t *l_pend_buf = NULL;
	bool vsync_wait = false;
	fsp_err_t err;

	if (desc->pitch < desc->width) {
		LOG_ERR("Pitch is smaller than width");
		return -EINVAL;
	}

	if ((desc->pitch * data->pixel_size * desc->height) > desc->buf_size) {
		LOG_ERR("Input buffer too small");
		return -EINVAL;
	}

	if (x == 0 && y == 0 && desc->height == config->height && desc->width == config->width) {
		l_pend_buf = buf;
	} else {
#if CONFIG_RENESAS_RA_GLCDC_FB_NUM == 0
		LOG_ERR("Partial write requires internal frame buffer");
		return -ENOTSUP;
#else
		const uint8_t *src = buf;
		uint8_t *dst = NULL;
		uint16_t row;

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
#endif /* CONFIG_RENESAS_RA_GLCDC_FB_NUM == 0 */
	}

	k_sem_reset(&data->frame_buf_sem);

	if (data->front_buf != l_pend_buf) {
		data->pend_buf = l_pend_buf;

		err = R_GLCDC_BufferChange(&data->display_ctrl, (uint8_t *)data->pend_buf,
					   DISPLAY_FRAME_LAYER_1);
		if (err != FSP_SUCCESS) {
			LOG_ERR("GLCDC buffer change failed");
			return -EIO;
		}

		vsync_wait = true;
	}

	if (data->display_ctrl.state != DISPLAY_STATE_DISPLAYING) {
		err = R_GLCDC_Start(&data->display_ctrl);
		if (err != FSP_SUCCESS) {
			LOG_ERR("GLCDC start failed");
			return -EIO;
		}

		vsync_wait = true;
	}

	if (vsync_wait) {
		k_sem_take(&data->frame_buf_sem, K_FOREVER);
	}

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
	int ret = 0;

	if (config->backlight_gpio.port != NULL) {
		ret = gpio_pin_set_dt(&config->backlight_gpio, 0);
	} else {
		ret = -ENOTSUP;
	}

	return ret;
}

static int ra_display_blanking_off(const struct device *dev)
{
	const struct display_ra_config *config = dev->config;
	int ret = 0;

	if (config->backlight_gpio.port != NULL) {
		ret = gpio_pin_set_dt(&config->backlight_gpio, 1);
	} else {
		ret = -ENOTSUP;
	}

	return ret;
}

static void ra_display_get_capabilities(const struct device *dev,
					struct display_capabilities *capabilities)
{
	const struct display_ra_config *config = dev->config;
	struct display_ra_data *data = dev->data;

	capabilities->x_resolution = config->width;
	capabilities->y_resolution = config->height;
	capabilities->current_orientation = DISPLAY_ORIENTATION_NORMAL;
	capabilities->supported_pixel_formats =
		PIXEL_FORMAT_RGB_888 | PIXEL_FORMAT_ARGB_8888 | PIXEL_FORMAT_RGB_565;
	capabilities->current_pixel_format = data->current_pixel_format;
	capabilities->screen_info = 0U;
}

static int ra_display_set_pixel_format(const struct device *dev,
				       const enum display_pixel_format pixel_format)
{
	const struct display_ra_config *config = dev->config;
	struct display_ra_data *data = dev->data;
	display_runtime_cfg_t layer_cfg;
	enum display_pixel_format set_pixel_format;
	display_in_format_t hardware_pixel_format;
	uint32_t buf_len;
	fsp_err_t err;

	if (pixel_format == data->current_pixel_format) {
		return 0;
	}

	if (data->display_ctrl.state == DISPLAY_STATE_DISPLAYING) {
		LOG_ERR("Cannot change the display format while displaying");
		return -EWOULDBLOCK;
	}

	switch (pixel_format) {
	case PIXEL_FORMAT_RGB_888:
		set_pixel_format = PIXEL_FORMAT_ARGB_8888;
		hardware_pixel_format = DISPLAY_IN_FORMAT_32BITS_RGB888;
		break;

	case PIXEL_FORMAT_ARGB_8888:
		set_pixel_format = PIXEL_FORMAT_ARGB_8888;
		hardware_pixel_format = DISPLAY_IN_FORMAT_32BITS_ARGB8888;
		break;

	case PIXEL_FORMAT_RGB_565:
		set_pixel_format = PIXEL_FORMAT_RGB_565;
		hardware_pixel_format = DISPLAY_IN_FORMAT_16BITS_RGB565;
		break;

	default:
		return -ENOTSUP;
	}

	buf_len = (config->height * config->width * DISPLAY_BITS_PER_PIXEL(set_pixel_format)) >> 3;

	if (buf_len > data->frame_buffer_len) {
		LOG_ERR("Frame buffer is smaller than new pixel format require");
		return -ENOTSUP;
	}

	memcpy(&layer_cfg.input, &data->display_fsp_cfg.input[0], sizeof(display_input_cfg_t));
	memcpy(&layer_cfg.layer, &data->display_fsp_cfg.layer[0], sizeof(display_layer_t));
	layer_cfg.input.format = hardware_pixel_format;
	layer_cfg.input.hstride =
		ROUND_UP(layer_cfg.input.hsize * DISPLAY_BITS_PER_PIXEL(set_pixel_format),
			 NUM_BITS(uint64_t)) /
		DISPLAY_BITS_PER_PIXEL(set_pixel_format);

	err = R_GLCDC_LayerChange(&data->display_ctrl, &layer_cfg, DISPLAY_FRAME_LAYER_1);
	if (err != FSP_SUCCESS) {
		LOG_ERR("Failed to change the pixel format");
		return -EIO;
	}

	data->current_pixel_format = pixel_format;
	data->pixel_size = DISPLAY_BITS_PER_PIXEL(set_pixel_format) >> 3;

	return 0;
}

static int ra_display_color_config(const struct device *dev,
				   const display_correction_t *display_color_cfg)
{
	struct display_ra_data *data = dev->data;
	fsp_err_t err;
	int ret;

	err = R_GLCDC_ColorCorrection(&data->display_ctrl, display_color_cfg);
	switch (err) {
	case FSP_SUCCESS:
		ret = 0;
		break;
	case FSP_ERR_INVALID_UPDATE_TIMING:
		ret = -EWOULDBLOCK;
		break;
	default:
		ret = -EIO;
		break;
	}

	return ret;
}

#define RENESAS_RA_GLCDC_BRIGHTNESS_MAX 1023U

static int ra_display_set_brightness(const struct device *dev, const uint8_t brightness)
{
	struct display_ra_data *data = dev->data;
	const uint32_t brightness_adj =
		DIV_ROUND_CLOSEST(brightness * RENESAS_RA_GLCDC_BRIGHTNESS_MAX, UINT8_MAX);
	display_correction_t display_color_cfg;

	if (brightness_adj == 0) {
		return -EINVAL;
	}

	memcpy(&display_color_cfg.contrast, &data->display_fsp_cfg.output.contrast,
	       sizeof(display_contrast_t));
	display_color_cfg.brightness = (display_brightness_t){
		.enable = true, .r = brightness_adj, .g = brightness_adj, .b = brightness_adj};

	return ra_display_color_config(dev, &display_color_cfg);
}

static int ra_display_set_contrast(const struct device *dev, const uint8_t contrast)
{
	struct display_ra_data *data = dev->data;
	display_correction_t display_color_cfg;

	if (contrast == 0) {
		return -EINVAL;
	}

	memcpy(&display_color_cfg.brightness, &data->display_fsp_cfg.output.brightness,
	       sizeof(display_brightness_t));
	display_color_cfg.contrast =
		(display_contrast_t){.enable = true, .r = contrast, .g = contrast, .b = contrast};

	return ra_display_color_config(dev, &display_color_cfg);
}

static void *ra_display_get_framebuffer(const struct device *dev)
{
	struct display_ra_data *data = dev->data;

	return (void *)data->front_buf;
}

static DEVICE_API(display, display_api) = {
	.blanking_on = ra_display_blanking_on,
	.blanking_off = ra_display_blanking_off,
	.get_capabilities = ra_display_get_capabilities,
	.set_pixel_format = ra_display_set_pixel_format,
	.write = ra_display_write,
	.read = ra_display_read,
	.set_brightness = ra_display_set_brightness,
	.set_contrast = ra_display_set_contrast,
	.get_framebuffer = ra_display_get_framebuffer,
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

	if (config->pincfg != NULL) {
		err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
		if (err) {
			LOG_ERR("pin function initial failed");
			return err;
		}
	}

	k_sem_init(&data->frame_buf_sem, 0, 1);

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

	config->irq_configure();

	return 0;
}

#define IRQ_CONFIGURE_FUNC(id)                                                                     \
	static void glcdc_renesas_ra_configure_func_##id(void)                                     \
	{                                                                                          \
		R_ICU->IELSR[DT_INST_IRQ_BY_NAME(id, line, irq)] =                                 \
			BSP_PRV_IELS_ENUM(EVENT_GLCDC_LINE_DETECT);                                \
		BSP_ASSIGN_EVENT_TO_CURRENT_CORE(BSP_PRV_IELS_ENUM(EVENT_GLCDC_LINE_DETECT));      \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(id, line, irq),                                    \
			    DT_INST_IRQ_BY_NAME(id, line, priority), renesas_ra_glcdc_isr,         \
			    DEVICE_DT_INST_GET(id), 0);                                            \
		irq_enable(DT_INST_IRQ_BY_NAME(id, line, irq));                                    \
	}

#define IRQ_CONFIGURE_DEFINE(id) .irq_configure = glcdc_renesas_ra_configure_func_##id

#define RENESAS_RA_FRAME_BUFFER_LEN(id)                                                            \
	(RENESAS_RA_GLCDC_PIXEL_BYTE_SIZE(id) * DT_INST_PROP(id, height) * DT_INST_PROP(id, width))

#ifdef CONFIG_RENESAS_RA_GLCDC_FRAME_BUFFER_SECTION
#define FRAME_BUFFER_SECTION Z_GENERIC_SECTION(CONFIG_RENESAS_RA_GLCDC_FRAME_BUFFER_SECTION)
#else
#define FRAME_BUFFER_SECTION
#endif /* CONFIG_RENESAS_RA_GLCDC_FRAME_BUFFER_SECTION */

#define RENESAS_RA_GLCDC_DEVICE_PINCTRL_INIT(n)                                                    \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, pinctrl_0), (PINCTRL_DT_INST_DEFINE(n)), ())

#define RENESAS_RA_GLCDC_DEVICE_PINCTRL_GET(n)                                                     \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, pinctrl_0), (PINCTRL_DT_INST_DEV_CONFIG_GET(n)),      \
		    (NULL))

#define RENESAS_RA_DEVICE_INIT(id)                                                                 \
	RENESAS_RA_GLCDC_DEVICE_PINCTRL_INIT(id);                                                  \
	IRQ_CONFIGURE_FUNC(id)                                                                     \
	FRAME_BUFFER_SECTION static uint8_t __aligned(64)                                          \
	fb_background##id[CONFIG_RENESAS_RA_GLCDC_FB_NUM * RENESAS_RA_FRAME_BUFFER_LEN(id)];       \
	static const glcdc_extended_cfg_t display_extend_cfg##id = {                               \
		.tcon_hsync = RENESAS_RA_GLCDC_TCON_HSYNC_PIN(id),                                 \
		.tcon_vsync = RENESAS_RA_GLCDC_TCON_VSYNC_PIN(id),                                 \
		.tcon_de = RENESAS_RA_GLCDC_TCON_DE_PIN(id),                                       \
		.correction_proc_order = GLCDC_CORRECTION_PROC_ORDER_BRIGHTNESS_CONTRAST2GAMMA,    \
		.clksrc = GLCDC_CLK_SRC_INTERNAL,                                                  \
		.clock_div_ratio = RENESAS_RA_GLCDC_OUTPUT_CLOCK_DIV(id),                          \
		.phy_layer = NULL};                                                                \
	static struct display_ra_data ra_data##id = {                                              \
		.frame_buffer = fb_background##id,                                                 \
		.frame_buffer_len = RENESAS_RA_FRAME_BUFFER_LEN(id),                               \
		.front_buf = fb_background##id,                                                    \
		.pend_buf = fb_background##id,                                                     \
		.pixel_size = RENESAS_RA_GLCDC_PIXEL_BYTE_SIZE(id),                                \
		.current_pixel_format = RENESAS_RA_DISPLAY_GET_PIXEL_FORMAT(id),                   \
		.display_fsp_cfg = {                                                               \
			.input[0] = {.p_base = (uint32_t *)fb_background##id,                      \
				     .hsize = DISPLAY_HSIZE(id),                                   \
				     .vsize = DISPLAY_VSIZE(id),                                   \
				     .hstride = RENESAS_RA_DISPLAY_BUFFER_HSTRIDE_BYTE(id),        \
				     .format = RENESAS_RA_GLCDC_IN_PIXEL_FORMAT(id),               \
				     .line_descending_enable = false,                              \
				     .lines_repeat_enable = false,                                 \
				     .lines_repeat_times = 0},                                     \
			.layer[0] = {.coordinate = {.x = 0, .y = 0},                               \
				     .bg_color = RENESAS_RA_GLCDC_BG_COLOR(id),                    \
				     .fade_control = DISPLAY_FADE_CONTROL_NONE,                    \
				     .fade_speed = 0},                                             \
			.input[1] = {.p_base = NULL},                                              \
			.output = {.htiming = RENESAS_RA_GLCDC_HTIMING(id),                        \
				   .vtiming = RENESAS_RA_GLCDC_VTIMING(id),                        \
				   .format = RENESAS_RA_GLCDC_OUT_PIXEL_FORMAT(id),                \
				   .endian = RENESAS_RA_GLCDC_OUTPUT_ENDIAN(id),                   \
				   .color_order = RENESAS_RA_GLCDC_OUTPUT_COLOR_ODER(id),          \
				   .data_enable_polarity =                                         \
					   RENESAS_RA_GLCDC_OUTPUT_DE_POLARITY(id),                \
				   .sync_edge = RENESAS_RA_GLCDC_OUTPUT_SYNC_EDGE(id),             \
				   .bg_color = RENESAS_RA_GLCDC_BG_COLOR(id),                      \
				   .brightness = {.enable = false},                                \
				   .contrast = {.enable = false},                                  \
				   .dithering_on = false},                                         \
			.p_callback = renesas_ra_callback_adapter,                                 \
			.p_context = (void *)DEVICE_DT_INST_GET(id),                               \
			.p_extend = (void *)(&display_extend_cfg##id),                             \
			.line_detect_irq = DT_INST_IRQ_BY_NAME(id, line, irq),                     \
			.line_detect_ipl = DT_INST_IRQ_BY_NAME(id, line, priority),                \
			.underflow_1_irq = BSP_IRQ_DISABLED,                                       \
			.underflow_2_irq = BSP_IRQ_DISABLED}};                                     \
	static struct display_ra_config ra_config##id = {                                          \
		IRQ_CONFIGURE_DEFINE(id),                                                          \
		.pincfg = RENESAS_RA_GLCDC_DEVICE_PINCTRL_GET(id),                                 \
		.backlight_gpio = GPIO_DT_SPEC_INST_GET_OR(id, backlight_gpios, NULL),             \
		.height = DT_INST_PROP(id, height),                                                \
		.width = DT_INST_PROP(id, width),                                                  \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(id)),                               \
		.clock_glcdc_subsys = {.mstp = (uint32_t)DT_INST_CLOCKS_CELL_BY_IDX(id, 0, mstp),  \
				       .stop_bit = DT_INST_CLOCKS_CELL_BY_IDX(id, 0, stop_bit)}};  \
	DEVICE_DT_INST_DEFINE(id, &display_init, NULL, &ra_data##id, &ra_config##id, POST_KERNEL,  \
			      CONFIG_DISPLAY_INIT_PRIORITY, &display_api);

DT_INST_FOREACH_STATUS_OKAY(RENESAS_RA_DEVICE_INIT)
