/*
 * Copyright (c) 2022 Byte-Lab d.o.o. <dev@byte-lab.com>
 * Copyright 2023 NXP
 * Copyright (c) 2024 STMicroelectronics
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
#include <zephyr/drivers/reset.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/cache.h>
#if defined(CONFIG_STM32_LTDC_FB_USE_SHARED_MULTI_HEAP)
#include <zephyr/multi_heap/shared_multi_heap.h>
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(display_stm32_ltdc, CONFIG_DISPLAY_LOG_LEVEL);

/* Horizontal synchronization pulse polarity */
#define LTDC_HSPOL_ACTIVE_LOW     0x00000000
#define LTDC_HSPOL_ACTIVE_HIGH    0x80000000

/* Vertical synchronization pulse polarity */
#define LTDC_VSPOL_ACTIVE_LOW     0x00000000
#define LTDC_VSPOL_ACTIVE_HIGH    0x40000000

/* Data enable pulse polarity */
#define LTDC_DEPOL_ACTIVE_LOW     0x00000000
#define LTDC_DEPOL_ACTIVE_HIGH    0x20000000

/* Pixel clock polarity */
#define LTDC_PCPOL_ACTIVE_LOW     0x00000000
#define LTDC_PCPOL_ACTIVE_HIGH    0x10000000

#if CONFIG_STM32_LTDC_ARGB8888
#define STM32_LTDC_INIT_PIXEL_SIZE	4u
#define STM32_LTDC_INIT_PIXEL_FORMAT	LTDC_PIXEL_FORMAT_ARGB8888
#define DISPLAY_INIT_PIXEL_FORMAT	PIXEL_FORMAT_ARGB_8888
#elif CONFIG_STM32_LTDC_RGB888
#define STM32_LTDC_INIT_PIXEL_SIZE	3u
#define STM32_LTDC_INIT_PIXEL_FORMAT	LTDC_PIXEL_FORMAT_RGB888
#define DISPLAY_INIT_PIXEL_FORMAT	PIXEL_FORMAT_RGB_888
#elif CONFIG_STM32_LTDC_RGB565
#define STM32_LTDC_INIT_PIXEL_SIZE	2u
#define STM32_LTDC_INIT_PIXEL_FORMAT	LTDC_PIXEL_FORMAT_RGB565
#define DISPLAY_INIT_PIXEL_FORMAT	PIXEL_FORMAT_RGB_565
#else
#error "Invalid LTDC pixel format chosen"
#endif

struct display_stm32_ltdc_data {
	LTDC_HandleTypeDef hltdc;
	enum display_pixel_format current_pixel_format;
	uint8_t current_pixel_size;
	uint8_t *frame_buffer;
	uint32_t frame_buffer_len;
	const uint8_t *pend_buf;
	const uint8_t *front_buf;
	struct k_sem sem;
};

struct display_stm32_ltdc_config {
	uint32_t width;
	uint32_t height;
	struct gpio_dt_spec disp_on_gpio;
	struct gpio_dt_spec bl_ctrl_gpio;
	const struct stm32_pclken *pclken;
	size_t pclk_len;
	const struct reset_dt_spec reset;
	const struct pinctrl_dev_config *pctrl;
	void (*irq_config_func)(const struct device *dev);
	const struct device *display_controller;
};

