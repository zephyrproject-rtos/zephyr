/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2021 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_gpio

/* Include esp-idf headers first to avoid redefining BIT() macro */
#include <soc/dport_reg.h>
#include <soc/gpio_reg.h>
#include <soc/io_mux_reg.h>
#include <soc/soc.h>
#include <hal/gpio_ll.h>
#include <esp_attr.h>

#include <errno.h>
#include <device.h>
#include <drivers/gpio.h>
#include <drivers/interrupt_controller/intc_esp32.h>
#include <kernel.h>
#include <sys/util.h>
#include <drivers/pinmux.h>

#include "gpio_utils.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(gpio_esp32, CONFIG_LOG_DEFAULT_LEVEL);

#define DEV_CFG(_dev) ((struct gpio_esp32_config *const)(_dev)->config)

struct gpio_esp32_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config drv_cfg;
	gpio_dev_t *const gpio_base;
	gpio_dev_t *const gpio_dev;
	const bool gpio_port0;
};

struct gpio_esp32_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	const struct device *pinmux;
	sys_slist_t cb;
};

static inline bool gpio_pin_is_valid(uint32_t pin)
{
	return ((BIT(pin) & SOC_GPIO_VALID_GPIO_MASK) != 0);
}

static inline bool gpio_pin_is_output_capable(uint32_t pin)
{
	return ((BIT(pin) & SOC_GPIO_VALID_OUTPUT_GPIO_MASK) != 0);
}

static inline int gpio_get_pin_offset(const struct device *dev)
{
	struct gpio_esp32_config *const cfg = DEV_CFG(dev);

	return ((cfg->gpio_port0) ? 0 : 32);
}

static int gpio_esp32_config(const struct device *dev,
			     gpio_pin_t pin,
			     gpio_flags_t flags)
{
	struct gpio_esp32_config *const cfg = DEV_CFG(dev);
	struct gpio_esp32_data *data = dev->data;
	uint32_t io_pin = pin + gpio_get_pin_offset(dev);
	uint32_t key;
	int r = 0;

	if (!gpio_pin_is_valid(io_pin)) {
		return -EINVAL;
	}

	key = irq_lock();

	/* Set pin function as GPIO */
	pinmux_pin_set(data->pinmux, io_pin, PIN_FUNC_GPIO);

	if (flags & GPIO_PULL_UP) {
		pinmux_pin_pullup(data->pinmux, io_pin, PINMUX_PULLUP_ENABLE);
	} else if (flags & GPIO_PULL_DOWN) {
		pinmux_pin_pullup(data->pinmux, io_pin, PINMUX_PULLUP_DISABLE);
	}

	if (flags & GPIO_OUTPUT) {

		if (!gpio_pin_is_output_capable(pin)) {
			LOG_ERR("GPIO can only be used as input");
			return -EINVAL;
		}

		if (flags & GPIO_SINGLE_ENDED) {
			if (flags & GPIO_LINE_OPEN_DRAIN) {
				gpio_ll_od_enable(cfg->gpio_base, io_pin);
			} else {
				r = -ENOTSUP;
			}
		} else {
			gpio_ll_od_disable(cfg->gpio_base, io_pin);
		}

		/*
		 * By default, drive strength is set to its maximum value when the pin is set
		 * to either low or high states. Alternative drive strength is weak-only,
		 * while any other intermediary combination is considered invalid.
		 */
		switch (flags & (GPIO_DS_LOW_MASK | GPIO_DS_HIGH_MASK)) {
		case GPIO_DS_DFLT_LOW | GPIO_DS_DFLT_HIGH:
			gpio_ll_set_drive_capability(cfg->gpio_base, io_pin, GPIO_DRIVE_CAP_3);
			break;
		case GPIO_DS_ALT_LOW | GPIO_DS_ALT_HIGH:
			gpio_ll_set_drive_capability(cfg->gpio_base, io_pin, GPIO_DRIVE_CAP_0);
			break;
		default:
			return -EINVAL;
		}

		/* Set output pin initial value */
		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			gpio_ll_set_level(cfg->gpio_base, io_pin, 1);
		} else if (flags & GPIO_OUTPUT_INIT_LOW) {
			gpio_ll_set_level(cfg->gpio_base, io_pin, 0);
		}

		r = pinmux_pin_input_enable(data->pinmux, io_pin, PINMUX_OUTPUT_ENABLED);

	} else { /* Input */
		r = pinmux_pin_input_enable(data->pinmux, io_pin, PINMUX_INPUT_ENABLED);
	}

	irq_unlock(key);

	return r;
}

