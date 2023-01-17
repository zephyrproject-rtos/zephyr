/*
 * Copyright (c) 2022 Byte-Lab d.o.o. <dev@byte-lab.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_ltdc

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <stm32_ll_rcc.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/pm/device.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(display_stm32_ltdc, CONFIG_DISPLAY_LOG_LEVEL);

#if defined(CONFIG_STM32_LTDC_ARGB8888)
#define STM32_LTDC_INIT_PIXEL_SIZE	4u
#define STM32_LTDC_INIT_PIXEL_FORMAT	LTDC_PIXEL_FORMAT_ARGB8888
#define DISPLAY_INIT_PIXEL_FORMAT	PIXEL_FORMAT_ARGB_8888
#elif defined(CONFIG_STM32_LTDC_RGB888)
#define STM32_LTDC_INIT_PIXEL_SIZE	3u
#define STM32_LTDC_INIT_PIXEL_FORMAT	LTDC_PIXEL_FORMAT_RGB888
#define DISPLAY_INIT_PIXEL_FORMAT	PIXEL_FORMAT_RGB_888
#elif defined(CONFIG_STM32_LTDC_RGB565)
#define STM32_LTDC_INIT_PIXEL_SIZE	2u
#define STM32_LTDC_INIT_PIXEL_FORMAT	LTDC_PIXEL_FORMAT_RGB565
#define DISPLAY_INIT_PIXEL_FORMAT	PIXEL_FORMAT_RGB_565
#else
#error "Invalid LTDC pixel format chosen"
#endif /* CONFIG_STM32_LTDC_ARGB8888 */

#if defined(CONFIG_HAS_CMSIS_CORE_M)
#include <zephyr/arch/arm/aarch32/cortex_m/cmsis.h>

#if __DCACHE_PRESENT == 1
#define CACHE_INVALIDATE(addr, size)	SCB_InvalidateDCache_by_Addr((addr), (size))
#define CACHE_CLEAN(addr, size)		SCB_CleanDCache_by_Addr((addr), (size))
#else
#define CACHE_INVALIDATE(addr, size)
#define CACHE_CLEAN(addr, size)		__DSB()
#endif /* __DCACHE_PRESENT == 1 */

#else
#define CACHE_INVALIDATE(addr, size)
#define CACHE_CLEAN(addr, size)
#endif /* CONFIG_HAS_CMSIS_CORE_M */

struct display_stm32_ltdc_data {
	LTDC_HandleTypeDef hltdc;
	enum display_pixel_format current_pixel_format;
	uint8_t current_pixel_size;
	uint8_t *frame_buffer;
};

struct display_stm32_ltdc_config {
	uint32_t width;
	uint32_t height;
	struct gpio_dt_spec disp_on_gpio;
	struct gpio_dt_spec bl_ctrl_gpio;
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
	struct display_stm32_ltdc_data *data = dev->data;

	return (void *) data->frame_buffer;
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
	struct display_stm32_ltdc_data *data = dev->data;

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
	ARG_UNUSED(dev);

	if (orientation == DISPLAY_ORIENTATION_NORMAL) {
		return 0;
	}

	return -ENOTSUP;
}

static void stm32_ltdc_get_capabilities(const struct device *dev,
				struct display_capabilities *capabilities)
{
	struct display_stm32_ltdc_data *data = dev->data;

	memset(capabilities, 0, sizeof(struct display_capabilities));

	capabilities->x_resolution = data->hltdc.LayerCfg[0].WindowX1 -
				     data->hltdc.LayerCfg[0].WindowX0;
	capabilities->y_resolution = data->hltdc.LayerCfg[0].WindowY1 -
				     data->hltdc.LayerCfg[0].WindowY0;
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
	const struct display_stm32_ltdc_config *config = dev->config;
	struct display_stm32_ltdc_data *data = dev->data;
	uint8_t *dst = data->frame_buffer;
	const uint8_t *src = buf;
	uint16_t row;

