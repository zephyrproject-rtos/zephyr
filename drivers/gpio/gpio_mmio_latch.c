/*
 * Copyright (c) 2026 Zhaoming Li
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief GPIO driver for a memory-mapped output latch register
 *
 * This driver exposes a simple memory-mapped latch register through the
 * Zephyr GPIO API. The latch is treated as an output-only GPIO controller
 * and the driver keeps a software shadow of the last value written.
 */

#define DT_DRV_COMPAT gpio_mmio_latch

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

struct mmio_latch_gpio_config {
	struct gpio_driver_config common;
	uintptr_t base;
	uint8_t reg_size;
	gpio_port_value_t initial_output;
};

struct mmio_latch_gpio_data {
	struct gpio_driver_data common;
	struct k_spinlock lock;
	gpio_port_value_t shadow;
};

#define MMIO_LATCH_SUPPORTED_FLAGS                                                 \
	(GPIO_OUTPUT | GPIO_OUTPUT_INIT_LOW | GPIO_OUTPUT_INIT_HIGH |               \
	 GPIO_ACTIVE_LOW)

static bool mmio_latch_gpio_pin_valid(const struct mmio_latch_gpio_config *cfg,
				      gpio_pin_t pin)
{
	if (pin >= GPIO_MAX_PINS_PER_PORT) {
		return false;
	}

	return (BIT(pin) & cfg->common.port_pin_mask) != 0U;
}

static void mmio_latch_gpio_hw_write(const struct mmio_latch_gpio_config *cfg,
				     gpio_port_value_t value)
{
	switch (cfg->reg_size) {
	case 1:
		sys_write8((uint8_t)value, cfg->base);
		break;
	case 2:
		sys_write16((uint16_t)value, cfg->base);
		break;
	case 4:
		sys_write32((uint32_t)value, cfg->base);
		break;
	default:
		break;
	}
}

static void mmio_latch_gpio_apply_shadow(const struct device *dev,
					 gpio_port_value_t value)
{
	const struct mmio_latch_gpio_config *cfg = dev->config;
	struct mmio_latch_gpio_data *data = dev->data;

	/* Avoid redundant MMIO writes when the output state is unchanged. */
	if (data->shadow == value) {
		return;
	}

	data->shadow = value;
	mmio_latch_gpio_hw_write(cfg, value);
}

