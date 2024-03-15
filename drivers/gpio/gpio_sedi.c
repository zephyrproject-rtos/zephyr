/* Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT intel_sedi_gpio

#include "sedi_driver_gpio.h"
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/pm/device.h>

struct gpio_sedi_config {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_config common;
	sedi_gpio_t device;
	uint32_t pin_nums;
	void (*irq_config)(void);

	DEVICE_MMIO_ROM;
};

struct gpio_sedi_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_config common;
	sys_slist_t callbacks;

	DEVICE_MMIO_RAM;
};

static int gpio_sedi_init(const struct device *dev);

#ifdef CONFIG_PM_DEVICE
static int gpio_sedi_suspend_device(const struct device *dev)
{
	const struct gpio_sedi_config *config = dev->config;
	sedi_gpio_t gpio_dev = config->device;
	int ret;

	if (pm_device_is_busy(dev)) {
		return -EBUSY;
	}

	ret = sedi_gpio_set_power(gpio_dev, SEDI_POWER_SUSPEND);

	if (ret != SEDI_DRIVER_OK) {
		return -EIO;
	}

	return 0;
}

static int gpio_sedi_resume_device_from_suspend(const struct device *dev)
{
	const struct gpio_sedi_config *config = dev->config;
	sedi_gpio_t gpio_dev = config->device;
	int ret;

	ret = sedi_gpio_set_power(gpio_dev, SEDI_POWER_FULL);
	if (ret != SEDI_DRIVER_OK) {
		return -EIO;
	}

	return 0;
}

static int gpio_sedi_pm_action(const struct device *dev,
			      enum pm_device_action action)
{
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		ret = gpio_sedi_suspend_device(dev);
		break;
	case PM_DEVICE_ACTION_RESUME:
		ret = gpio_sedi_resume_device_from_suspend(dev);
		break;

	default:
		ret = -ENOTSUP;
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */

static void gpio_sedi_callback(const uint32_t pin_mask,
			       const sedi_gpio_port_t port,
			       void *param)
{
	ARG_UNUSED(port);
	struct device *dev = (struct device *)param;
	struct gpio_sedi_data *data =
		(struct gpio_sedi_data *)(dev->data);

	/* call the callbacks */
	gpio_fire_callbacks(&data->callbacks, dev, pin_mask);
}

static void gpio_sedi_write_raw(const struct device *dev,
				uint32_t pins,
				bool is_clear)
{
	uint8_t i;
	const struct gpio_sedi_config *config = dev->config;
	sedi_gpio_t gpio_dev = config->device;
	sedi_gpio_pin_state_t val;

	if (is_clear) {
		val = SEDI_GPIO_STATE_LOW;
	} else {
		val = SEDI_GPIO_STATE_HIGH;
	}

	for (i = 0; i < config->pin_nums; i++) {
		if (pins & 0x1) {
			sedi_gpio_write_pin(gpio_dev, i, val);
		}
		pins >>= 1;
		if (pins == 0) {
			break;
		}
	}
}

static int gpio_sedi_configure(const struct device *dev, gpio_pin_t pin,
			       gpio_flags_t flags)
{
	const struct gpio_sedi_config *config = dev->config;
	sedi_gpio_t gpio_dev = config->device;
	sedi_gpio_pin_config_t pin_config = { 0 };

	if ((flags & GPIO_OUTPUT) && (flags & GPIO_INPUT)) {
		/* Pin cannot be configured as input and output */
		return -ENOTSUP;
	} else if (!(flags & (GPIO_INPUT | GPIO_OUTPUT))) {
		/* Pin has to be configured as input or output */
		return -ENOTSUP;
	}

	pin_config.enable_interrupt = false;
	/* Map direction */
	if (flags & GPIO_OUTPUT) {
		pin_config.direction = SEDI_GPIO_DIR_MODE_OUTPUT;
		sedi_gpio_config_pin(gpio_dev, pin, pin_config);
		/* Set start state */
		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
			sedi_gpio_write_pin(gpio_dev, pin, 1);
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
			sedi_gpio_write_pin(gpio_dev, pin, 0);
		}
	} else {
		pin_config.direction = SEDI_GPIO_DIR_MODE_INPUT;
		sedi_gpio_config_pin(gpio_dev, pin, pin_config);
	}

	return 0;
}

static int gpio_sedi_get_raw(const struct device *dev, uint32_t *value)
{
	const struct gpio_sedi_config *config = dev->config;
	sedi_gpio_t gpio_dev = config->device;

	*value = sedi_gpio_read_pin_32bits(gpio_dev, 0);

	return 0;
}

static int gpio_sedi_set_masked_raw(const struct device *dev,
				    uint32_t mask,
				    uint32_t value)
{
	gpio_sedi_write_raw(dev, (mask & value), false);

	return 0;
}

static int gpio_sedi_set_bits_raw(const struct device *dev, uint32_t pins)
{
	gpio_sedi_write_raw(dev, pins, false);

	return 0;
}

static int gpio_sedi_clear_bits_raw(const struct device *dev, uint32_t pins)
{
	gpio_sedi_write_raw(dev, pins, true);

	return 0;
}