static void stm32_ltdc_global_isr(const struct device *dev)
{
	struct display_stm32_ltdc_data *data = dev->data;

	if (__HAL_LTDC_GET_FLAG(&data->hltdc, LTDC_FLAG_LI) &&
	    __HAL_LTDC_GET_IT_SOURCE(&data->hltdc, LTDC_IT_LI)) {
		if (data->front_buf != data->pend_buf) {
			data->front_buf = data->pend_buf;

			LTDC_LAYER(&data->hltdc, LTDC_LAYER_1)->CFBAR = (uint32_t)data->front_buf;
			__HAL_LTDC_RELOAD_CONFIG(&data->hltdc);

			k_sem_give(&data->sem);
		}

		__HAL_LTDC_CLEAR_FLAG(&data->hltdc, LTDC_FLAG_LI);
	}
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
	uint8_t *dst = NULL;
	const uint8_t *pend_buf = NULL;
	const uint8_t *src = buf;
	uint16_t row;

	if ((x == 0) && (y == 0) &&
	    (desc->width == config->width) &&
	    (desc->height ==  config->height) &&
	    (desc->pitch == desc->width)) {
		/* Use buf as ltdc frame buffer directly if it length same as ltdc frame buffer. */
		pend_buf = buf;
	} else {
		if (CONFIG_STM32_LTDC_FB_NUM == 0)  {
			LOG_ERR("Partial write requires internal frame buffer");
			return -ENOTSUP;
		}

		dst = data->frame_buffer;

		if (CONFIG_STM32_LTDC_FB_NUM == 2) {
			if (data->front_buf == data->frame_buffer) {
				dst = data->frame_buffer + data->frame_buffer_len;
			}

			memcpy(dst, data->front_buf, data->frame_buffer_len);
		}

		pend_buf = dst;

		/* dst = pointer to upper left pixel of the rectangle
		 *       to be updated in frame buffer.
		 */
		dst += (x * data->current_pixel_size);
		dst += (y * config->width * data->current_pixel_size);

		for (row = 0; row < desc->height; row++) {
			(void) memcpy(dst, src, desc->width * data->current_pixel_size);
			sys_cache_data_flush_range(dst, desc->width * data->current_pixel_size);
			dst += (config->width * data->current_pixel_size);
			src += (desc->pitch * data->current_pixel_size);
		}

	}

	if (data->front_buf == pend_buf) {
		return 0;
	}

	k_sem_reset(&data->sem);

	data->pend_buf = pend_buf;

	k_sem_take(&data->sem, K_FOREVER);

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
	const uint8_t *src = data->front_buf;
	uint16_t row;

	/* src = pointer to upper left pixel of the rectangle to be read from frame buffer */
	src += (x * data->current_pixel_size);
	src += (y * config->width * data->current_pixel_size);

	for (row = 0; row < desc->height; row++) {
		(void) memcpy(dst, src, desc->width * data->current_pixel_size);
		sys_cache_data_flush_range(dst, desc->width * data->current_pixel_size);
		src += (config->width * data->current_pixel_size);
		dst += (desc->pitch * data->current_pixel_size);
	}

	return 0;
}

static void *stm32_ltdc_get_framebuffer(const struct device *dev)
{
	struct display_stm32_ltdc_data *data = dev->data;

	return ((void *)data->front_buf);
}

static int stm32_ltdc_display_blanking_off(const struct device *dev)
{
	const struct display_stm32_ltdc_config *config = dev->config;
	const struct device *display_dev = config->display_controller;

	/* Panel controller's phandle is not passed to LTDC in devicetree */
	if (display_dev == NULL) {
		LOG_ERR("There is no panel controller to forward blanking_off call to");
		return -ENOSYS;
	}

	if (!device_is_ready(display_dev)) {
		LOG_ERR("Display device %s not ready", display_dev->name);
		return -ENODEV;
	}

	return display_blanking_off(display_dev);
}

static int stm32_ltdc_display_blanking_on(const struct device *dev)
{
	const struct display_stm32_ltdc_config *config = dev->config;
	const struct device *display_dev = config->display_controller;

	/* Panel controller's phandle is not passed to LTDC in devicetree */
	if (display_dev == NULL) {
		LOG_ERR("There is no panel controller to forward blanking_on call to");
		return -ENOSYS;
	}

	if (!device_is_ready(display_dev)) {
		LOG_ERR("Display device %s not ready", display_dev->name);
		return -ENODEV;
	}

	return display_blanking_on(display_dev);
}

/* This symbol takes the value 1 if one of the device instances */
/* is configured in dts with a domain clock */
#if STM32_DT_INST_DEV_DOMAIN_CLOCK_SUPPORT
#define STM32_LTDC_DOMAIN_CLOCK_SUPPORT 1
#else
#define STM32_LTDC_DOMAIN_CLOCK_SUPPORT 0
#endif