static int mmio_latch_gpio_pin_configure(const struct device *dev,
					 gpio_pin_t pin,
					 gpio_flags_t flags)
{
	const struct mmio_latch_gpio_config *cfg = dev->config;
	struct mmio_latch_gpio_data *data = dev->data;
	k_spinlock_key_t key;

	if (!mmio_latch_gpio_pin_valid(cfg, pin)) {
		return -EINVAL;
	}

	if ((flags & ~MMIO_LATCH_SUPPORTED_FLAGS) != 0U) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_OUTPUT) == 0U) {
		return -ENOTSUP;
	}

	key = k_spin_lock(&data->lock);

	if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0U) {
		mmio_latch_gpio_apply_shadow(dev, data->shadow | BIT(pin));
	} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0U) {
		mmio_latch_gpio_apply_shadow(dev, data->shadow & ~BIT(pin));
	} else {
		/* Keep the current shadow state when no init level is requested. */
	}

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int mmio_latch_gpio_port_get_raw(const struct device *dev,
					gpio_port_value_t *value)
{
	struct mmio_latch_gpio_data *data = dev->data;
	k_spinlock_key_t key;

	/* Read back the software shadow because the latch may be write-only. */
	key = k_spin_lock(&data->lock);
	*value = data->shadow;
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int mmio_latch_gpio_port_set_masked_raw(const struct device *dev,
					       gpio_port_pins_t mask,
					       gpio_port_value_t value)
{
	const struct mmio_latch_gpio_config *cfg = dev->config;
	struct mmio_latch_gpio_data *data = dev->data;
	k_spinlock_key_t key;
	gpio_port_value_t new_value;

	mask &= cfg->common.port_pin_mask;

	key = k_spin_lock(&data->lock);
	new_value = (data->shadow & ~mask) | (value & mask);
	mmio_latch_gpio_apply_shadow(dev, new_value);
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int mmio_latch_gpio_port_set_bits_raw(const struct device *dev,
					     gpio_port_pins_t pins)
{
	return mmio_latch_gpio_port_set_masked_raw(dev, pins, pins);
}

static int mmio_latch_gpio_port_clear_bits_raw(const struct device *dev,
					       gpio_port_pins_t pins)
{
	return mmio_latch_gpio_port_set_masked_raw(dev, pins, 0U);
}

static int mmio_latch_gpio_port_toggle_bits(const struct device *dev,
					    gpio_port_pins_t pins)
{
	const struct mmio_latch_gpio_config *cfg = dev->config;
	struct mmio_latch_gpio_data *data = dev->data;
	k_spinlock_key_t key;
	gpio_port_value_t new_value;

	pins &= cfg->common.port_pin_mask;

	key = k_spin_lock(&data->lock);
	new_value = data->shadow ^ pins;
	mmio_latch_gpio_apply_shadow(dev, new_value);
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int mmio_latch_gpio_init(const struct device *dev)
{
	const struct mmio_latch_gpio_config *cfg = dev->config;
	struct mmio_latch_gpio_data *data = dev->data;
	gpio_port_value_t initial_value = cfg->initial_output;

	/* A latch may power up in an unknown state, so always write init value. */
	mmio_latch_gpio_hw_write(cfg, initial_value);
	data->shadow = initial_value;

	return 0;
}

static DEVICE_API(gpio, mmio_latch_gpio_driver_api) = {
	.pin_configure = mmio_latch_gpio_pin_configure,
	.port_get_raw = mmio_latch_gpio_port_get_raw,
	.port_set_masked_raw = mmio_latch_gpio_port_set_masked_raw,
	.port_set_bits_raw = mmio_latch_gpio_port_set_bits_raw,
	.port_clear_bits_raw = mmio_latch_gpio_port_clear_bits_raw,
	.port_toggle_bits = mmio_latch_gpio_port_toggle_bits,
};

#define MMIO_LATCH_DT_PORT_MASK(inst) GPIO_PORT_PIN_MASK_FROM_DT_INST(inst)

#define MMIO_LATCH_DT_INITIAL_OUTPUT(inst) DT_INST_PROP_OR(inst, initial_output, 0)

#define MMIO_LATCH_GPIO_INIT(inst)                                                          \
	BUILD_ASSERT((DT_INST_REG_SIZE(inst) == 1) || (DT_INST_REG_SIZE(inst) == 2) ||     \
			     (DT_INST_REG_SIZE(inst) == 4),                               \
		     "gpio-mmio-latch reg size must be 1, 2, or 4 bytes");                \
	BUILD_ASSERT(DT_INST_PROP(inst, ngpios) <= (DT_INST_REG_SIZE(inst) * 8),            \
		     "ngpios exceeds register width");                                   \
	BUILD_ASSERT((MMIO_LATCH_DT_INITIAL_OUTPUT(inst) &                                \
		      ~(gpio_port_value_t)MMIO_LATCH_DT_PORT_MASK(inst)) == 0,         \
		     "initial-output sets bits outside valid GPIO mask");               \
                                                                                           \
	static struct mmio_latch_gpio_data mmio_latch_gpio_data_##inst;                   \
                                                                                           \
	static const struct mmio_latch_gpio_config mmio_latch_gpio_config_##inst = {      \
		.common = GPIO_COMMON_CONFIG_FROM_DT_INST(inst),                          \
		.base = DT_INST_REG_ADDR(inst),                                           \
		.reg_size = DT_INST_REG_SIZE(inst),                                       \
		.initial_output = (gpio_port_value_t)(                                    \
			MMIO_LATCH_DT_INITIAL_OUTPUT(inst) &                               \
			MMIO_LATCH_DT_PORT_MASK(inst)),                                    \
	};                                                                                 \
                                                                                           \
	DEVICE_DT_INST_DEFINE(inst,                                                        \
			      mmio_latch_gpio_init,                                        \
			      NULL,                                                        \
			      &mmio_latch_gpio_data_##inst,                               \
			      &mmio_latch_gpio_config_##inst,                             \
			      POST_KERNEL,                                                 \
			      CONFIG_GPIO_INIT_PRIORITY,                                  \
			      &mmio_latch_gpio_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MMIO_LATCH_GPIO_INIT)