static int gpio_sedi_toggle_bits(const struct device *dev, uint32_t pins)
{
	const struct gpio_sedi_config *config = dev->config;
	sedi_gpio_t gpio_dev = config->device;
	uint8_t i;

	for (i = 0; i < config->pin_nums; i++) {
		if (pins & 0x1) {
			sedi_gpio_toggle_pin(gpio_dev, i);
		}
		pins >>= 1;
		if (pins == 0) {
			break;
		}
	}

	return 0;
}

static int gpio_sedi_interrupt_configure(const struct device *dev,
					 gpio_pin_t pin,
					 enum gpio_int_mode mode,
					 enum gpio_int_trig trig)
{
	const struct gpio_sedi_config *config = dev->config;
	sedi_gpio_t gpio_dev = config->device;
	sedi_gpio_pin_config_t pin_config = { 0 };

	/* Not support level trigger */
	if (mode == GPIO_INT_MODE_LEVEL) {
		return -EINVAL;
	}
	/* Only input needs interrupt enabled */
	pin_config.direction = SEDI_GPIO_DIR_MODE_INPUT;
	pin_config.enable_wakeup = true;
	if (mode == GPIO_INT_MODE_DISABLED) {
		pin_config.enable_interrupt = false;
	} else {
		pin_config.enable_interrupt = true;
		switch (trig) {
		case GPIO_INT_TRIG_LOW:
			pin_config.interrupt_mode =
				SEDI_GPIO_INT_MODE_FALLING_EDGE;
			break;
		case GPIO_INT_TRIG_HIGH:
			pin_config.interrupt_mode =
				SEDI_GPIO_INT_MODE_RISING_EDGE;
			break;
		case GPIO_INT_TRIG_BOTH:
			pin_config.interrupt_mode =
				SEDI_GPIO_INT_MODE_BOTH_EDGE;
			break;
		default:
			return -EINVAL;
		}
	}
	/* Configure interrupt mode */
	sedi_gpio_config_pin(gpio_dev, pin, pin_config);

	return 0;
}

static int gpio_sedi_manage_callback(const struct device *dev,
				     struct gpio_callback *callback,
				     bool set)
{
	struct gpio_sedi_data *data = dev->data;

	gpio_manage_callback(&(data->callbacks), callback, set);

	return 0;
}

static uint32_t gpio_sedi_get_pending(const struct device *dev)
{
	const struct gpio_sedi_config *config = dev->config;
	sedi_gpio_t gpio_dev = config->device;

	return sedi_gpio_get_gisr(gpio_dev, 0);
}

static const struct gpio_driver_api gpio_sedi_driver_api = {
	.pin_configure = gpio_sedi_configure,
	.port_get_raw = gpio_sedi_get_raw,
	.port_set_masked_raw = gpio_sedi_set_masked_raw,
	.port_set_bits_raw = gpio_sedi_set_bits_raw,
	.port_clear_bits_raw = gpio_sedi_clear_bits_raw,
	.port_toggle_bits = gpio_sedi_toggle_bits,
	.pin_interrupt_configure = gpio_sedi_interrupt_configure,
	.manage_callback = gpio_sedi_manage_callback,
	.get_pending_int = gpio_sedi_get_pending
};

extern void gpio_isr(IN sedi_gpio_t gpio_device);

static int gpio_sedi_init(const struct device *dev)
{
	int ret;
	const struct gpio_sedi_config *config = dev->config;
	sedi_gpio_t gpio_dev = config->device;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	/* Call sedi gpio init */
	ret = sedi_gpio_init(gpio_dev, gpio_sedi_callback, (void *)dev);

	if (ret != 0) {
		return ret;
	}
	sedi_gpio_set_power(gpio_dev, SEDI_POWER_FULL);

	config->irq_config();

	return 0;
}

#define GPIO_SEDI_IRQ_FLAGS_SENSE0(n) 0
#define GPIO_SEDI_IRQ_FLAGS_SENSE1(n) DT_INST_IRQ(n, sense)
#define GPIO_SEDI_IRQ_FLAGS(n) \
	_CONCAT(GPIO_SEDI_IRQ_FLAGS_SENSE, DT_INST_IRQ_HAS_CELL(n, sense))(n)

#define GPIO_DEVICE_INIT_SEDI(n)				       \
	static struct gpio_sedi_data gpio##n##_data;	               \
	static void gpio_sedi_irq_config_##n(void)		       \
	{							       \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), \
			    gpio_isr, n,			       \
			    GPIO_SEDI_IRQ_FLAGS(n));		       \
		irq_enable(DT_INST_IRQN(n));			       \
	};							       \
	static const struct gpio_sedi_config gpio##n##_config = {      \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                  \
		.common = { 0xFFFFFFFF },			       \
		.device = DT_INST_PROP(n, peripheral_id),              \
		.pin_nums = DT_INST_PROP(n, ngpios),                   \
		.irq_config = gpio_sedi_irq_config_##n,	               \
	};							       \
	PM_DEVICE_DEFINE(gpio_##n, gpio_sedi_pm_action);               \
	DEVICE_DT_INST_DEFINE(n,				       \
		      &gpio_sedi_init,				       \
		      PM_DEVICE_GET(gpio_##n),		               \
		      &gpio##n##_data,			               \
		      &gpio##n##_config,			       \
		      POST_KERNEL,				       \
		      CONFIG_GPIO_INIT_PRIORITY,	               \
		      &gpio_sedi_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_DEVICE_INIT_SEDI)