	/* dst = pointer to upper left pixel of the rectangle to be updated in frame buffer */
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
	const struct display_stm32_ltdc_config *config = dev->config;
	struct display_stm32_ltdc_data *data = dev->data;
	uint8_t *dst = buf;
	const uint8_t *src = data->frame_buffer;
	uint16_t row;

	/* src = pointer to upper left pixel of the rectangle to be read from frame buffer */
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
	const struct display_stm32_ltdc_config *config = dev->config;
	struct display_stm32_ltdc_data *data = dev->data;

	/* Configure and set display on/off GPIO */
	if (config->disp_on_gpio.port) {
		err = gpio_pin_configure_dt(&config->disp_on_gpio, GPIO_OUTPUT_ACTIVE);
		if (err < 0) {
			LOG_ERR("Configuration of display on/off control GPIO failed");
			return err;
		}
	}

	/* Configure and set display backlight control GPIO */
	if (config->bl_ctrl_gpio.port) {
		err = gpio_pin_configure_dt(&config->bl_ctrl_gpio, GPIO_OUTPUT_ACTIVE);
		if (err < 0) {
			LOG_ERR("Configuration of display backlight control GPIO failed");
			return err;
		}
	}

	/* Configure DT provided pins */
	if (!IS_ENABLED(CONFIG_MIPI_DSI)) {
		err = pinctrl_apply_state(config->pctrl, PINCTRL_STATE_DEFAULT);
		if (err < 0) {
			LOG_ERR("LTDC pinctrl setup failed");
			return err;
		}
	}

	if (!device_is_ready(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE))) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	/* Turn on LTDC peripheral clock */
	err = clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				(clock_control_subsys_t) &config->pclken);
	if (err < 0) {
		LOG_ERR("Could not enable LTDC peripheral clock");
		return err;
	}

#if defined(CONFIG_SOC_SERIES_STM32F4X)
	LL_RCC_PLLSAI_Disable();
	LL_RCC_PLLSAI_ConfigDomain_LTDC(LL_RCC_PLLSOURCE_HSE,
					LL_RCC_PLLSAIM_DIV_8,
					192,
					LL_RCC_PLLSAIR_DIV_4,
					LL_RCC_PLLSAIDIVR_DIV_8);

	LL_RCC_PLLSAI_Enable();
	while (LL_RCC_PLLSAI_IsReady() != 1) {
	}
#endif

#if defined(CONFIG_SOC_SERIES_STM32F7X)
	LL_RCC_PLLSAI_Disable();
	LL_RCC_PLLSAI_ConfigDomain_LTDC(LL_RCC_PLLSOURCE_HSE,
					LL_RCC_PLLM_DIV_25,
					384,
					LL_RCC_PLLSAIR_DIV_5,
					LL_RCC_PLLSAIDIVR_DIV_8);

	LL_RCC_PLLSAI_Enable();
	while (LL_RCC_PLLSAI_IsReady() != 1) {
	}
#endif

	/* reset LTDC peripheral */
	__HAL_RCC_LTDC_FORCE_RESET();
	__HAL_RCC_LTDC_RELEASE_RESET();

	data->current_pixel_format = DISPLAY_INIT_PIXEL_FORMAT;
	data->current_pixel_size = STM32_LTDC_INIT_PIXEL_SIZE;

	/* Initialise the LTDC peripheral */
	err = HAL_LTDC_Init(&data->hltdc);
	if (err != HAL_OK) {
		return err;
	}

	/* Configure layer 0 (only one layer is used) */
	/* LTDC starts fetching pixels and sending them to display after this call */
	err = HAL_LTDC_ConfigLayer(&data->hltdc, &data->hltdc.LayerCfg[0], 0);
	if (err != HAL_OK) {
		return err;
	}

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int stm32_ltdc_suspend(const struct device *dev)
{
	const struct display_stm32_ltdc_config *config = dev->config;
	int err;

	/* Turn off disp_en (if its GPIO is defined in device tree) */
	if (config->disp_on_gpio.port) {
		err = gpio_pin_set_dt(&config->disp_on_gpio, 0);
		if (err < 0) {
			return err;
		}
	}

	/* Turn off backlight (if its GPIO is defined in device tree) */
	if (config->disp_on_gpio.port) {
		err = gpio_pin_set_dt(&config->bl_ctrl_gpio, 0);
		if (err < 0) {
			return err;
		}
	}

	/* Reset LTDC peripheral registers */
	__HAL_RCC_LTDC_FORCE_RESET();
	__HAL_RCC_LTDC_RELEASE_RESET();

	/* Turn off LTDC peripheral clock */
	err = clock_control_off(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				(clock_control_subsys_t) &config->pclken);

	return err;
}

