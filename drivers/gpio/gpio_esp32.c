/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2021 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_gpio

/* Include esp-idf headers first to avoid redefining BIT() macro */
#include <soc/gpio_reg.h>
#include <soc/io_mux_reg.h>
#include <soc/soc.h>
#include <hal/gpio_ll.h>
#include <esp_attr.h>

#include <soc.h>
#include <errno.h>
#include <device.h>
#include <drivers/gpio.h>
#ifdef CONFIG_SOC_ESP32C3
#include <drivers/interrupt_controller/intc_esp32c3.h>
#else
#include <drivers/interrupt_controller/intc_esp32.h>
#endif
#include <kernel.h>
#include <sys/util.h>
#include <drivers/pinmux.h>

#include "gpio_utils.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(gpio_esp32, CONFIG_LOG_DEFAULT_LEVEL);

#define DEV_CFG(_dev) ((struct gpio_esp32_config *const)(_dev)->config)

#ifdef CONFIG_SOC_ESP32C3
/* gpio structs in esp32c3 series are different from xtensa ones */
#define out out.data
#define in in.data
#define out_w1ts out_w1ts.val
#define out_w1tc out_w1tc.val
/* arch_curr_cpu() is not available for riscv based chips */
#define CPU_ID()  0
#define ISR_HANDLER isr_handler_t
#else
#define CPU_ID() arch_curr_cpu()->id
#define ISR_HANDLER intr_handler_t
#endif

struct gpio_esp32_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config drv_cfg;
	gpio_dev_t *const gpio_base;
	gpio_dev_t *const gpio_dev;
	const int gpio_port;
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

	return ((cfg->gpio_port) ? 32 : 0);
}

static int gpio_esp32_config(const struct device *dev,
			     gpio_pin_t pin,
			     gpio_flags_t flags)
{
	struct gpio_esp32_config *const cfg = DEV_CFG(dev);
	struct gpio_esp32_data *data = dev->data;
	uint32_t io_pin = pin + gpio_get_pin_offset(dev);
	uint32_t key;
	int ret = 0;

	if (!gpio_pin_is_valid(io_pin)) {
		LOG_ERR("Selected IO pin is not valid.");
		return -EINVAL;
	}

	key = irq_lock();

	/* Set pin function as GPIO */
	ret = pinmux_pin_set(data->pinmux, io_pin, PIN_FUNC_GPIO);
	if (ret < 0) {
		LOG_ERR("Invalid pinmux configuration.");
		goto end;
	}

	if (flags & GPIO_PULL_UP) {
		ret = pinmux_pin_pullup(data->pinmux, io_pin, PINMUX_PULLUP_ENABLE);
	} else if (flags & GPIO_PULL_DOWN) {
		ret = pinmux_pin_pullup(data->pinmux, io_pin, PINMUX_PULLUP_DISABLE);
	}

	if (ret < 0) {
		LOG_ERR("Invalid pinmux configuration.");
		goto end;
	}

	if (flags & GPIO_OUTPUT) {

		if (!gpio_pin_is_output_capable(pin)) {
			LOG_ERR("GPIO can only be used as input");
			ret = -EINVAL;
			goto end;
		}

		if (flags & GPIO_SINGLE_ENDED) {
			if (flags & GPIO_LINE_OPEN_DRAIN) {
				gpio_ll_od_enable(cfg->gpio_base, io_pin);
			} else {
				LOG_ERR("GPIO configuration not supported");
				ret = -ENOTSUP;
				goto end;
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
			ret = -EINVAL;
			goto end;
		}

		/* Set output pin initial value */
		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			gpio_ll_set_level(cfg->gpio_base, io_pin, 1);
		} else if (flags & GPIO_OUTPUT_INIT_LOW) {
			gpio_ll_set_level(cfg->gpio_base, io_pin, 0);
		}

		ret = pinmux_pin_input_enable(data->pinmux, io_pin, PINMUX_OUTPUT_ENABLED);
		if (ret < 0) {
			goto end;
		}
	}

	if (flags & GPIO_INPUT) {
		ret = pinmux_pin_input_enable(data->pinmux, io_pin, PINMUX_INPUT_ENABLED);
	}

end:
	irq_unlock(key);

	return ret;
}

static int gpio_esp32_port_get_raw(const struct device *port, uint32_t *value)
{
	struct gpio_esp32_config *const cfg = DEV_CFG(port);

	if (cfg->gpio_port == 0) {
		*value = cfg->gpio_dev->in;
#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio1), okay)
	} else {
		*value = cfg->gpio_dev->in1.data;
#endif
	}

	return 0;
}

