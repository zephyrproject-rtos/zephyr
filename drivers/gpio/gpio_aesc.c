/*
 * Copyright (c) 2025 Aesc Silicon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT aesc_gpio

#include <errno.h>
#include <ip_identification.h>
#include <soc.h>

#include <zephyr/arch/common/sys_bitops.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/spinlock.h>


#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(aesc_gpio, CONFIG_GPIO_LOG_LEVEL);

struct gpio_aesc_config {
	DEVICE_MMIO_ROM;
};

struct gpio_aesc_regs {
	uint32_t info;
	uint32_t read;
	uint32_t write;
	uint32_t direction;
	uint32_t high_ip;
	uint32_t high_ie;
	uint32_t low_ip;
	uint32_t low_ie;
	uint32_t rise_ip;
	uint32_t rise_ie;
	uint32_t fall_ip;
	uint32_t fall_ie;
} __packed;

struct gpio_aesc_data {
	DEVICE_MMIO_RAM;
	sys_slist_t cb;
	struct k_spinlock lock;
};

#define DEV_CFG(dev) ((struct gpio_aesc_config *)(dev)->config)
#define DEV_DATA(dev) ((struct gpio_aesc_data *)(dev)->data)
#define DEV_GPIO(dev) ((struct gpio_aesc_regs *)DEVICE_MMIO_GET(dev))

static int gpio_aesc_config(const struct device *dev, gpio_pin_t pin,
			    gpio_flags_t flags)
{
	volatile struct gpio_aesc_regs *gpio = DEV_GPIO(dev);
	struct gpio_aesc_data *data = DEV_DATA(dev);
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);
	/* Configure gpio direction */
	if (flags & GPIO_OUTPUT) {
		gpio->direction |= BIT(pin);
	} else {
		gpio->direction &= ~BIT(pin);
	}
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int gpio_aesc_port_get_raw(const struct device *dev,
				  gpio_port_value_t *value)
{
	volatile struct gpio_aesc_regs *gpio = DEV_GPIO(dev);

	*value = gpio->read;

	return 0;
}

static int gpio_aesc_port_set_masked_raw(const struct device *dev,
					 gpio_port_pins_t mask,
					 gpio_port_value_t value)
{
	volatile struct gpio_aesc_regs *gpio = DEV_GPIO(dev);
	struct gpio_aesc_data *data = DEV_DATA(dev);
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);
	gpio->write = (gpio->write & ~mask) | (value & mask);
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int gpio_aesc_port_set_bits_raw(const struct device *dev,
				       gpio_port_pins_t mask)
{
	volatile struct gpio_aesc_regs *gpio = DEV_GPIO(dev);
	struct gpio_aesc_data *data = DEV_DATA(dev);
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);
	gpio->write |= mask;
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int gpio_aesc_port_clear_bits_raw(const struct device *dev,
					 gpio_port_pins_t mask)
{
	volatile struct gpio_aesc_regs *gpio = DEV_GPIO(dev);
	struct gpio_aesc_data *data = DEV_DATA(dev);
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);
	gpio->write &= ~mask;
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int gpio_aesc_port_toggle_bits(const struct device *dev,
				      gpio_port_pins_t mask)
{
	volatile struct gpio_aesc_regs *gpio = DEV_GPIO(dev);
	struct gpio_aesc_data *data = DEV_DATA(dev);
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);
	gpio->write ^= mask;
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int gpio_aesc_init(const struct device *dev)
{
	volatile uintptr_t *base_addr = (volatile uintptr_t *)DEV_GPIO(dev);
	volatile struct gpio_aesc_regs *gpio;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
	LOG_DBG("IP core version: %i.%i.%i.",
		ip_id_get_major_version(base_addr),
		ip_id_get_minor_version(base_addr),
		ip_id_get_patchlevel(base_addr)
	);
	DEVICE_MMIO_GET(dev) = ip_id_relocate_driver(base_addr);
	LOG_DBG("Relocate driver to address 0x%lx.", DEVICE_MMIO_GET(dev));
	gpio = DEV_GPIO(dev);

	gpio->high_ie = 0;
	gpio->low_ie = 0;
	gpio->rise_ie = 0;
	gpio->fall_ie = 0;

	return 0;
}

static DEVICE_API(gpio, gpio_aesc_driver_api) = {
	.pin_configure = gpio_aesc_config,
	.port_get_raw = gpio_aesc_port_get_raw,
	.port_set_masked_raw = gpio_aesc_port_set_masked_raw,
	.port_set_bits_raw = gpio_aesc_port_set_bits_raw,
	.port_clear_bits_raw = gpio_aesc_port_clear_bits_raw,
	.port_toggle_bits = gpio_aesc_port_toggle_bits,
};

#define AESC_GPIO_INIT(no)						      \
	static struct gpio_aesc_data gpio_aesc_dev_data_##no;		      \
	static struct gpio_aesc_config gpio_aesc_dev_cfg_##no = {	      \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(no)),			      \
	};								      \
	DEVICE_DT_INST_DEFINE(no,					      \
			      gpio_aesc_init,				      \
			      NULL,					      \
			      &gpio_aesc_dev_data_##no,			      \
			      &gpio_aesc_dev_cfg_##no,			      \
			      PRE_KERNEL_2,				      \
			      CONFIG_GPIO_INIT_PRIORITY,		      \
			      (void *)&gpio_aesc_driver_api);

DT_INST_FOREACH_STATUS_OKAY(AESC_GPIO_INIT)
