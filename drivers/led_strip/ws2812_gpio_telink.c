/*
 * Copyright (c) 2023-2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT worldsemi_ws2812_gpio

#include <string.h>

#include <zephyr/drivers/led_strip.h>

#define LOG_LEVEL CONFIG_LED_STRIP_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ws2812_gpio);

#include <clock.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/dt-bindings/led/led.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#define GPIO_OUTPUT_REG_OFFSET		3

struct ws2812_gpio_cfg {
	struct gpio_dt_spec gpio;
	uint8_t num_colors;
	const uint8_t *color_mapping;
};

#define ASM_8_CYCLE_NOP	"nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n"
#define ASM_DELAY_T0H	ASM_8_CYCLE_NOP "nop\n"
#define ASM_DELAY_T0L	ASM_8_CYCLE_NOP ASM_8_CYCLE_NOP "nop\n"
#define ASM_DELAY_T1H	ASM_8_CYCLE_NOP ASM_8_CYCLE_NOP ASM_8_CYCLE_NOP "nop\nnop\nnop\n"
#define ASM_DELAY_T1L	"nop\n"

#define ONE_BIT(PORT_LOW_ADDR, PIN) do {                                \
	write_reg8((PORT_LOW_ADDR), read_reg8((PORT_LOW_ADDR)) | (PIN));    \
	__asm volatile (ASM_DELAY_T1H ::);                                  \
	write_reg8((PORT_LOW_ADDR), read_reg8((PORT_LOW_ADDR)) & ~(PIN));   \
	__asm volatile (ASM_DELAY_T1L ::);                                  \
} while (false)

#define ZERO_BIT(PORT_LOW_ADDR, PIN) do {                               \
	write_reg8((PORT_LOW_ADDR), read_reg8((PORT_LOW_ADDR)) | (PIN));    \
	__asm volatile (ASM_DELAY_T0H ::);                                  \
	write_reg8((PORT_LOW_ADDR), read_reg8((PORT_LOW_ADDR)) & ~(PIN));   \
	__asm volatile (ASM_DELAY_T0L ::);                                  \
} while (false)

_attribute_ram_code_ int send_buf(const struct device *dev, uint8_t *buf, size_t len)
{
	unsigned int key;
	uint8_t clk_sel_reg;
	const struct ws2812_gpio_cfg *cfg = dev->config;
	/* Load the lower 12 bits of the base address port for the asm LBU command */
	const uint32_t *portBaseAddr = (uint32_t *)((uint8_t *)cfg->gpio.port->config
			+ sizeof(struct gpio_driver_config));
	const uint32_t port_out_reg_addr = (*portBaseAddr + GPIO_OUTPUT_REG_OFFSET);
	const uint8_t pin = BIT(cfg->gpio.pin);

	key = irq_lock();

	/* Set the sclk to 48 MHZ */
	clk_sel_reg = read_reg8(0x1401e8);
	if ((clk_sel_reg & PAD_PLL_DIV) == 0) {
		irq_unlock(key);
		LOG_ERR("System clock type is not supported in this driver");
		return -EPERM;
	} else if ((clk_sel_reg & 0xf0) != (sys_clk.pll_clk/48)) {
		write_reg8(0x1401e8, (clk_sel_reg & 0xf0) | (sys_clk.pll_clk/48));
	} else {
		clk_sel_reg = 0;
	}

	while (len--) {
		uint32_t b = *buf++;
		int32_t i;

		for (i = 7; i >= 0; i--) {
			if (b & BIT(i)) {
				ONE_BIT(port_out_reg_addr, pin);
			} else {
				ZERO_BIT(port_out_reg_addr, pin);
			}
		}
	}

	if (clk_sel_reg) {
		/* Return the sclk */
		write_reg8(0x1401e8, clk_sel_reg);
	}

	irq_unlock(key);

	return 0;
}

