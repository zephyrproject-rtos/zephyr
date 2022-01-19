/*
 * Copyright (c) 2022 Byte-Lab d.o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_ltdc

#include <string.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/display.h>
#include <sys/byteorder.h>
#include <drivers/gpio.h>
#include <drivers/pinctrl.h>
#include <drivers/clock_control/stm32_clock_control.h>
#include <drivers/clock_control.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(display_stm32_ltdc, CONFIG_DISPLAY_LOG_LEVEL);

#if defined(CONFIG_STM32_LTDC_ARGB8888)
#define STM32_LTDC_INIT_PIXEL_SIZE 		4u
#define STM32_LTDC_INIT_PIXEL_FORMAT	LTDC_PIXEL_FORMAT_ARGB8888
#define DISPLAY_INIT_PIXEL_FORMAT		PIXEL_FORMAT_ARGB_8888
#elif defined(CONFIG_STM32_LTDC_RGB888)
#define STM32_LTDC_INIT_PIXEL_SIZE 		3u
#define STM32_LTDC_INIT_PIXEL_FORMAT	LTDC_PIXEL_FORMAT_RGB888
#define DISPLAY_INIT_PIXEL_FORMAT		PIXEL_FORMAT_RGB_888
#elif defined(CONFIG_STM32_LTDC_RGB565)
#define STM32_LTDC_INIT_PIXEL_SIZE 		2u
#define STM32_LTDC_INIT_PIXEL_FORMAT	LTDC_PIXEL_FORMAT_RGB565
#define DISPLAY_INIT_PIXEL_FORMAT		PIXEL_FORMAT_RGB_565
#else
#error "Invalid LTDC pixel format chosen"
#endif /* CONFIG_STM32_LTDC_PIXEL_FORMAT */


#ifdef CONFIG_PLACE_FRAME_BUFFER_IN_EXTERNAL_SDRAM

#if DT_NODE_HAS_STATUS(DT_NODELABEL(sdram1), okay)
#define FRAME_BUFFER_SECTION __stm32_sdram1_section
#elif DT_NODE_HAS_STATUS(DT_NODELABEL(sdram2), okay)
#define FRAME_BUFFER_SECTION __stm32_sdram2_section
#else
#error "SDRAM node does not exist in device tree"
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(sdramX), okay) */

#else
#define FRAME_BUFFER_SECTION
#endif /* CONFIG_PLACE_FRAME_BUFFER_IN_EXTERNAL_SDRAM */


#if CONFIG_HAS_CMSIS_CORE_M
#include <arch/arm/aarch32/cortex_m/cmsis.h>

#if __DCACHE_PRESENT == 1
#define CACHE_INVALIDATE(addr, size) SCB_InvalidateDCache_by_Addr((addr), (size))
#define CACHE_CLEAN(addr, size) SCB_CleanDCache_by_Addr((addr), (size))
#else
#define CACHE_INVALIDATE(addr, size)
#define CACHE_CLEAN(addr, size) __DSB()
#endif /* __DCACHE_PRESENT == 1 */

#else /* CONFIG_HAS_CMSIS_CORE_M */
#define CACHE_INVALIDATE(addr, size)
#define CACHE_CLEAN(addr, size)
#endif /* CONFIG_HAS_CMSIS_CORE_M */

struct display_stm32_ltdc_gpio_data {
	const char *const name;
	gpio_pin_t pin;
	gpio_dt_flags_t flags;
};

struct display_stm32_ltdc_data {
	LTDC_HandleTypeDef hltdc;
	enum display_pixel_format current_pixel_format;
	uint8_t current_pixel_size;
	const struct device *disp_on_gpio_dev;
	const struct device *bl_ctrl_gpio_dev;
};

// frame buffer aligned to cache line width for optimal cache flushing
FRAME_BUFFER_SECTION static uint8_t __aligned(32) frame_buffer[STM32_LTDC_INIT_PIXEL_SIZE *
				DT_INST_PROP(0, height) *
				DT_INST_PROP(0, width)];


struct display_stm32_ltdc_config {
	uint32_t width;
	uint32_t height;
	struct display_stm32_ltdc_gpio_data disp_on_gpio;
	struct display_stm32_ltdc_gpio_data bl_ctrl_gpio;
	struct stm32_pclken pclken;
	const struct pinctrl_dev_config *pctrl;
};


static int stm32_ltdc_blanking_on(const struct device *dev)
{
	return -ENOTSUP;
}

static int stm32_ltdc_blanking_off(const struct device *dev)
{
	return -ENOTSUP;
}