static int stm32_ltdc_init(const struct device *dev)
{
	int err;
	const struct display_stm32_ltdc_config *config = dev->config;
	struct display_stm32_ltdc_data *data = dev->data;
#if defined(CONFIG_SOC_SERIES_STM32N6X)
	RIMC_MasterConfig_t rimc = {0};
#endif

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
				(clock_control_subsys_t) &config->pclken[0]);
	if (err < 0) {
		LOG_ERR("Could not enable LTDC peripheral clock");
		return err;
	}

	if (IS_ENABLED(STM32_LTDC_DOMAIN_CLOCK_SUPPORT) && (config->pclk_len > 1)) {
		/* Enable LTDC clock source */
		err = clock_control_configure(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
					      (clock_control_subsys_t) &config->pclken[1],
					      NULL);
		if (err < 0) {
			LOG_ERR("Could not configure LTDC peripheral clock");
			return err;
		}
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
	(void)reset_line_toggle_dt(&config->reset);

	data->current_pixel_format = DISPLAY_INIT_PIXEL_FORMAT;
	data->current_pixel_size = STM32_LTDC_INIT_PIXEL_SIZE;

	k_sem_init(&data->sem, 0, 1);

	config->irq_config_func(dev);

#ifdef CONFIG_STM32_LTDC_DISABLE_FMC_BANK1
	/* Clear MBKEN and MTYP[1:0] bits. */
#ifdef CONFIG_SOC_SERIES_STM32F7X
	FMC_Bank1->BTCR[0] &= ~(0x0000000D);
#else /* CONFIG_SOC_SERIES_STM32H7X */
	FMC_Bank1_R->BTCR[0] &= ~(0x0000000D);
#endif
#endif /* CONFIG_STM32_LTDC_DISABLE_FMC_BANK1 */

	/* Initialise the LTDC peripheral */
	err = HAL_LTDC_Init(&data->hltdc);
	if (err != HAL_OK) {
		return err;
	}

#if defined(CONFIG_STM32_LTDC_FB_USE_SHARED_MULTI_HEAP)
	data->frame_buffer = shared_multi_heap_aligned_alloc(
			CONFIG_VIDEO_BUFFER_SMH_ATTRIBUTE,
			32,
			CONFIG_STM32_LTDC_FB_NUM * data->frame_buffer_len);

	if (data->frame_buffer == NULL) {
		return -ENOMEM;
	}

	data->pend_buf = data->frame_buffer;
	data->front_buf = data->frame_buffer;
	data->hltdc.LayerCfg[0].FBStartAdress = (uint32_t) data->frame_buffer;
#endif

	/* Configure layer 1 (only one layer is used) */
	/* LTDC starts fetching pixels and sending them to display after this call */
	err = HAL_LTDC_ConfigLayer(&data->hltdc, &data->hltdc.LayerCfg[0], LTDC_LAYER_1);
	if (err != HAL_OK) {
		return err;
	}

#if defined(CONFIG_SOC_SERIES_STM32N6X)
	/* Configure RIF for LTDC layer 1 */
	rimc.MasterCID = RIF_CID_1;
	rimc.SecPriv = RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV;
	HAL_RIF_RIMC_ConfigMasterAttributes(RIF_MASTER_INDEX_LTDC1 , &rimc);
	HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_LTDCL1,
					      RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV);
#endif

	/* Disable layer 2, since it not used */
	__HAL_LTDC_LAYER_DISABLE(&data->hltdc, LTDC_LAYER_2);

	/* Set the line interrupt position */
	LTDC->LIPCR = 0U;

	__HAL_LTDC_CLEAR_FLAG(&data->hltdc, LTDC_FLAG_LI);
	__HAL_LTDC_ENABLE_IT(&data->hltdc, LTDC_IT_LI);

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
	if (config->bl_ctrl_gpio.port) {
		err = gpio_pin_set_dt(&config->bl_ctrl_gpio, 0);
		if (err < 0) {
			return err;
		}
	}

	/* Reset LTDC peripheral registers */
	(void)reset_line_toggle_dt(&config->reset);

	/* Turn off LTDC peripheral clock */
	err = clock_control_off(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				(clock_control_subsys_t) &config->pclken[0]);

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

