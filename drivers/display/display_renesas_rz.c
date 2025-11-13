/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rz_lcdc

#include "display_renesas_rz.h"
#include "r_lcdc.h"
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/cache.h>

LOG_MODULE_REGISTER(display_renesas_rz, CONFIG_DISPLAY_LOG_LEVEL);

struct display_rz_config {
	const display_api_t *fsp_api;
	const struct pinctrl_dev_config *pincfg;
	const struct gpio_dt_spec backlight_gpio;
	uint16_t height;
	uint16_t width;
	void (*irq_configure)(void);
};

struct display_rz_data {
	lcdc_instance_ctrl_t display_ctrl;
	display_cfg_t display_fsp_cfg;
	const uint8_t *pend_buf;
	const uint8_t *front_buf;
	uint8_t pixel_size;
	enum display_pixel_format current_pixel_format;
	uint8_t *frame_buffer;
	uint32_t frame_buffer_len;
	struct k_sem frame_buf_sem;
};

extern void lcdc_vspd_int(void *irq);

static void renesas_rz_lcdc_isr(const struct device *dev)
{
	struct display_rz_data *data = dev->data;
	const lcdc_extended_cfg_t *ext_cfg = data->display_fsp_cfg.p_extend;

	lcdc_vspd_int((void *)ext_cfg->frame_end_irq);
}

static void renesas_rz_callback_adapter(display_callback_args_t *p_args)
{
	const struct device *dev = p_args->p_context;
	struct display_rz_data *data = dev->data;

	if (p_args->event == DISPLAY_EVENT_FRAME_END) {
		if (data->front_buf != data->pend_buf) {
			data->front_buf = data->pend_buf;
		}

		k_sem_give(&data->frame_buf_sem);
	}
}

static int rz_display_write(const struct device *dev, const uint16_t x, const uint16_t y,
			    const struct display_buffer_descriptor *desc, const void *buf)
{
	struct display_rz_data *data = dev->data;
	const struct display_rz_config *config = dev->config;
	const uint8_t *l_pend_buf = NULL;
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
#if CONFIG_RENESAS_RZ_LCDC_FB_NUM == 0
		LOG_ERR("Partial write requires internal frame buffer");
		return -ENOTSUP;
#else
		const uint8_t *src = buf;
		uint8_t *dst = NULL;
		uint16_t row;

		dst = data->frame_buffer;

		if (CONFIG_RENESAS_RZ_LCDC_FB_NUM == 2) {
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
			sys_cache_data_flush_range(dst, desc->width * data->pixel_size);
			dst += (config->width * data->pixel_size);
			src += (desc->pitch * data->pixel_size);
		}
#endif /* CONFIG_RENESAS_RZ_LCDC_FB_NUM == 0 */
	}

	if (data->front_buf != l_pend_buf) {
		k_sem_reset(&data->frame_buf_sem);
		data->pend_buf = l_pend_buf;
		sys_cache_data_flush_range((void *)data->pend_buf, data->frame_buffer_len);
		err = config->fsp_api->bufferChange(&data->display_ctrl, (uint8_t *)data->pend_buf,
						    DISPLAY_FRAME_LAYER_1);
		if (err != FSP_SUCCESS) {
			LOG_ERR("LCDC buffer change failed");
			return -EIO;
		}
		k_sem_take(&data->frame_buf_sem, K_FOREVER);
	}

	return 0;
}