static int gpio_esp32_port_set_masked_raw(const struct device *port,
					  uint32_t mask, uint32_t value)
{
	struct gpio_esp32_config *const cfg = DEV_CFG(port);

	uint32_t key = irq_lock();

	if (cfg->gpio_port == 0) {
		cfg->gpio_dev->out = (cfg->gpio_dev->out & ~mask) | (mask & value);
#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio1), okay)
	} else {
		cfg->gpio_dev->out1.data = (cfg->gpio_dev->out1.data & ~mask) | (mask & value);
#endif
	}

	irq_unlock(key);

	return 0;
}

static int gpio_esp32_port_set_bits_raw(const struct device *port,
					uint32_t pins)
{
	struct gpio_esp32_config *const cfg = DEV_CFG(port);

	if (cfg->gpio_port == 0) {
		cfg->gpio_dev->out_w1ts = pins;
#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio1), okay)
	} else {
		cfg->gpio_dev->out1_w1ts.data = pins;
#endif
	}

	return 0;
}

static int gpio_esp32_port_clear_bits_raw(const struct device *port,
					  uint32_t pins)
{
	struct gpio_esp32_config *const cfg = DEV_CFG(port);

	if (cfg->gpio_port == 0) {
		cfg->gpio_dev->out_w1tc = pins;
#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio1), okay)
	} else {
		cfg->gpio_dev->out1_w1tc.data = pins;
#endif
	}

	return 0;
}

static int gpio_esp32_port_toggle_bits(const struct device *port,
				       uint32_t pins)
{
	struct gpio_esp32_config *const cfg = DEV_CFG(port);
	uint32_t key = irq_lock();

	if (cfg->gpio_port == 0) {
		cfg->gpio_dev->out ^= pins;
#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio1), okay)
	} else {
		cfg->gpio_dev->out1.data ^= pins;
#endif
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
	gpio_ll_intr_enable_on_core(cfg->gpio_base, CPU_ID(), io_pin);
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
	uint32_t const core_id = CPU_ID();

	if (cfg->gpio_port == 0) {
		gpio_ll_get_intr_status(cfg->gpio_base, core_id, &irq_status);
	} else {
		gpio_ll_get_intr_status_high(cfg->gpio_base, core_id, &irq_status);
	}

	return irq_status;
}

static void IRAM_ATTR gpio_esp32_fire_callbacks(const struct device *dev)
{
	struct gpio_esp32_config *const cfg = DEV_CFG(dev);
	struct gpio_esp32_data *data = dev->data;
	uint32_t irq_status;
	uint32_t const core_id = CPU_ID();

	if (cfg->gpio_port == 0) {
		gpio_ll_get_intr_status(cfg->gpio_base, core_id, &irq_status);
		gpio_ll_clear_intr_status(cfg->gpio_base, irq_status);
	} else {
		gpio_ll_get_intr_status_high(cfg->gpio_base, core_id, &irq_status);
		gpio_ll_clear_intr_status_high(cfg->gpio_base, irq_status);
	}

	if (irq_status != 0) {
		gpio_fire_callbacks(&data->cb, dev, irq_status);
	}
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
		esp_intr_alloc(DT_IRQN(DT_NODELABEL(gpio0)),
			0,
			(ISR_HANDLER)gpio_esp32_isr,
			(void *)dev,
			NULL);

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
	static struct gpio_esp32_data gpio_data_##_id;	\
	static struct gpio_esp32_config gpio_config_##_id = {			\
		.drv_cfg = {							\
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(_id),	\
		},								\
		.gpio_base = (gpio_dev_t *)DT_REG_ADDR(DT_NODELABEL(gpio0)),	\
		.gpio_dev = (gpio_dev_t *)DT_REG_ADDR(DT_NODELABEL(gpio##_id)),	\
		.gpio_port = _id	\
	};									\
	DEVICE_DT_DEFINE(DT_NODELABEL(gpio##_id),				\
			&gpio_esp32_init,					\
			NULL,							\
			&gpio_data_##_id,					\
			&gpio_config_##_id,					\
			PRE_KERNEL_2,						\
			CONFIG_KERNEL_INIT_PRIORITY_DEVICE,			\
			&gpio_esp32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ESP_SOC_GPIO_INIT);

static void IRAM_ATTR gpio_esp32_isr(void *param)
{
	ARG_UNUSED(param);

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio0), okay)
	gpio_esp32_fire_callbacks(DEVICE_DT_INST_GET(0));
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio1), okay)
	gpio_esp32_fire_callbacks(DEVICE_DT_INST_GET(1));
#endif
}
