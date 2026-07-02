/*
 * SPDX-FileCopyrightText: 2025 Aesc Silicon
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
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>


#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(aesc_gpio, CONFIG_GPIO_LOG_LEVEL);

struct gpio_aesc_config {
	struct gpio_driver_config common;

	DEVICE_MMIO_NAMED_ROM(mmio);

	const struct pinctrl_dev_config *pcfg;
	void (*irq_config)(const struct device *dev);
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
	struct gpio_driver_data common;

	DEVICE_MMIO_NAMED_RAM(mmio);

	uintptr_t reg_base;
	sys_slist_t cb;
	struct k_spinlock lock;
};

#define DEV_CFG(dev) ((const struct gpio_aesc_config * const)(dev)->config)
#define DEV_DATA(dev) ((struct gpio_aesc_data *)(dev)->data)
#define DEV_GPIO(dev)							      \
	((volatile struct gpio_aesc_regs *)DEV_DATA(dev)->reg_base)

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

static int gpio_aesc_pin_interrupt_configure(const struct device *dev,
					     gpio_pin_t pin,
					     enum gpio_int_mode mode,
					     enum gpio_int_trig trig)
{
	volatile struct gpio_aesc_regs *gpio = DEV_GPIO(dev);

	gpio->rise_ie &= ~BIT(pin);
	gpio->fall_ie &= ~BIT(pin);
	gpio->high_ie &= ~BIT(pin);
	gpio->low_ie  &= ~BIT(pin);

	switch (mode) {
	case GPIO_INT_MODE_DISABLED:
		break;
	case GPIO_INT_MODE_LEVEL:
		if (trig == GPIO_INT_TRIG_HIGH) {
			gpio->high_ip = BIT(pin);
			gpio->high_ie |= BIT(pin);
		}
		if (trig == GPIO_INT_TRIG_LOW) {
			gpio->low_ip = BIT(pin);
			gpio->low_ie  |= BIT(pin);
		}
		break;
	case GPIO_INT_MODE_EDGE:
		if ((trig & GPIO_INT_HIGH_1) != 0) {
			gpio->rise_ip = BIT(pin);
			gpio->rise_ie |= BIT(pin);
		}
		if ((trig & GPIO_INT_LOW_0) != 0) {
			gpio->fall_ip = BIT(pin);
			gpio->fall_ie |= BIT(pin);
		}
		break;
	default:
		__ASSERT(false, "Invalid MODE %d passed to driver", mode);
		return -ENOTSUP;
	}

	return 0;
}

static int gpio_aesc_manage_callback(const struct device *dev,
				     struct gpio_callback *callback,
				     bool set)
{
	struct gpio_aesc_data *data = DEV_DATA(dev);

	return gpio_manage_callback(&data->cb, callback, set);
}

static void gpio_aesc_isr(const struct device *dev)
{
	volatile struct gpio_aesc_regs *gpio = DEV_GPIO(dev);
	struct gpio_aesc_data *data = DEV_DATA(dev);
	gpio_port_value_t pins = 0;

	pins |= gpio->rise_ip;
	pins |= gpio->fall_ip;
	pins |= gpio->high_ip;
	pins |= gpio->low_ip;

	gpio->rise_ip = pins;
	gpio->fall_ip = pins;
	gpio->high_ip = pins;
	gpio->low_ip  = pins;

	gpio_fire_callbacks(&data->cb, dev, pins);
}

static int gpio_aesc_init(const struct device *dev)
{
	DEVICE_MMIO_NAMED_MAP(dev, mmio, K_MEM_CACHE_NONE);
	const struct gpio_aesc_config *cfg = DEV_CFG(dev);
	volatile uintptr_t *base_addr =
		(volatile uintptr_t *)DEVICE_MMIO_NAMED_GET(dev, mmio);
	struct gpio_aesc_data *data = DEV_DATA(dev);
	volatile struct gpio_aesc_regs *gpio;
	int ret;

	LOG_DBG("IP core version: %i.%i.%i.",
		ip_id_get_major_version(base_addr),
		ip_id_get_minor_version(base_addr),
		ip_id_get_patchlevel(base_addr)
	);
	data->reg_base = ip_id_relocate_driver(base_addr);
	LOG_DBG("Relocate driver to address 0x%lx.", data->reg_base);
	gpio = DEV_GPIO(dev);

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("failed to apply pinctrl");
		return ret;
	}

	gpio->high_ie = 0;
	gpio->low_ie = 0;
	gpio->rise_ie = 0;
	gpio->fall_ie = 0;

	cfg->irq_config(dev);

	return 0;
}

static DEVICE_API(gpio, gpio_aesc_driver_api) = {
	.pin_configure = gpio_aesc_config,
	.port_get_raw = gpio_aesc_port_get_raw,
	.port_set_masked_raw = gpio_aesc_port_set_masked_raw,
	.port_set_bits_raw = gpio_aesc_port_set_bits_raw,
	.port_clear_bits_raw = gpio_aesc_port_clear_bits_raw,
	.port_toggle_bits = gpio_aesc_port_toggle_bits,
	.pin_interrupt_configure = gpio_aesc_pin_interrupt_configure,
	.manage_callback = gpio_aesc_manage_callback,
};

#define AESC_GPIO_INIT(no)						      \
	PINCTRL_DT_INST_DEFINE(no);					      \
	static struct gpio_aesc_data gpio_aesc_dev_data_##no;		      \
	static void gpio_aesc_irq_config_##no(const struct device *dev)	      \
	{								      \
		IRQ_CONNECT(DT_INST_IRQN(no),				      \
			    DT_INST_IRQ(no, priority),			      \
			    gpio_aesc_isr,				      \
			    DEVICE_DT_INST_GET(no),			      \
			    0);						      \
		irq_enable(DT_INST_IRQN(no));				      \
	}								      \
	static struct gpio_aesc_config gpio_aesc_dev_cfg_##no = {	      \
		.common = GPIO_COMMON_CONFIG_FROM_DT_INST(no),		      \
		DEVICE_MMIO_NAMED_ROM_INIT(mmio, DT_DRV_INST(no)),	      \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(no),		      \
		.irq_config = gpio_aesc_irq_config_##no,		      \
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