static int ws2812_gpio_update_rgb(const struct device *dev,
				  struct led_rgb *pixels,
				  size_t num_pixels)
{
	const struct ws2812_gpio_cfg *cfg = dev->config;
	uint8_t *ptr = (uint8_t *)pixels;
	size_t i;

	/* Convert from RGB to on-wire format (e.g. GRB, GRBW, RGB, etc) */
	for (i = 0; i < num_pixels; i++) {
		uint8_t j;

		for (j = 0; j < cfg->num_colors; j++) {
			switch (cfg->color_mapping[j]) {
			/* White channel is not supported by LED strip API. */
			case LED_COLOR_ID_WHITE:
				*ptr++ = 0;
				break;
			case LED_COLOR_ID_RED:
				*ptr++ = pixels[i].r;
				break;
			case LED_COLOR_ID_GREEN:
				*ptr++ = pixels[i].g;
				break;
			case LED_COLOR_ID_BLUE:
				*ptr++ = pixels[i].b;
				break;
			default:
				return -EINVAL;
			}
		}
	}

	return send_buf(dev, (uint8_t *)pixels, num_pixels * cfg->num_colors);
}

static int ws2812_gpio_update_channels(const struct device *dev,
					   uint8_t *channels,
					   size_t num_channels)
{
	LOG_ERR("update_channels not implemented");
	return -ENOTSUP;
}

static const struct led_strip_driver_api ws2812_gpio_api = {
	.update_rgb = ws2812_gpio_update_rgb,
	.update_channels = ws2812_gpio_update_channels,
};

/*
 * Retrieve the channel to color mapping (e.g. RGB, BGR, GRB, ...) from the
 * "color-mapping" DT property.
 */
#define WS2812_COLOR_MAPPING(idx)					\
	static const uint8_t ws2812_gpio_##idx##_color_mapping[] =		\
		DT_INST_PROP(idx, color_mapping)

#define WS2812_NUM_COLORS(idx) (DT_INST_PROP_LEN(idx, color_mapping))

#define WS2812_GPIO_DEVICE(idx)					\
									\
	static int ws2812_gpio_##idx##_init(const struct device *dev)	\
	{								\
		const struct ws2812_gpio_cfg *cfg = dev->config;	\
		uint8_t i;						\
									\
		if (!device_is_ready(cfg->gpio.port)) {		\
			LOG_ERR("GPIO device not ready");		\
			return -ENODEV;					\
		}							\
									\
		for (i = 0; i < cfg->num_colors; i++) {			\
			switch (cfg->color_mapping[i]) {		\
			case LED_COLOR_ID_WHITE:			\
			case LED_COLOR_ID_RED:				\
			case LED_COLOR_ID_GREEN:			\
			case LED_COLOR_ID_BLUE:				\
				break;					\
			default:					\
				LOG_ERR("%s: invalid channel to color mapping." \
					" Check the color-mapping DT property",	\
					dev->name);			\
				return -EINVAL;				\
			}						\
		}							\
									\
		return gpio_pin_configure_dt(&cfg->gpio, GPIO_OUTPUT); \
	}								\
									\
	WS2812_COLOR_MAPPING(idx);					\
									\
	static const struct ws2812_gpio_cfg ws2812_gpio_##idx##_cfg = { \
		.gpio = GPIO_DT_SPEC_INST_GET(idx, gpios),	\
		.num_colors = WS2812_NUM_COLORS(idx),			\
		.color_mapping = ws2812_gpio_##idx##_color_mapping,	\
	};								\
									\
	DEVICE_DT_INST_DEFINE(idx,					\
			    ws2812_gpio_##idx##_init,			\
			    NULL,					\
			    NULL,					\
			    &ws2812_gpio_##idx##_cfg, POST_KERNEL,	\
			    CONFIG_LED_STRIP_INIT_PRIORITY,		\
			    &ws2812_gpio_api);

DT_INST_FOREACH_STATUS_OKAY(WS2812_GPIO_DEVICE)
