/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2021 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_pinmux

/* Include esp-idf headers first to avoid redefining BIT() macro */
#include <soc/soc.h>
#include <hal/gpio_types.h>
#include <hal/gpio_ll.h>
#include <hal/rtc_io_hal.h>
#include <soc.h>

#include <errno.h>
#include <drivers/pinmux.h>

#ifndef SOC_GPIO_SUPPORT_RTC_INDEPENDENT
#define SOC_GPIO_SUPPORT_RTC_INDEPENDENT 0
#endif

static inline bool rtc_gpio_is_valid_gpio(uint32_t gpio_num)
{
#if SOC_RTCIO_INPUT_OUTPUT_SUPPORTED
	return (gpio_num < SOC_GPIO_PIN_COUNT && rtc_io_num_map[gpio_num] >= 0);
#else
	return false;
#endif
}

static int pinmux_set(const struct device *dev, uint32_t pin, uint32_t func)
{
	ARG_UNUSED(dev);

	if ((func > PINMUX_FUNC_G) || (pin >= GPIO_NUM_MAX)) {
		return -EINVAL;
	}

	gpio_ll_iomux_func_sel(GPIO_PIN_MUX_REG[pin], func);

	return 0;
}

static int pinmux_get(const struct device *dev, uint32_t pin, uint32_t *func)
{
	ARG_UNUSED(dev);

	if (pin >= GPIO_NUM_MAX) {
		return -EINVAL;
	}

	*func = REG_GET_FIELD(GPIO_PIN_MUX_REG[pin], MCU_SEL);

	return 0;
}

static int pinmux_pullup(const struct device *dev, uint32_t pin, uint8_t func)
{
	ARG_UNUSED(dev);

	switch (func) {
	case PINMUX_PULLUP_DISABLE:
		if (!rtc_gpio_is_valid_gpio(pin) || SOC_GPIO_SUPPORT_RTC_INDEPENDENT) {
			gpio_ll_pullup_dis(&GPIO, pin);
			gpio_ll_pulldown_en(&GPIO, pin);
		} else {
#if SOC_RTCIO_INPUT_OUTPUT_SUPPORTED
			rtcio_hal_pullup_disable(rtc_io_num_map[pin]);
			rtcio_hal_pulldown_enable(rtc_io_num_map[pin]);
#endif
		}
		break;
	case PINMUX_PULLUP_ENABLE:
		if (!rtc_gpio_is_valid_gpio(pin) || SOC_GPIO_SUPPORT_RTC_INDEPENDENT) {
			gpio_ll_pulldown_dis(&GPIO, pin);
			gpio_ll_pullup_en(&GPIO, pin);
		} else {
#if SOC_RTCIO_INPUT_OUTPUT_SUPPORTED
			rtcio_hal_pulldown_disable(rtc_io_num_map[pin]);
			rtcio_hal_pullup_enable(rtc_io_num_map[pin]);
#endif
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int pinmux_input(const struct device *dev, uint32_t pin, uint8_t func)
{
	ARG_UNUSED(dev);

	switch (func) {
	case PINMUX_INPUT_ENABLED:
		gpio_ll_input_enable(&GPIO, pin);
		break;
	case PINMUX_OUTPUT_ENABLED:
		gpio_ll_output_enable(&GPIO, pin);
		esp_rom_gpio_matrix_out(pin, SIG_GPIO_OUT_IDX, false, false);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static struct pinmux_driver_api api_funcs = {
	.set = pinmux_set,
	.get = pinmux_get,
	.pullup = pinmux_pullup,
	.input = pinmux_input
};

static int pinmux_initialize(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

/* Initialize using PRE_KERNEL_1 priority so that GPIO can use the pin
 * mux driver.
 */
DEVICE_DT_INST_DEFINE(0, &pinmux_initialize,
		    NULL, NULL, NULL,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &api_funcs);