static void *stm32_ltdc_get_framebuffer(const struct device *dev)
{
	return (void*) frame_buffer;
}

static int stm32_ltdc_set_brightness(const struct device *dev,
				   const uint8_t brightness)
{
	return -ENOTSUP;
}

static int stm32_ltdc_set_contrast(const struct device *dev,
				 const uint8_t contrast)
{
	return -ENOTSUP;
}

static int stm32_ltdc_set_pixel_format(const struct device *dev,
				     const enum display_pixel_format format)
{
	int err;
	struct display_stm32_ltdc_data *data =
			(struct display_stm32_ltdc_data *) dev->data;

	switch (format) {
	case PIXEL_FORMAT_RGB_565:
		err = HAL_LTDC_SetPixelFormat(&data->hltdc, LTDC_PIXEL_FORMAT_RGB565, 0);
		data->current_pixel_format = PIXEL_FORMAT_RGB_565;
		data->current_pixel_size = 2u;
		break;
	case PIXEL_FORMAT_RGB_888:
		err = HAL_LTDC_SetPixelFormat(&data->hltdc, LTDC_PIXEL_FORMAT_RGB888, 0);
		data->current_pixel_format = PIXEL_FORMAT_RGB_888;
		data->current_pixel_size = 3u;
		break;
	case PIXEL_FORMAT_ARGB_8888:
		err = HAL_LTDC_SetPixelFormat(&data->hltdc, LTDC_PIXEL_FORMAT_ARGB8888, 0);
		data->current_pixel_format = PIXEL_FORMAT_ARGB_8888;
		data->current_pixel_size = 4u;
	default:
		err = -ENOTSUP;
		break;
	}

	return err;
}

static int stm32_ltdc_set_orientation(const struct device *dev,
				    const enum display_orientation orientation)
{
	int err;

	switch (orientation) {
	case DISPLAY_ORIENTATION_NORMAL:
		err = 0;
	default:
		err = -ENOTSUP;
	}

	return err;
}

static void stm32_ltdc_get_capabilities(const struct device *dev,
				      struct display_capabilities *capabilities)
{
	struct display_stm32_ltdc_config *config =
			(struct display_stm32_ltdc_config *) dev->config;
	struct display_stm32_ltdc_data *data =
			(struct display_stm32_ltdc_data *) dev->data;

	memset(capabilities, 0, sizeof(struct display_capabilities));

	capabilities->x_resolution = config->width;
	capabilities->y_resolution = config->height;
	capabilities->supported_pixel_formats = PIXEL_FORMAT_ARGB_8888 |
		PIXEL_FORMAT_RGB_888 |
		PIXEL_FORMAT_RGB_565;
	capabilities->screen_info = 0;

	capabilities->current_pixel_format = data->current_pixel_format;
	capabilities->current_orientation = DISPLAY_ORIENTATION_NORMAL;
}

static int stm32_ltdc_write(const struct device *dev, const uint16_t x,
			  const uint16_t y,
			  const struct display_buffer_descriptor *desc,
			  const void *buf)
{
	struct display_stm32_ltdc_config *config =
			(struct display_stm32_ltdc_config *) dev->config;
	struct display_stm32_ltdc_data *data =
			(struct display_stm32_ltdc_data *) dev->data;
	uint8_t *dst = frame_buffer;
	const uint8_t *src = buf;
	uint16_t row;

	// dst = pointer to upper left pixel of the rectangle to be updated in frame buffer
	dst += (x * data->current_pixel_size);
	dst += (y * config->width * data->current_pixel_size);

	for (row = 0; row < desc->height; row++) {
		(void) memcpy(dst, src, desc->width * data->current_pixel_size);
		CACHE_CLEAN(dst, desc->width * data->current_pixel_size);
		dst += (config->width * data->current_pixel_size);
		src += (desc->pitch * data->current_pixel_size);
	}

	return 0;
}

static int stm32_ltdc_read(const struct device *dev, const uint16_t x,
			 const uint16_t y,
			 const struct display_buffer_descriptor *desc,
			 void *buf)
{
	struct display_stm32_ltdc_config *config =
			(struct display_stm32_ltdc_config *) dev->config;
	struct display_stm32_ltdc_data *data =
			(struct display_stm32_ltdc_data *) dev->data;
	uint8_t *dst = buf;
	const uint8_t *src = frame_buffer;
	uint16_t row;

	// src = pointer to upper left pixel of the rectangle to be read from frame buffer
	src += (x * data->current_pixel_size);
	src += (y * config->width * data->current_pixel_size);

	for (row = 0; row < desc->height; row++) {
		(void) memcpy(dst, src, desc->width * data->current_pixel_size);
		CACHE_CLEAN(dst, desc->width * data->current_pixel_size);
		src += (config->width * data->current_pixel_size);
		dst += (desc->pitch * data->current_pixel_size);
	}

	return 0;
}