static int stm32_ltdc_pm_action(const struct device *dev,
				enum pm_device_action action)
{
	int err;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		err = stm32_ltdc_init(dev);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		err = stm32_ltdc_suspend(dev);
		break;
	default:
		return -ENOTSUP;
	}

	if (err < 0) {
		LOG_ERR("%s: failed to set power mode", dev->name);
	}

	return err;
}
#endif /* CONFIG_PM_DEVICE */

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

#if DT_INST_NODE_HAS_PROP(0, ext_sdram)

#if DT_SAME_NODE(DT_INST_PHANDLE(0, ext_sdram), DT_NODELABEL(sdram1))
#define FRAME_BUFFER_SECTION __stm32_sdram1_section
#elif DT_SAME_NODE(DT_INST_PHANDLE(0, ext_sdram), DT_NODELABEL(sdram2))
#define FRAME_BUFFER_SECTION __stm32_sdram2_section
#else
#error "LTDC ext-sdram property in device tree does not reference SDRAM1 or SDRAM2 node"
#define FRAME_BUFFER_SECTION
#endif /* DT_SAME_NODE(DT_INST_PHANDLE(0, ext_sdram), DT_NODELABEL(sdram1)) */

#else
#define FRAME_BUFFER_SECTION
#endif /* DT_INST_NODE_HAS_PROP(0, ext_sdram) */

#ifdef CONFIG_MIPI_DSI
#define STM32_LTDC_DEVICE_PINCTRL_INIT(n)
#define STM32_LTDC_DEVICE_PINCTRL_GET(n) (NULL)
#else
#define STM32_LTDC_DEVICE_PINCTRL_INIT(n) PINCTRL_DT_INST_DEFINE(n)
#define STM32_LTDC_DEVICE_PINCTRL_GET(n) PINCTRL_DT_INST_DEV_CONFIG_GET(n)
#endif

