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
#include <zephyr/sys/sys_io.h>
#include <zephyr/logging/log.h>

#include <soc.h>

LOG_MODULE_REGISTER(gpio_neorv32, CONFIG_GPIO_LOG_LEVEL);

#include <zephyr/drivers/gpio/gpio_utils.h>

/* Register offsets */
#define NEORV32_GPIO_PORT_IN  0x00
#define NEORV32_GPIO_PORT_OUT 0x04

/* Maximum number of GPIOs supported */
#define MAX_GPIOS 32

struct neorv32_gpio_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	const struct device *syscon;
	mm_reg_t base;
};

struct neorv32_gpio_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	/* Shadow register for output */
	uint32_t output;
};

static inline uint32_t neorv32_gpio_read(const struct device *dev)
{
	const struct neorv32_gpio_config *config = dev->config;

	return sys_read32(config->base + NEORV32_GPIO_PORT_IN);
}

static inline void neorv32_gpio_write(const struct device *dev, uint32_t val)
{
	const struct neorv32_gpio_config *config = dev->config;

	sys_write32(val, config->base + NEORV32_GPIO_PORT_OUT);
}

static int neorv32_gpio_pin_configure(const struct device *dev, gpio_pin_t pin,
				      gpio_flags_t flags)
{
	const struct neorv32_gpio_config *config = dev->config;
	struct neorv32_gpio_data *data = dev->data;
	unsigned int key;

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
		key = irq_lock();

		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
			data->output |= BIT(pin);
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
			data->output &= ~BIT(pin);
		}

		neorv32_gpio_write(dev, data->output);
		irq_unlock(key);
	}

	return 0;
}

static int neorv32_gpio_port_get_raw(const struct device *dev,
				      gpio_port_value_t *value)
{
	*value = neorv32_gpio_read(dev);
	return 0;
}

static int neorv32_gpio_port_set_masked_raw(const struct device *dev,
					     gpio_port_pins_t mask,
					     gpio_port_value_t value)
{
	struct neorv32_gpio_data *data = dev->data;
	unsigned int key;

	key = irq_lock();
	data->output = (data->output & ~mask) | (mask & value);
	neorv32_gpio_write(dev, data->output);
	irq_unlock(key);

	return 0;
}

static int neorv32_gpio_port_set_bits_raw(const struct device *dev,
					   gpio_port_pins_t pins)
{
	struct neorv32_gpio_data *data = dev->data;
	unsigned int key;

	key = irq_lock();
	data->output |= pins;
	neorv32_gpio_write(dev, data->output);
	irq_unlock(key);

	return 0;
}

static int neorv32_gpio_port_clear_bits_raw(const struct device *dev,
					     gpio_port_pins_t pins)
{
	struct neorv32_gpio_data *data = dev->data;
	unsigned int key;

	key = irq_lock();
	data->output &= ~pins;
	neorv32_gpio_write(dev, data->output);
	irq_unlock(key);

	return 0;
}

static int neorv32_gpio_port_toggle_bits(const struct device *dev,
					  gpio_port_pins_t pins)
{
	struct neorv32_gpio_data *data = dev->data;
	unsigned int key;

	key = irq_lock();
	data->output ^= pins;
	neorv32_gpio_write(dev, data->output);
	irq_unlock(key);

	return 0;
}

static int neorv32_gpio_manage_callback(const struct device *dev,
					struct gpio_callback *cb,
					bool set)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	ARG_UNUSED(set);

	return -ENOTSUP;
}

static uint32_t neorv32_gpio_get_pending_int(const struct device *dev)
{
	return 0;
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

	neorv32_gpio_write(dev, data->output);

	return 0;
}

static DEVICE_API(gpio, neorv32_gpio_driver_api) = {
	.pin_configure = neorv32_gpio_pin_configure,
	.port_get_raw = neorv32_gpio_port_get_raw,
	.port_set_masked_raw = neorv32_gpio_port_set_masked_raw,
	.port_set_bits_raw = neorv32_gpio_port_set_bits_raw,
	.port_clear_bits_raw = neorv32_gpio_port_clear_bits_raw,
	.port_toggle_bits = neorv32_gpio_port_toggle_bits,
	.manage_callback = neorv32_gpio_manage_callback,
	.get_pending_int = neorv32_gpio_get_pending_int,
};

#define NEORV32_GPIO_INIT(n)						\
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