static int stm32_ltdc_init(const struct device *dev)
{
	int err;
	struct display_stm32_ltdc_data *data =
			(struct display_stm32_ltdc_data *) dev->data;
	struct display_stm32_ltdc_config *config =
			(struct display_stm32_ltdc_config *) dev->config;
	const uint32_t frame_buffer_size = config->width *
			config->height *
			STM32_LTDC_INIT_PIXEL_SIZE;

#if DT_INST_NODE_HAS_PROP(0, disp_on_gpios)
	data->disp_on_gpio_dev = device_get_binding(config->disp_on_gpio.name);
	if (data->disp_on_gpio_dev == NULL) {
		LOG_ERR("Could not get GPIO device for LCD power on/off control");
		return -EPERM;
	}

	if (gpio_pin_configure(data->disp_on_gpio_dev,
						   config->disp_on_gpio.pin,
						   config->disp_on_gpio.flags | GPIO_OUTPUT_HIGH)) {
		LOG_ERR("Couldn't configure LCD power on/off control pin");
		return -EIO;
	}
#endif


#if DT_INST_NODE_HAS_PROP(0, bl_ctrl_gpios)
	data->bl_ctrl_gpio_dev = device_get_binding(config->bl_ctrl_gpio.name);
	if (data->bl_ctrl_gpio_dev == NULL) {
		LOG_ERR("Could not get GPIO device for LCD backlight on/off control");
		return -EPERM;
	}

	if (gpio_pin_configure(data->bl_ctrl_gpio_dev,
						   config->bl_ctrl_gpio.pin,
						   config->bl_ctrl_gpio.flags | GPIO_OUTPUT_HIGH)) {
		LOG_ERR("Couldn't configure LCD backlight on/off control pin");
		return -EIO;
	}
#endif

	/* Configure dt provided device signals when available */
	err = pinctrl_apply_state(config->pctrl, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		LOG_ERR("LTDC pinctrl setup failed (%d)", err);
		return err;
	}

	if (clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
			       (clock_control_subsys_t) &config->pclken) != 0) {
		LOG_ERR("Could not enable LTDC peripheral clock");
		return -EIO;
	}

	data->current_pixel_format = DISPLAY_INIT_PIXEL_FORMAT;
	data->current_pixel_size = STM32_LTDC_INIT_PIXEL_SIZE;
	(void) memset(frame_buffer, 0xFFu, frame_buffer_size);
	CACHE_CLEAN(frame_buffer, frame_buffer_size);

	// reset LTDC peripheral
	__HAL_RCC_LTDC_FORCE_RESET();
	__HAL_RCC_LTDC_RELEASE_RESET();

	err = HAL_LTDC_Init(&data->hltdc);
	if (err != HAL_OK) {
		return err;
	}

	err = HAL_LTDC_ConfigLayer(&data->hltdc, &data->hltdc.LayerCfg[0], 0);
	if (err != HAL_OK) {
		return err;
	}

	return 0;
}

PINCTRL_DT_INST_DEFINE(0);

static const struct display_driver_api stm32_ltdc_display_api = {
	.blanking_on = stm32_ltdc_blanking_on,
	.blanking_off = stm32_ltdc_blanking_off,
	.write = stm32_ltdc_write,
	.read = stm32_ltdc_read,
	.get_framebuffer = stm32_ltdc_get_framebuffer,
	.set_brightness = stm32_ltdc_set_brightness,
	.set_contrast = stm32_ltdc_set_contrast,
	.get_capabilities = stm32_ltdc_get_capabilities,
	.set_pixel_format = stm32_ltdc_set_pixel_format,
	.set_orientation = stm32_ltdc_set_orientation
};