static DEVICE_API(display, stm32_ltdc_display_api) = {
	.write = stm32_ltdc_write,
	.read = stm32_ltdc_read,
	.get_framebuffer = stm32_ltdc_get_framebuffer,
	.get_capabilities = stm32_ltdc_get_capabilities,
	.set_pixel_format = stm32_ltdc_set_pixel_format,
	.set_orientation = stm32_ltdc_set_orientation,
	.blanking_off = stm32_ltdc_display_blanking_off,
	.blanking_on = stm32_ltdc_display_blanking_on,
};

#if DT_INST_NODE_HAS_PROP(0, ext_sdram)

#if DT_SAME_NODE(DT_INST_PHANDLE(0, ext_sdram), DT_NODELABEL(sdram1))
#define FRAME_BUFFER_SECTION __stm32_sdram1_section
#elif DT_SAME_NODE(DT_INST_PHANDLE(0, ext_sdram), DT_NODELABEL(sdram2))
#define FRAME_BUFFER_SECTION __stm32_sdram2_section
#elif DT_SAME_NODE(DT_INST_PHANDLE(0, ext_sdram), DT_NODELABEL(psram))
#define FRAME_BUFFER_SECTION __stm32_psram_section
#else
#error "LTDC ext-sdram property in device tree does not reference SDRAM1 or SDRAM2 node or PSRAM node"
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

#define STM32_LTDC_FRAME_BUFFER_LEN(inst)							\
	(STM32_LTDC_INIT_PIXEL_SIZE * DT_INST_PROP(inst, height) * DT_INST_PROP(inst, width))	\

#if defined(CONFIG_STM32_LTDC_FB_USE_SHARED_MULTI_HEAP)
#define STM32_LTDC_FRAME_BUFFER_ADDR(inst)  (NULL)
#define STM32_LTDC_FRAME_BUFFER_DEFINE(inst)
#else
#define STM32_LTDC_FRAME_BUFFER_ADDR(inst)  frame_buffer_##inst
#define STM32_LTDC_FRAME_BUFFER_DEFINE(inst)    \
	/* frame buffer aligned to cache line width for optimal cache flushing */                  \
	FRAME_BUFFER_SECTION static uint8_t __aligned(32)                                          \
		frame_buffer_##inst[CONFIG_STM32_LTDC_FB_NUM * STM32_LTDC_FRAME_BUFFER_LEN(inst)];
#endif