#define STM32_LTDC_DEVICE(inst)									\
	STM32_LTDC_DEVICE_PINCTRL_INIT(inst);							\
	PM_DEVICE_DT_INST_DEFINE(inst, stm32_ltdc_pm_action);					\
	/* frame buffer aligned to cache line width for optimal cache flushing */		\
	FRAME_BUFFER_SECTION static uint8_t __aligned(32)					\
				frame_buffer_##inst[STM32_LTDC_INIT_PIXEL_SIZE *		\
						DT_INST_PROP(inst, height) *			\
						DT_INST_PROP(inst, width)];			\
	static struct display_stm32_ltdc_data stm32_ltdc_data_##inst = {			\
		.frame_buffer = frame_buffer_##inst,						\
		.hltdc = {									\
			.Instance = (LTDC_TypeDef *) DT_INST_REG_ADDR(inst),			\
			.Init = {								\
				.HSPolarity = DT_INST_PROP(inst, hsync_pol),			\
				.VSPolarity = DT_INST_PROP(inst, vsync_pol),			\
				.DEPolarity = DT_INST_PROP(inst, de_pol),			\
				.PCPolarity = DT_INST_PROP(inst, pclk_pol),			\
				.HorizontalSync = DT_INST_PROP(inst, hsync_duration) - 1,	\
				.VerticalSync = DT_INST_PROP(inst, vsync_duration) - 1,		\
				.AccumulatedHBP = DT_INST_PROP(inst, hbp_duration) +		\
						DT_INST_PROP(inst, hsync_duration) - 1,		\
				.AccumulatedVBP = DT_INST_PROP(inst, vbp_duration) +		\
						DT_INST_PROP(inst, vsync_duration) - 1,		\
				.AccumulatedActiveW = DT_INST_PROP(inst, hbp_duration) +	\
						DT_INST_PROP(inst, hsync_duration) +		\
						DT_INST_PROP(inst, width) - 1,			\
				.AccumulatedActiveH = DT_INST_PROP(inst, vbp_duration) +	\
						DT_INST_PROP(inst, vsync_duration) +		\
						DT_INST_PROP(inst, height) - 1,			\
				.TotalWidth = DT_INST_PROP(inst, hbp_duration) +		\
						DT_INST_PROP(inst, hsync_duration) +		\
						DT_INST_PROP(inst, width) +			\
						DT_INST_PROP(inst, hfp_duration) - 1,		\
				.TotalHeigh = DT_INST_PROP(inst, vbp_duration) +		\
						DT_INST_PROP(inst, vsync_duration) +		\
						DT_INST_PROP(inst, height) +			\
						DT_INST_PROP(inst, vfp_duration) - 1,		\
				.Backcolor.Red =						\
					DT_INST_PROP_OR(inst, def_back_color_red, 0xFF),	\
				.Backcolor.Green =						\
					DT_INST_PROP_OR(inst, def_back_color_green, 0xFF),	\
				.Backcolor.Blue =						\
					DT_INST_PROP_OR(inst, def_back_color_blue, 0xFF),	\
			},									\
			.LayerCfg[0] = {							\
				.WindowX0 = DT_INST_PROP_OR(inst, window0_x0, 0),		\
				.WindowX1 = DT_INST_PROP_OR(inst, window0_x1,			\
								DT_INST_PROP(inst, width)),	\
				.WindowY0 = DT_INST_PROP_OR(inst, window0_y0, 0),		\
				.WindowY1 = DT_INST_PROP_OR(inst, window0_y1,			\
								DT_INST_PROP(inst, height)),	\
				.PixelFormat = STM32_LTDC_INIT_PIXEL_FORMAT,			\
				.Alpha = 255,							\
				.Alpha0 = 0,							\
				.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA,			\
				.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA,			\
				.FBStartAdress = (uint32_t) frame_buffer_##inst,		\
				.ImageWidth = DT_INST_PROP(inst, width),			\
				.ImageHeight = DT_INST_PROP(inst, height),			\
				.Backcolor.Red =						\
					DT_INST_PROP_OR(inst, def_back_color_red, 0xFF),	\
				.Backcolor.Green =						\
					DT_INST_PROP_OR(inst, def_back_color_green, 0xFF),	\
				.Backcolor.Blue =						\
					DT_INST_PROP_OR(inst, def_back_color_blue, 0xFF),	\
			},									\
		},										\
	};											\
	static const struct display_stm32_ltdc_config stm32_ltdc_config_##inst = {		\
		.width = DT_INST_PROP(inst, width),						\
		.height = DT_INST_PROP(inst, height),						\
		.disp_on_gpio = COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, disp_on_gpios),		\
				(GPIO_DT_SPEC_INST_GET(inst, disp_on_gpios)), ({ 0 })),		\
		.bl_ctrl_gpio = COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, bl_ctrl_gpios),		\
				(GPIO_DT_SPEC_INST_GET(inst, bl_ctrl_gpios)), ({ 0 })),		\
		.pclken = {									\
			.enr = DT_INST_CLOCKS_CELL(inst, bits),					\
			.bus = DT_INST_CLOCKS_CELL(inst, bus)					\
		},										\
		.pctrl = STM32_LTDC_DEVICE_PINCTRL_GET(inst),					\
	};											\
	DEVICE_DT_INST_DEFINE(inst,								\
			&stm32_ltdc_init,							\
			PM_DEVICE_DT_INST_GET(inst),						\
			&stm32_ltdc_data_##inst,						\
			&stm32_ltdc_config_##inst,						\
			POST_KERNEL,								\
			CONFIG_DISPLAY_INIT_PRIORITY,						\
			&stm32_ltdc_display_api);

DT_INST_FOREACH_STATUS_OKAY(STM32_LTDC_DEVICE)