static struct display_stm32_ltdc_data ltdc_data = {
	.hltdc = {
		.Instance = (LTDC_TypeDef *) DT_REG_ADDR(DT_NODELABEL(ltdc)),
		.Init = {
			.HSPolarity = DT_INST_PROP(0, hsync_pol),
			.VSPolarity = DT_INST_PROP(0, vsync_pol),
			.DEPolarity = DT_INST_PROP(0, de_pol),
			.PCPolarity = DT_INST_PROP(0, pclk_pol),
			.HorizontalSync = DT_INST_PROP(0, hsync_duration) - 1,
			.VerticalSync = DT_INST_PROP(0, vsync_duration) - 1,
			.AccumulatedHBP = DT_INST_PROP(0, hbp_duration) +
							  DT_INST_PROP(0, hsync_duration) - 1,
			.AccumulatedVBP = DT_INST_PROP(0, vbp_duration) +
							  DT_INST_PROP(0, vsync_duration) - 1,
			.AccumulatedActiveW = DT_INST_PROP(0, hbp_duration) +
								  DT_INST_PROP(0, hsync_duration) +
								  DT_INST_PROP(0, width) - 1,
			.AccumulatedActiveH = DT_INST_PROP(0, vbp_duration) +
								  DT_INST_PROP(0, vsync_duration) +
								  DT_INST_PROP(0, height) - 1,
			.TotalWidth = DT_INST_PROP(0, hbp_duration) +
						  DT_INST_PROP(0, hsync_duration) +
						  DT_INST_PROP(0, width) +
						  DT_INST_PROP(0, hfp_duration) - 1,
			.TotalHeigh = DT_INST_PROP(0, vbp_duration) +
						  DT_INST_PROP(0, vsync_duration) +
						  DT_INST_PROP(0, height) +
						  DT_INST_PROP(0, vfp_duration) - 1,
			.Backcolor.Red = DT_INST_PROP_OR(0, def_back_color_red, 0xFF),
			.Backcolor.Green = DT_INST_PROP_OR(0, def_back_color_green, 0xFF),
			.Backcolor.Blue = DT_INST_PROP_OR(0, def_back_color_blue, 0xFF),
		},
		// only one layer is used
		.LayerCfg[0] = {
			.WindowX0 = 0,
			.WindowX1 = DT_INST_PROP(0, width),
			.WindowY0 = 0,
			.WindowY1 = DT_INST_PROP(0, height),
			.PixelFormat = STM32_LTDC_INIT_PIXEL_FORMAT,
			.Alpha = 255,
			.Alpha0 = 0,
			.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA,
			.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA,
			.FBStartAdress = (uint32_t) frame_buffer,
			.ImageWidth = DT_INST_PROP(0, width),
			.ImageHeight = DT_INST_PROP(0, height),
			.Backcolor.Red = DT_INST_PROP_OR(0, def_back_color_red, 0xFF),
			.Backcolor.Green = DT_INST_PROP_OR(0, def_back_color_green, 0xFF),
			.Backcolor.Blue = DT_INST_PROP_OR(0, def_back_color_blue, 0xFF),
		},
	},
};

static const struct display_stm32_ltdc_config ltdc_config = {
	.width = DT_INST_PROP(0, width),
	.height = DT_INST_PROP(0, height),

	.disp_on_gpio.name = UTIL_AND(
		DT_INST_NODE_HAS_PROP(0, disp_on_gpios),
		DT_INST_GPIO_LABEL(0, disp_on_gpios)),
	.disp_on_gpio.pin = UTIL_AND(
		DT_INST_NODE_HAS_PROP(0, disp_on_gpios),
		DT_INST_GPIO_PIN(0, disp_on_gpios)),
	.disp_on_gpio.flags = UTIL_AND(
		DT_INST_NODE_HAS_PROP(0, disp_on_gpios),
		DT_INST_GPIO_FLAGS(0, disp_on_gpios)),
	.bl_ctrl_gpio.name = UTIL_AND(
		DT_INST_NODE_HAS_PROP(0, bl_ctrl_gpios),
		DT_INST_GPIO_LABEL(0, bl_ctrl_gpios)),
	.bl_ctrl_gpio.pin = UTIL_AND(
		DT_INST_NODE_HAS_PROP(0, bl_ctrl_gpios),
		DT_INST_GPIO_PIN(0, bl_ctrl_gpios)),
	.bl_ctrl_gpio.flags = UTIL_AND(
		DT_INST_NODE_HAS_PROP(0, bl_ctrl_gpios),
		DT_INST_GPIO_FLAGS(0, bl_ctrl_gpios)),

	.pclken = {
		.enr = DT_INST_CLOCKS_CELL(0, bits),
		.bus = DT_INST_CLOCKS_CELL(0, bus)
	},
	.pctrl = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
};

DEVICE_DT_INST_DEFINE(0, &stm32_ltdc_init, NULL, &ltdc_data, &ltdc_config,
		POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY, &stm32_ltdc_display_api);