#define STM32_LTDC_DEVICE(inst)									\
	STM32_LTDC_FRAME_BUFFER_DEFINE(inst);                       \
	STM32_LTDC_DEVICE_PINCTRL_INIT(inst);							\
	PM_DEVICE_DT_INST_DEFINE(inst, stm32_ltdc_pm_action);					\
	static void stm32_ltdc_irq_config_func_##inst(const struct device *dev)			\
	{											\
		IRQ_CONNECT(DT_INST_IRQN(inst),							\
			    DT_INST_IRQ(inst, priority),					\
			    stm32_ltdc_global_isr,						\
			    DEVICE_DT_INST_GET(inst),						\
			    0);									\
		irq_enable(DT_INST_IRQN(inst));							\
	}											\
	static struct display_stm32_ltdc_data stm32_ltdc_data_##inst = {			\
		.frame_buffer = STM32_LTDC_FRAME_BUFFER_ADDR(inst),				\
		.frame_buffer_len = STM32_LTDC_FRAME_BUFFER_LEN(inst),				\
		.front_buf = STM32_LTDC_FRAME_BUFFER_ADDR(inst),				\
		.pend_buf = STM32_LTDC_FRAME_BUFFER_ADDR(inst),					\
		.hltdc = {									\
			.Instance = (LTDC_TypeDef *) DT_INST_REG_ADDR(inst),			\
			.Init = {								\
				.HSPolarity = (DT_PROP(DT_INST_CHILD(inst, display_timings),	\
						hsync_active) ?					\
						LTDC_HSPOL_ACTIVE_HIGH : LTDC_HSPOL_ACTIVE_LOW),\
				.VSPolarity = (DT_PROP(DT_INST_CHILD(inst,			\
						display_timings), vsync_active) ?		\
						LTDC_VSPOL_ACTIVE_HIGH : LTDC_VSPOL_ACTIVE_LOW),\
				.DEPolarity = (DT_PROP(DT_INST_CHILD(inst,			\
						display_timings), de_active) ?			\
						LTDC_DEPOL_ACTIVE_HIGH : LTDC_DEPOL_ACTIVE_LOW),\
				.PCPolarity = (DT_PROP(DT_INST_CHILD(inst,			\
						display_timings), pixelclk_active) ?		\
						LTDC_PCPOL_ACTIVE_HIGH : LTDC_PCPOL_ACTIVE_LOW),\
				.HorizontalSync = DT_PROP(DT_INST_CHILD(inst,			\
							display_timings), hsync_len) - 1,	\
				.VerticalSync = DT_PROP(DT_INST_CHILD(inst,			\
							display_timings), vsync_len) - 1,	\
				.AccumulatedHBP = DT_PROP(DT_INST_CHILD(inst,			\
							display_timings), hback_porch) +	\
							DT_PROP(DT_INST_CHILD(inst,		\
							display_timings), hsync_len) - 1,	\
				.AccumulatedVBP = DT_PROP(DT_INST_CHILD(inst,			\
							display_timings), vback_porch) +	\
							DT_PROP(DT_INST_CHILD(inst,		\
							display_timings), vsync_len) - 1,	\
				.AccumulatedActiveW = DT_PROP(DT_INST_CHILD(inst,		\
							display_timings), hback_porch) +	\
							DT_PROP(DT_INST_CHILD(inst,		\
							display_timings), hsync_len) +		\
							DT_INST_PROP(inst, width) - 1,		\
				.AccumulatedActiveH = DT_PROP(DT_INST_CHILD(inst,		\
							display_timings), vback_porch) +	\
							DT_PROP(DT_INST_CHILD(inst,		\
							display_timings), vsync_len) +		\
							DT_INST_PROP(inst, height) - 1,		\
				.TotalWidth = DT_PROP(DT_INST_CHILD(inst,			\
						display_timings), hback_porch) +		\
						DT_PROP(DT_INST_CHILD(inst,			\
						display_timings), hsync_len) +			\
						DT_INST_PROP(inst, width) +			\
						DT_PROP(DT_INST_CHILD(inst,			\
						display_timings), hfront_porch) - 1,		\
				.TotalHeigh = DT_PROP(DT_INST_CHILD(inst,			\
						display_timings), vback_porch) +		\
						DT_PROP(DT_INST_CHILD(inst,			\
						display_timings), vsync_len) +			\
						DT_INST_PROP(inst, height) +			\
						DT_PROP(DT_INST_CHILD(inst,			\
						display_timings), vfront_porch) - 1,		\
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
				.FBStartAdress = (uint32_t) STM32_LTDC_FRAME_BUFFER_ADDR(inst), \
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
	static const struct stm32_pclken pclken_##inst[] =			\
					 STM32_DT_INST_CLOCKS(inst);		\
										\
	static const struct display_stm32_ltdc_config stm32_ltdc_config_##inst = {		\
		.width = DT_INST_PROP(inst, width),						\
		.height = DT_INST_PROP(inst, height),						\
		.disp_on_gpio = COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, disp_on_gpios),		\
				(GPIO_DT_SPEC_INST_GET(inst, disp_on_gpios)), ({ 0 })),		\
		.bl_ctrl_gpio = COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, bl_ctrl_gpios),		\
				(GPIO_DT_SPEC_INST_GET(inst, bl_ctrl_gpios)), ({ 0 })),		\
		.reset = RESET_DT_SPEC_INST_GET(0),						\
		.pclken = pclken_##inst,					\
		.pclk_len = DT_INST_NUM_CLOCKS(inst),				\
		.pctrl = STM32_LTDC_DEVICE_PINCTRL_GET(inst),					\
		.irq_config_func = stm32_ltdc_irq_config_func_##inst,				\
		.display_controller = DEVICE_DT_GET_OR_NULL(					\
			DT_INST_PHANDLE(inst, display_controller)),				\
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
