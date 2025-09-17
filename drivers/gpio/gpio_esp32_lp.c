/*
 * Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_lpgpio

#include <soc.h>
#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/dt-bindings/gpio/espressif-esp32-gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <zephyr/drivers/gpio/gpio_utils.h>

#include <hal/rtc_io_hal.h>
#include <soc/rtc_io_periph.h>
#include <ulp_lp_core_interrupts.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gpio_esp32, CONFIG_LOG_DEFAULT_LEVEL);

struct gpio_esp32_lp_config {
	struct gpio_driver_config drv_cfg;
	lp_io_dev_t *const lp_io_dev;
};

struct gpio_esp32_lp_data {
	struct gpio_driver_data common;
	sys_slist_t cb;
};

void ulp_lp_core_lp_io_intr_handler(void)
{
	uint32_t intr_status = rtcio_ll_get_interrupt_status();
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(lp_gpio));
	struct gpio_esp32_lp_data *data = dev->data;

	rtcio_ll_clear_interrupt_status();
	gpio_fire_callbacks(&data->cb, dev, intr_status);
}

bool lp_gpio_is_valid(uint32_t pin)
{
	return rtc_io_num_map[pin] >= 0;
}

static int gpio_esp32_lp_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	if (!lp_gpio_is_valid(pin)) {
		LOG_ERR("Selected LP IO pin is not valid.");
		return -EINVAL;
	}

	rtcio_hal_function_select(pin, RTCIO_FUNC_RTC);

	if (flags & GPIO_OUTPUT) {
		rtcio_hal_set_direction(pin, RTC_GPIO_MODE_OUTPUT_ONLY);
		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			rtcio_hal_set_level(pin, 1);
		} else if (flags & GPIO_OUTPUT_INIT_LOW) {
			rtcio_hal_set_level(pin, 0);
		}
	} else if (flags & GPIO_INPUT) {
		rtcio_hal_set_direction(pin, RTC_GPIO_MODE_INPUT_ONLY);
	}

	return 0;
}

static int gpio_esp32_lp_port_get_raw(const struct device *port, uint32_t *value)
{
	const struct gpio_esp32_lp_config *const cfg = port->config;

	*value = cfg->lp_io_dev->in.val;

	return 0;
}

static int gpio_esp32_lp_port_set_masked_raw(const struct device *port, uint32_t mask,
					     uint32_t value)
{
	const struct gpio_esp32_lp_config *const cfg = port->config;

	cfg->lp_io_dev->out_data.val = (cfg->lp_io_dev->out_data.val & ~mask) | (mask & value);
	return 0;
}

static int gpio_esp32_lp_port_set_bits_raw(const struct device *port, uint32_t pins)
{
	const struct gpio_esp32_lp_config *const cfg = port->config;

	cfg->lp_io_dev->out_data_w1ts.val = pins;
	return 0;
}

static int gpio_esp32_lp_port_clear_bits_raw(const struct device *port, uint32_t pins)
{
	const struct gpio_esp32_lp_config *const cfg = port->config;

	cfg->lp_io_dev->out_data_w1tc.val = pins;
	return 0;
}

static int gpio_esp32_lp_port_toggle_bits(const struct device *port, uint32_t pins)
{
	const struct gpio_esp32_lp_config *const cfg = port->config;

	cfg->lp_io_dev->out_data.val ^= pins;
	return 0;
}

static int lp_gpio_convert_int_type(enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	if (mode == GPIO_INT_MODE_DISABLED) {
		return RTCIO_INTR_DISABLE;
	}

	if (mode == GPIO_INT_MODE_LEVEL) {
		switch (trig) {
		case GPIO_INT_TRIG_LOW:
			return RTCIO_INTR_LOW_LEVEL;
		case GPIO_INT_TRIG_HIGH:
			return RTCIO_INTR_HIGH_LEVEL;
		default:
			return -EINVAL;
		}
	} else { /* edge interrupts */
		switch (trig) {
		case GPIO_INT_TRIG_HIGH:
			return RTCIO_INTR_POSEDGE;
		case GPIO_INT_TRIG_LOW:
			return RTCIO_INTR_NEGEDGE;
		case GPIO_INT_TRIG_BOTH:
			return RTCIO_INTR_ANYEDGE;
		default:
			return -EINVAL;
		}
	}

	/* Any other type of interrupt triggering is invalid. */
	return -EINVAL;
}

static int gpio_esp32_lp_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
						 enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	int intr_trig_mode = lp_gpio_convert_int_type(mode, trig);

	if (!lp_gpio_is_valid(pin)) {
		LOG_ERR("Selected LP IO pin is not valid.");
		return -EINVAL;
	}

	rtcio_ll_clear_interrupt_status();
	ulp_lp_core_intr_enable();

	rtcio_ll_intr_enable(pin, intr_trig_mode);

	return 0;
}

static int gpio_esp32_lp_manage_callback(const struct device *dev, struct gpio_callback *callback,
					 bool set)
{
	struct gpio_esp32_lp_data *data = dev->data;

	return gpio_manage_callback(&data->cb, callback, set);
}

static uint32_t gpio_esp32_lp_get_pending_int(const struct device *dev)
{
	ARG_UNUSED(dev);

	return rtcio_ll_get_interrupt_status();
}

static int gpio_esp32_lp_init(const struct device *dev)
{
	return 0;
}

static DEVICE_API(gpio, gpio_esp32_lp_driver_api) = {
	.pin_configure = gpio_esp32_lp_configure,
	.port_get_raw = gpio_esp32_lp_port_get_raw,
	.port_set_masked_raw = gpio_esp32_lp_port_set_masked_raw,
	.port_set_bits_raw = gpio_esp32_lp_port_set_bits_raw,
	.port_clear_bits_raw = gpio_esp32_lp_port_clear_bits_raw,
	.port_toggle_bits = gpio_esp32_lp_port_toggle_bits,
	.pin_interrupt_configure = gpio_esp32_lp_pin_interrupt_configure,
	.manage_callback = gpio_esp32_lp_manage_callback,
	.get_pending_int = gpio_esp32_lp_get_pending_int
};

static struct gpio_esp32_lp_data gpio_esp32_lp_data_0;
static struct gpio_esp32_lp_config gpio_esp32_lp_cfg = {
	.drv_cfg = {
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_NODE(DT_NODELABEL(lp_gpio)),
		},
	.lp_io_dev = (lp_io_dev_t *)DT_REG_ADDR(DT_NODELABEL(lp_gpio)),
};

DEVICE_DT_DEFINE(DT_NODELABEL(lp_gpio), gpio_esp32_lp_init, NULL, &gpio_esp32_lp_data_0,
		 &gpio_esp32_lp_cfg, PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY,
		 &gpio_esp32_lp_driver_api);
