/*
 * Copyright (c) 2021,2025 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT neorv32_gpio

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/syscon.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/logging/log.h>

#include <soc.h>

LOG_MODULE_REGISTER(gpio_neorv32, CONFIG_GPIO_LOG_LEVEL);

#include <zephyr/drivers/gpio/gpio_utils.h>

/* Register offsets */
#define NEORV32_GPIO_PORT_IN      0x00
#define NEORV32_GPIO_PORT_OUT     0x04
#define NEORV32_GPIO_IRQ_TYPE     0x10
#define NEORV32_GPIO_IRQ_POLARITY 0x14
#define NEORV32_GPIO_IRQ_ENABLE   0x18
#define NEORV32_GPIO_IRQ_PENDING  0x1c

struct neorv32_gpio_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	const struct device *syscon;
	mm_reg_t base;
	void (*irq_config_func)(void);
};

struct neorv32_gpio_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	/* Shadow register for output */
	uint32_t output;
	struct k_spinlock lock;
	sys_slist_t callbacks;
};

static inline uint32_t neorv32_gpio_read(const struct device *dev, uint16_t reg)
{
	const struct neorv32_gpio_config *config = dev->config;

	return sys_read32(config->base + reg);
}

static inline void neorv32_gpio_write(const struct device *dev, uint16_t reg, uint32_t val)
{
	const struct neorv32_gpio_config *config = dev->config;

	sys_write32(val, config->base + reg);
}

static int neorv32_gpio_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct neorv32_gpio_config *config = dev->config;
	struct neorv32_gpio_data *data = dev->data;
	k_spinlock_key_t key;

	if (!(BIT(pin) & config->common.port_pin_mask)) {
		return -EINVAL;
	}

	if ((flags & GPIO_SINGLE_ENDED) != 0) {
		return -ENOTSUP;
	}

	if ((flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) != 0) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_OUTPUT) != 0) {
		key = k_spin_lock(&data->lock);

		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
			data->output |= BIT(pin);
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
			data->output &= ~BIT(pin);
		}

		neorv32_gpio_write(dev, NEORV32_GPIO_PORT_OUT, data->output);
		k_spin_unlock(&data->lock, key);
	}

	return 0;
}

static int neorv32_gpio_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	*value = neorv32_gpio_read(dev, NEORV32_GPIO_PORT_IN);
	return 0;
}

static int neorv32_gpio_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
					    gpio_port_value_t value)
{
	struct neorv32_gpio_data *data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);
	data->output = (data->output & ~mask) | (mask & value);
	neorv32_gpio_write(dev, NEORV32_GPIO_PORT_OUT, data->output);
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int neorv32_gpio_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	struct neorv32_gpio_data *data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);
	data->output |= pins;
	neorv32_gpio_write(dev, NEORV32_GPIO_PORT_OUT, data->output);
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int neorv32_gpio_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	struct neorv32_gpio_data *data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);
	data->output &= ~pins;
	neorv32_gpio_write(dev, NEORV32_GPIO_PORT_OUT, data->output);
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int neorv32_gpio_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	struct neorv32_gpio_data *data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);
	data->output ^= pins;
	neorv32_gpio_write(dev, NEORV32_GPIO_PORT_OUT, data->output);
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int neorv32_gpio_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
						enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	const struct neorv32_gpio_config *config = dev->config;
	struct neorv32_gpio_data *data = dev->data;
	const uint32_t mask = BIT(pin);
	k_spinlock_key_t key;
	uint32_t polarity;
	uint32_t enable;
	uint32_t type;
	int err = 0;

	if (!(mask & config->common.port_pin_mask)) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	type = neorv32_gpio_read(dev, NEORV32_GPIO_IRQ_TYPE);
	polarity = neorv32_gpio_read(dev, NEORV32_GPIO_IRQ_POLARITY);
	enable = neorv32_gpio_read(dev, NEORV32_GPIO_IRQ_ENABLE);

	if (mode == GPIO_INT_MODE_DISABLED) {
		enable &= ~mask;
		neorv32_gpio_write(dev, NEORV32_GPIO_IRQ_ENABLE, enable);
		neorv32_gpio_write(dev, NEORV32_GPIO_IRQ_PENDING, ~mask);
	} else {
		enable |= mask;

		if (mode == GPIO_INT_MODE_LEVEL) {
			type &= ~mask;
		} else if (mode == GPIO_INT_MODE_EDGE) {
			type |= mask;
		} else {
			LOG_ERR("unsupported interrupt mode 0x%02x", mode);
			err = -ENOTSUP;
			goto unlock;
		}

		if (trig == GPIO_INT_TRIG_LOW) {
			polarity &= ~mask;
		} else if (trig == GPIO_INT_TRIG_HIGH) {
			polarity |= mask;
		} else {
			LOG_ERR("unsupported interrupt trig 0x%02x", trig);
			err = -ENOTSUP;
			goto unlock;
		}

		neorv32_gpio_write(dev, NEORV32_GPIO_IRQ_TYPE, type);
		neorv32_gpio_write(dev, NEORV32_GPIO_IRQ_POLARITY, polarity);

		neorv32_gpio_write(dev, NEORV32_GPIO_IRQ_PENDING, ~mask);
		neorv32_gpio_write(dev, NEORV32_GPIO_IRQ_ENABLE, enable);
	}