static int gpio_esp32_port_get_raw(const struct device *port, uint32_t *value)
{
	struct gpio_esp32_config *const cfg = DEV_CFG(port);

	if (cfg->gpio_port0) {
		*value = cfg->gpio_dev->in;
	} else {
		*value = cfg->gpio_dev->in1.data;
	}

	return 0;
}

static int gpio_esp32_port_set_masked_raw(const struct device *port,
					  uint32_t mask, uint32_t value)
{
	struct gpio_esp32_config *const cfg = DEV_CFG(port);

	uint32_t key = irq_lock();

	if (cfg->gpio_port0) {
		cfg->gpio_dev->out = (cfg->gpio_dev->out & ~mask) | (mask & value);
	} else {
		cfg->gpio_dev->out1.data = (cfg->gpio_dev->out1.data & ~mask) | (mask & value);
	}

	irq_unlock(key);

	return 0;
}

static int gpio_esp32_port_set_bits_raw(const struct device *port,
					uint32_t pins)
{
	struct gpio_esp32_config *const cfg = DEV_CFG(port);

	if (cfg->gpio_port0) {
		cfg->gpio_dev->out_w1ts = pins;
	} else {
		cfg->gpio_dev->out1_w1ts.data = pins;
	}

	return 0;
}

static int gpio_esp32_port_clear_bits_raw(const struct device *port,
					  uint32_t pins)
{
	struct gpio_esp32_config *const cfg = DEV_CFG(port);

	if (cfg->gpio_port0) {
		cfg->gpio_dev->out_w1tc = pins;
	} else {
		cfg->gpio_dev->out1_w1tc.data = pins;
	}

	return 0;
}

static int gpio_esp32_port_toggle_bits(const struct device *port,
				       uint32_t pins)
{
	struct gpio_esp32_config *const cfg = DEV_CFG(port);
	uint32_t key = irq_lock();

	if (cfg->gpio_port0) {
		cfg->gpio_dev->out ^= pins;
	} else {
		cfg->gpio_dev->out1.data ^= pins;
	}

	irq_unlock(key);

	return 0;
}

static int convert_int_type(enum gpio_int_mode mode,
			    enum gpio_int_trig trig)
{
	if (mode == GPIO_INT_MODE_DISABLED) {
		return GPIO_INTR_DISABLE;
	}

	if (mode == GPIO_INT_MODE_LEVEL) {
		switch (trig) {
		case GPIO_INT_TRIG_LOW:
			return GPIO_INTR_LOW_LEVEL;
		case GPIO_INT_TRIG_HIGH:
			return GPIO_INTR_HIGH_LEVEL;
		default:
			return -EINVAL;
		}
	} else { /* edge interrupts */
		switch (trig) {
		case GPIO_INT_TRIG_HIGH:
			return GPIO_INTR_POSEDGE;
		case GPIO_INT_TRIG_LOW:
			return GPIO_INTR_NEGEDGE;
		case GPIO_INT_TRIG_BOTH:
			return GPIO_INTR_ANYEDGE;
		default:
			return -EINVAL;
		}
	}

	/* Any other type of interrupt triggering is invalid. */
	return -EINVAL;
}

static int gpio_esp32_pin_interrupt_configure(const struct device *port,
					      gpio_pin_t pin,
					      enum gpio_int_mode mode,
					      enum gpio_int_trig trig)
{
	struct gpio_esp32_config *const cfg = DEV_CFG(port);
	uint32_t io_pin = pin + gpio_get_pin_offset(port);
	int intr_trig_mode = convert_int_type(mode, trig);
	uint32_t key;

	if (intr_trig_mode < 0) {
		return intr_trig_mode;
	}

	key = irq_lock();
	gpio_ll_set_intr_type(cfg->gpio_base, io_pin, intr_trig_mode);
	irq_unlock(key);

	return 0;
}

static int gpio_esp32_manage_callback(const struct device *dev,
				      struct gpio_callback *callback,
				      bool set)
{
	struct gpio_esp32_data *data = dev->data;

	return gpio_manage_callback(&data->cb, callback, set);
}