static int rz_display_read(const struct device *dev, const uint16_t x, const uint16_t y,
			   const struct display_buffer_descriptor *desc, void *buf)
{
	struct display_rz_data *data = dev->data;
	const struct display_rz_config *config = dev->config;
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

static int rz_display_blanking_on(const struct device *dev)
{
	const struct display_rz_config *config = dev->config;
	int ret = 0;

	if (config->backlight_gpio.port != NULL) {
		ret = gpio_pin_set_dt(&config->backlight_gpio, 0);
	} else {
		ret = -ENOTSUP;
	}

	return ret;
}

static int rz_display_blanking_off(const struct device *dev)
{
	const struct display_rz_config *config = dev->config;
	int ret = 0;

	if (config->backlight_gpio.port != NULL) {
		ret = gpio_pin_set_dt(&config->backlight_gpio, 1);
	} else {
		ret = -ENOTSUP;
	}

	return ret;
}

static void rz_display_get_capabilities(const struct device *dev,
					struct display_capabilities *capabilities)
{
	const struct display_rz_config *config = dev->config;
	struct display_rz_data *data = dev->data;

	capabilities->x_resolution = config->width;
	capabilities->y_resolution = config->height;
	capabilities->current_orientation = DISPLAY_ORIENTATION_NORMAL;
	capabilities->supported_pixel_formats =
		PIXEL_FORMAT_RGB_888 | PIXEL_FORMAT_ARGB_8888 | PIXEL_FORMAT_RGB_565;
	capabilities->current_pixel_format = data->current_pixel_format;
	capabilities->screen_info = 0U;
}

static int rz_display_set_pixel_format(const struct device *dev,
				       const enum display_pixel_format pixel_format)
{
	const struct display_rz_config *config = dev->config;
	struct display_rz_data *data = dev->data;
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

	err = config->fsp_api->layerChange(&data->display_ctrl, &layer_cfg, DISPLAY_FRAME_LAYER_1);
	if (err != FSP_SUCCESS) {
		LOG_ERR("Failed to change the pixel format");
		return -EIO;
	}

	data->current_pixel_format = pixel_format;
	data->pixel_size = DISPLAY_BITS_PER_PIXEL(set_pixel_format) >> 3;

	return 0;
}

static void *rz_display_get_framebuffer(const struct device *dev)
{
	struct display_rz_data *data = dev->data;

	return (void *)data->front_buf;
}

static DEVICE_API(display, display_api) = {
	.blanking_on = rz_display_blanking_on,
	.blanking_off = rz_display_blanking_off,
	.get_capabilities = rz_display_get_capabilities,
	.set_pixel_format = rz_display_set_pixel_format,
	.write = rz_display_write,
	.read = rz_display_read,
	.get_framebuffer = rz_display_get_framebuffer,
};

static int display_init(const struct device *dev)
{
	const struct display_rz_config *config = dev->config;
	struct display_rz_data *data = dev->data;
	int err;

	if (config->pincfg != NULL) {
		err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
		if (err) {
			LOG_ERR("Pin function initial failed");
			return err;
		}
	}

	k_sem_init(&data->frame_buf_sem, 0, 1);

	err = config->fsp_api->open(&data->display_ctrl, &data->display_fsp_cfg);
	if (err) {
		LOG_ERR("LCDC open failed");
		return -EIO;
	}

	err = gpio_pin_configure_dt(&config->backlight_gpio, GPIO_OUTPUT_ACTIVE);
	if (err) {
		LOG_ERR("Backlight GPIO configuration failed");
		return err;
	}

	config->irq_configure();

	err = config->fsp_api->start(&data->display_ctrl);
	if (err) {
		LOG_ERR("LCDC start failed");
		return -EIO;
	}

	return 0;
}

#define IRQ_CONFIGURE_FUNC(id)                                                                     \
	static void lcdc_renesas_rz_configure_func_##id(void)                                      \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(id, vspd_int, irq),                                \
			    DT_INST_IRQ_BY_NAME(id, vspd_int, priority), renesas_rz_lcdc_isr,      \
			    DEVICE_DT_INST_GET(id), DT_INST_IRQ_BY_NAME(id, vspd_int, flags));     \
		irq_enable(DT_INST_IRQ_BY_NAME(id, vspd_int, irq));                                \
	}

#define IRQ_CONFIGURE_DEFINE(id) .irq_configure = lcdc_renesas_rz_configure_func_##id

#define RENESAS_RZ_FRAME_BUFFER_LEN(id)                                                            \
	(RENESAS_RZ_LCDC_PIXEL_BYTE_SIZE(id) * DT_INST_PROP(id, height) * DT_INST_PROP(id, width))

#define RENESAS_RZ_LCDC_DEVICE_PINCTRL_INIT(n)                                                     \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, pinctrl_0), (PINCTRL_DT_INST_DEFINE(n)), ())

#define RENESAS_RZ_LCDC_DEVICE_PINCTRL_GET(n)                                                      \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, pinctrl_0), (PINCTRL_DT_INST_DEV_CONFIG_GET(n)),      \
		    (NULL))