unlock:
	k_spin_unlock(&data->lock, key);

	return err;
}

static int neorv32_gpio_manage_callback(const struct device *dev, struct gpio_callback *cb,
					bool set)
{
	struct neorv32_gpio_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, cb, set);
}

static uint32_t neorv32_gpio_get_pending_int(const struct device *dev)
{
	return neorv32_gpio_read(dev, NEORV32_GPIO_IRQ_PENDING);
}

static void neorv32_gpio_isr(const struct device *dev)
{
	struct neorv32_gpio_data *data = dev->data;
	uint32_t pending;

	pending = neorv32_gpio_read(dev, NEORV32_GPIO_IRQ_PENDING);
	neorv32_gpio_write(dev, NEORV32_GPIO_IRQ_PENDING, ~(pending));

	gpio_fire_callbacks(&data->callbacks, dev, pending);
}

static int neorv32_gpio_init(const struct device *dev)
{
	const struct neorv32_gpio_config *config = dev->config;
	struct neorv32_gpio_data *data = dev->data;
	uint32_t features;
	int err;

	if (!device_is_ready(config->syscon)) {
		LOG_ERR("syscon device not ready");
		return -EINVAL;
	}

	err = syscon_read_reg(config->syscon, NEORV32_SYSINFO_SOC, &features);
	if (err < 0) {
		LOG_ERR("failed to determine implemented features (err %d)", err);
		return -EIO;
	}

	if ((features & NEORV32_SYSINFO_SOC_IO_GPIO) == 0) {
		LOG_ERR("neorv32 gpio not supported");
		return -ENODEV;
	}

	neorv32_gpio_write(dev, NEORV32_GPIO_PORT_OUT, data->output);

	config->irq_config_func();

	return 0;
}

static DEVICE_API(gpio, neorv32_gpio_driver_api) = {
	.pin_configure = neorv32_gpio_pin_configure,
	.port_get_raw = neorv32_gpio_port_get_raw,
	.port_set_masked_raw = neorv32_gpio_port_set_masked_raw,
	.port_set_bits_raw = neorv32_gpio_port_set_bits_raw,
	.port_clear_bits_raw = neorv32_gpio_port_clear_bits_raw,
	.port_toggle_bits = neorv32_gpio_port_toggle_bits,
	.pin_interrupt_configure = neorv32_gpio_pin_interrupt_configure,
	.manage_callback = neorv32_gpio_manage_callback,
	.get_pending_int = neorv32_gpio_get_pending_int,
};

#define NEORV32_GPIO_INIT(n)						\
	static void neorv32_gpio_config_func_##n(void)			\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),	\
			    neorv32_gpio_isr, DEVICE_DT_INST_GET(n), 0);\
		irq_enable(DT_INST_IRQN(n));				\
	}								\
									\
	static struct neorv32_gpio_data neorv32_gpio_##n##_data = {	\
		.output = 0,						\
	};								\
									\
	static const struct neorv32_gpio_config neorv32_gpio_##n##_config = { \
		.common = {						\
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n) \
		},							\
		.syscon = DEVICE_DT_GET(DT_INST_PHANDLE(n, syscon)),	\
		.base = DT_INST_REG_ADDR(n),				\
		.irq_config_func = neorv32_gpio_config_func_##n,	\
	};								\
									\
	DEVICE_DT_INST_DEFINE(n,					\
			neorv32_gpio_init,				\
			NULL,						\
			&neorv32_gpio_##n##_data,			\
			&neorv32_gpio_##n##_config,			\
			PRE_KERNEL_2,					\
			CONFIG_GPIO_INIT_PRIORITY,			\
			&neorv32_gpio_driver_api);

DT_INST_FOREACH_STATUS_OKAY(NEORV32_GPIO_INIT)