static uint32_t gpio_esp32_get_pending_int(const struct device *dev)
{
	struct gpio_esp32_config *const cfg = DEV_CFG(dev);
	uint32_t irq_status;
	uint32_t const core_id = arch_curr_cpu()->id;

#if defined(CONFIG_GPIO_ESP32_1)
	gpio_ll_get_intr_status_high(cfg->gpio_base, core_id, &irq_status);
#else
	gpio_ll_get_intr_status(cfg->gpio_base, core_id, &irq_status);
#endif
	return irq_status;
}

static void IRAM_ATTR gpio_esp32_fire_callbacks(const struct device *dev)
{
	struct gpio_esp32_config *const cfg = DEV_CFG(dev);
	struct gpio_esp32_data *data = dev->data;
	uint32_t irq_status;
	uint32_t const core_id = arch_curr_cpu()->id;

#if defined(CONFIG_GPIO_ESP32_1)
	gpio_ll_get_intr_status_high(cfg->gpio_base, core_id, &irq_status);
	gpio_ll_clear_intr_status_high(cfg->gpio_base, irq_status);
#else
	gpio_ll_get_intr_status(cfg->gpio_base, core_id, &irq_status);
	gpio_ll_clear_intr_status(cfg->gpio_base, irq_status);
#endif

	gpio_fire_callbacks(&data->cb, dev, irq_status);
}

static void IRAM_ATTR gpio_esp32_isr(void *param);

static int gpio_esp32_init(const struct device *dev)
{
	struct gpio_esp32_data *data = dev->data;
	static bool isr_connected;

	data->pinmux = DEVICE_DT_GET(DT_NODELABEL(pinmux));
	if ((data->pinmux != NULL)
	    && !device_is_ready(data->pinmux)) {
		data->pinmux = NULL;
	}

	if (!data->pinmux) {
		return -ENOTSUP;
	}

	if (!isr_connected) {
		esp_intr_alloc(DT_IRQN(DT_NODELABEL(gpio0)), 0, gpio_esp32_isr, (void *)dev, NULL);
		isr_connected = true;
	}

	return 0;
}

static const struct gpio_driver_api gpio_esp32_driver_api = {
	.pin_configure = gpio_esp32_config,
	.port_get_raw = gpio_esp32_port_get_raw,
	.port_set_masked_raw = gpio_esp32_port_set_masked_raw,
	.port_set_bits_raw = gpio_esp32_port_set_bits_raw,
	.port_clear_bits_raw = gpio_esp32_port_clear_bits_raw,
	.port_toggle_bits = gpio_esp32_port_toggle_bits,
	.pin_interrupt_configure = gpio_esp32_pin_interrupt_configure,
	.manage_callback = gpio_esp32_manage_callback,
	.get_pending_int = gpio_esp32_get_pending_int
};

#define ESP_SOC_GPIO_INIT(_id)							\
	static struct gpio_esp32_config gpio_data_##_id;			\
										\
	static struct gpio_esp32_config gpio_config_##_id = {			\
		.drv_cfg = {							\
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(_id),	\
		},								\
		.gpio_base = (gpio_dev_t *)DT_REG_ADDR(DT_NODELABEL(gpio0)),	\
		.gpio_dev = (gpio_dev_t *)DT_REG_ADDR(DT_NODELABEL(gpio##_id)),	\
		.gpio_port0 = _id ? false : true				\
	};									\
	DEVICE_DT_DEFINE(DT_NODELABEL(gpio##_id),				\
			&gpio_esp32_init,					\
			NULL,							\
			&gpio_data_##_id,					\
			&gpio_config_##_id,					\
			POST_KERNEL,						\
			CONFIG_KERNEL_INIT_PRIORITY_DEVICE,			\
			&gpio_esp32_driver_api)

/*
 * GPIOs are divided in two groups for ESP32 because the callback
 * API works with 32-bit bitmasks to manage interrupt callbacks,
 * and the device has 40 GPIO pins.
 */
#if defined(CONFIG_GPIO_ESP32_0)
ESP_SOC_GPIO_INIT(0);
#endif

#if defined(CONFIG_GPIO_ESP32_1)
ESP_SOC_GPIO_INIT(1);
#endif

static void IRAM_ATTR gpio_esp32_isr(void *param)
{

#if defined(CONFIG_GPIO_ESP32_0)
	gpio_esp32_fire_callbacks(DEVICE_DT_INST_GET(0));
#endif
#if defined(CONFIG_GPIO_ESP32_1)
	gpio_esp32_fire_callbacks(DEVICE_DT_INST_GET(1));
#endif

	ARG_UNUSED(param);
}