#define RENESAS_RZ_DEVICE_INIT(id)                                                                 \
	RENESAS_RZ_LCDC_DEVICE_PINCTRL_INIT(id);                                                   \
                                                                                                   \
	IRQ_CONFIGURE_FUNC(id)                                                                     \
                                                                                                   \
	static uint8_t __aligned(64)                                                               \
	fb_background##id[CONFIG_RENESAS_RZ_LCDC_FB_NUM * RENESAS_RZ_FRAME_BUFFER_LEN(id)];        \
                                                                                                   \
	static const lcdc_extended_cfg_t display_extend_cfg##id = {                                \
		.frame_end_ipl = DT_INST_IRQ_BY_NAME(id, vspd_int, priority),                      \
		.underrun_ipl = DT_INST_IRQ_BY_NAME(id, vspd_int, priority),                       \
		.frame_end_irq = DT_INST_IRQ_BY_NAME(id, vspd_int, irq),                           \
		.underrun_irq = DT_INST_IRQ_BY_NAME(id, vspd_int, irq)};                           \
                                                                                                   \
	static struct display_rz_data rz_data##id = {                                              \
		.frame_buffer = fb_background##id,                                                 \
		.frame_buffer_len = RENESAS_RZ_FRAME_BUFFER_LEN(id),                               \
		.front_buf = fb_background##id,                                                    \
		.pend_buf = fb_background##id,                                                     \
		.pixel_size = RENESAS_RZ_LCDC_PIXEL_BYTE_SIZE(id),                                 \
		.current_pixel_format = RENESAS_RZ_DISPLAY_GET_PIXEL_FORMAT(id),                   \
		.display_fsp_cfg = {                                                               \
			.input[0] =                                                                \
				{                                                                  \
					.p_base = (uint32_t *)fb_background##id,                   \
					.hsize = DISPLAY_HSIZE(id),                                \
					.vsize = DISPLAY_VSIZE(id),                                \
					.coordinate_x = 0,                                         \
					.coordinate_y = 0,                                         \
					.hstride = RENESAS_RZ_DISPLAY_BUFFER_HSTRIDE_BYTE(id),     \
					.format = RENESAS_RZ_LCDC_IN_PIXEL_FORMAT(id),             \
					.data_swap = DISPLAY_DATA_SWAP_64BIT |                     \
						     DISPLAY_DATA_SWAP_32BIT |                     \
						     DISPLAY_DATA_SWAP_16BIT,                      \
				},                                                                 \
			.input[1] = {.p_base = NULL},                                              \
			.output = {.htiming = RENESAS_RZ_LCDC_HTIMING(id),                         \
				   .vtiming = RENESAS_RZ_LCDC_VTIMING(id),                         \
				   .format = RENESAS_RZ_LCDC_OUT_PIXEL_FORMAT(id),                 \
				   .data_enable_polarity = RENESAS_RZ_LCDC_OUTPUT_DE_POLARITY(id), \
				   .sync_edge = RENESAS_RZ_LCDC_OUTPUT_SYNC_EDGE(id),              \
				   .bg_color = RENESAS_RZ_LCDC_BG_COLOR(id),                       \
				   .dithering_on = false},                                         \
			.p_callback = renesas_rz_callback_adapter,                                 \
			.p_context = DEVICE_DT_INST_GET(id),                                       \
			.p_extend = (void *)(&display_extend_cfg##id)}};                           \
                                                                                                   \
	static struct display_rz_config rz_config##id = {                                          \
		.fsp_api = &g_display_on_lcdc,                                                     \
		IRQ_CONFIGURE_DEFINE(id),                                                          \
		.pincfg = RENESAS_RZ_LCDC_DEVICE_PINCTRL_GET(id),                                  \
		.backlight_gpio = GPIO_DT_SPEC_INST_GET_OR(id, backlight_gpios, NULL),             \
		.height = DT_INST_PROP(id, height),                                                \
		.width = DT_INST_PROP(id, width)};                                                 \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(id, &display_init, NULL, &rz_data##id, &rz_config##id, POST_KERNEL,  \
			      CONFIG_DISPLAY_INIT_PRIORITY, &display_api);

DT_INST_FOREACH_STATUS_OKAY(RENESAS_RZ_DEVICE_INIT)
